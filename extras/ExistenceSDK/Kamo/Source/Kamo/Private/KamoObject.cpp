// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.


#include "KamoObject.h"

#include "KamoModule.h"
#include "KamoPersistable.h"
#include "KamoRt.h"
#include "KamoRuntime.h" // just for log category, plz fix
#include "IKamoComponentReflection.h"


// Stats
DECLARE_CYCLE_STAT(TEXT("KamoObject"), STAT_UpdateKamoState, STATGROUP_Kamo);
DECLARE_CYCLE_STAT(TEXT("PreCheckIfDirty"), STAT_PreCheckIfDirty, STATGROUP_Kamo);
DECLARE_CYCLE_STAT(TEXT("CheckIfDirty"), STAT_CheckIfDirty, STATGROUP_Kamo);


#define CHECKARG(ob, errmsg, ret) {if (!(ob)) {UE_LOG(LogKamoRt, Error, errmsg); return ret;}}


UKamoObject::UKamoObject() :
	object(nullptr),
	KamoDirtyProp(nullptr),
	id(NewObject<UKamoID>()),
	state(NewObject<UKamoState>())
{
	//UE_LOG(LogKamoRt, Warning, TEXT("UKamoObject: initializieidid"));
}


UKamoRuntime* UKamoObject::GetKamoRuntime(UWorld* InWorld)
{
	return FKamoModule::Get().GetKamoRuntime(InWorld);
}


UKamoRuntime* UKamoObject::GetKamoRuntime() const
{
	return FKamoModule::Get().GetKamoRuntime(GetWorld());
}


TArray<UClass*> UKamoObject::KamoClasses;

void UKamoObject::PostCDOContruct()
{
	KamoClasses.Add(GetClass());
}


void UKamoObject::SetObject(UObject* InObject)
{
	object = InObject;
	if (object)
	{
		UClass* ObjectClass = object->GetClass();
		FProperty* Property = ObjectClass->FindPropertyByName(FName(TEXT("KamoDirty")));
		KamoDirtyProp = CastField<FBoolProperty>(Property);
	}
	else
	{
		KamoDirtyProp = nullptr;
	}
}


// Give objects chance of doing post initialization after all objects have been loaded in.
void UKamoObject::ResolveSubobjects(UKamoRuntime* runtime)
{
	// We only resolve subobjects if the UObject or AActor instance is available
	if (object == nullptr)
	{
		//return;
	}

	// Resolve all Kamo object references
	kamo_subobjects.Empty();
	UKamoState* subobjects = nullptr;
	state->GetKamoState("kamo_subobjects", subobjects);
	if (!subobjects)
	{
		return;
	}

	for (const FString& key : subobjects->GetKamoStateKeys())
	{
		FString kamo_id_str;
		subobjects->GetString(key, kamo_id_str);
		UKamoObject* kamo_object = runtime->GetObject(KamoID(kamo_id_str), /*fail_silently*/true);		

		if (!kamo_object)
		{
			// The object may not be loaded yet, or it is in a different region or does not exist in the DB at all. We must distinguish between all 
			// those cases.
			EKamoLoadChildObject result = runtime->LoadChildObjectFromDB(KamoID(kamo_id_str));

			if (result == EKamoLoadChildObject::Success)
			{
				kamo_object = runtime->GetObject(KamoID(kamo_id_str), /*fail_silently*/false);
			}
			else if (result == EKamoLoadChildObject::NotOurRegion)
			{
				UE_LOG(LogKamoRt, Display, TEXT("ResolveSubobjects: Subobject '%s:%s' not loaded, it's in a different region."), *key, *kamo_id_str);
				unresolved_subobjects.Add(key, kamo_id_str);
			}
			else if (result == EKamoLoadChildObject::RegisterFailed)
			{
				UE_LOG(LogKamoRt, Error, TEXT("ResolveSubobjects: Subobject '%s:%s' failed to register."), *key, *kamo_id_str);
				unresolved_subobjects.Add(key, kamo_id_str);
			}
			else if (result == EKamoLoadChildObject::NotFound)
			{
				UE_LOG(LogKamoRt, Error, TEXT("ResolveSubobjects: Subobject '%s:%s' does not exist. The reference will be removed permanently!."), *key, *kamo_id_str);
				// By not adding it to 'unresolved_subobjects' the reference is removed for good.
			}
		}		
		else
		{
			kamo_subobjects.Add(key, kamo_object);
		}
	}	
}

bool UKamoObject::SubobjectKeyExists(const FString& object_key) const
{
	return kamo_subobjects.Contains(object_key) || unresolved_subobjects.Contains(object_key);
}


TMap<FString, UKamoObject*> UKamoObject::GetSubobjects() const
{
	return kamo_subobjects;
}


void UKamoObject::SetSubobject(const FString& object_key, UKamoObject* kamo_object)
{
	if (kamo_object == nullptr)
	{
		UE_LOG(LogKamoRt, Error, TEXT("SetSubobject for '%s': Kamo object is invalid (null)."), *object_key);
		return;
	}

	if (UKamoActor* kamo_actor = Cast<UKamoActor>(kamo_object))
	{
		kamo_actor->object_ref_mode = EObjectRefMode::RM_AttachManually;
	}

	kamo_subobjects.Add(object_key, kamo_object);
	dirty = true;
}


AActor* UKamoObject::SpawnSubobject(const FString& object_key, bool use_spawn_target)
{
	if (unresolved_subobjects.Contains(object_key))
	{
		UE_LOG(LogKamoRt, Warning, TEXT("SpawnSubobject: Subobject '%s:%s' is in a different region."), *object_key, *unresolved_subobjects[object_key]());
		return nullptr;
	}

	if (!kamo_subobjects.Contains(object_key))
	{
		UE_LOG(LogKamoRt, Warning, TEXT("SpawnSubobject: Subobject '%s' does not exist."), *object_key);
		return nullptr;
	}

	UKamoActor* kamo_object = Cast<UKamoActor>(kamo_subobjects[object_key]);

	if (kamo_object == nullptr)
	{
		UE_LOG(LogKamoRt, Warning, TEXT("SpawnSubobject: Subobject '%s' is not an actor object."), *object_key);
		return nullptr;
	}
		
	UKamoRuntime* runtime = GetKamoRuntime();

	if (runtime == nullptr)
	{
		UE_LOG(LogKamoRt, Warning, TEXT("SpawnSubobject: No KamoRuntime to spawn subobject '%s."), *object_key);
		return nullptr;
	}
	

	FTransform* transform = nullptr;
	FTransform spawn_target_transform;
	if (use_spawn_target)
	{
		FString spawn_target;
		if (state->GetString("spawn_target", spawn_target))
		{
			if (spawn_target.IsEmpty() || spawn_target == TEXT("ignore"))
			{
				// Use the transform from the Kamo object
			}
			else if (spawn_target == TEXT("playerstart"))
			{
				// Find a player start actor and use its transform
				if (AActor* PlayerStart = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()))
				{
					spawn_target_transform = PlayerStart->GetTransform();
				}
				else
				{
					UE_LOG(LogKamoRt, Warning, TEXT("SpawnSubobject: Spawn target specified as 'playerstart' but no APlayerStart actor exists in the level . Spawn Id: '%s."), *object_key);
				}
			}
			else
			{
				// Get transform of spawn target object
				if (UKamoObject* SpawnTargetObject = runtime->GetObject(KamoID(spawn_target), /*fail_silently*/false))
				{
					if (AActor* SpawnTargetActor = Cast<AActor>(SpawnTargetObject->GetObject()))
					{
						spawn_target_transform = SpawnTargetActor->GetTransform();
					}
				}
			}
			transform = &spawn_target_transform;
		}
	}

	runtime->SpawnActorForKamoObject(kamo_object, transform);
	return Cast<AActor>(kamo_object->GetObject());
}


bool UKamoObject::IsReadyForFinishDestroy()
 {
	if (object)
	{
		// Wait if any serialization is pending
		UKamoRuntime* runtime = GetKamoRuntime();
		if (runtime)
		{
			return runtime->IsReadyForFinishDestroy();
		}
	}
	return Super::IsReadyForFinishDestroy();
}


void UKamoObject::MarkForUpdate()
{
	if (KamoDirtyProp && object)
	{
		if (KamoDirtyProp->GetPropertyValue_InContainer(object))
		{
			dirty = true;
			KamoDirtyProp->SetPropertyValue_InContainer(object, false);
		}
	}
	
	if (dirty)
	{
		return;
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_PreCheckIfDirty);
		dirty = PreCheckIfDirty();
	}
	
	if (!dirty && check_if_dirty)
	{
		SCOPE_CYCLE_COUNTER(STAT_CheckIfDirty);
		dirty = CheckIfDirty();
	}
}


void UKamoObject::UpdateKamoStateFromActor_Implementation() 
{
	SCOPE_CYCLE_COUNTER(STAT_UpdateKamoState);
	auto current_time = FDateTime::UtcNow().ToIso8601();
	state->SetString("timestamp", current_time);
	state->SetBool("actor_deleted", actor_deleted);
	if (IsValid(object) && IsValid(state))
	{
		if (object->GetClass()->ImplementsInterface(UKamoObjectInterface::StaticClass()))
		{
			IKamoObjectInterface::Execute_UpdateKamoStateFromObject(object, state);
		}		
	}


	// Write out all Kamo object references
	UKamoState* subs = NewObject<UKamoState>();
	for (auto it = kamo_subobjects.CreateIterator(); it; ++it)
	{
		subs->SetString(it.Key(), it.Value()->id->GetID());
	}
	for (auto it = unresolved_subobjects.CreateIterator(); it; ++it)
	{
		subs->SetString(it.Key(), it.Value()());
	}

	state->SetKamoState("kamo_subobjects", subs);
}

void UKamoObject::ApplyKamoStateToActor_Implementation(EKamoStateStage stage_stage)
{
	if (IsValid(object) && IsValid(state))
	{
		if (object->GetClass()->ImplementsInterface(UKamoObjectInterface::StaticClass()))
		{
			IKamoObjectInterface::Execute_ApplyKamoStateToObject(object, stage_stage, state);
		}		
	}	
}

bool UKamoObject::CheckIfDirty_Implementation() {
    return false;
}

bool UKamoObject::PreCheckIfDirty() {
	return false;
}

void UKamoObject::HandleInboxCommand_Implementation(UKamoCommand* command) {
    
}


UKamoState* UKamoObject::GetKamoState_Implementation() {
    if (!state) {
        state = NewObject<UKamoState>();
    }
    
    return state;
}

bool UKamoObject::SendCommand_Implementation(const FString& command, UKamoState* args)
{
	UKamoRuntime* runtime = GetKamoRuntime();
	return runtime->SendCommandToObject(
		id->GetPrimitive(), 
		command, 
		args->GetStateAsString()
	);
}

bool UKamoObject::OnRegistered_Implementation()
{
	return false;
}


bool UKamoObject::OnCommandReceived_Implementation(const FString& command, UKamoState* args)
{
	UE_LOG(LogKamoRt, Warning, TEXT("OnCommandReceived: Not handler for: %s"), *command);
	return false;
}


void UKamoObject::OnObjectSet_Implementation(UObject* obj, UKamoRuntime* runtime) 
{
	// Overwritten in blueprints for any custom logic needed to happen before the object is moved
	if (!obj) 
	{
		UE_LOG(LogKamoRt, Error, TEXT("OnObjectSet: Object refernce is null. This should never happen."));
		return;
	}
}


AGameModeBase* UKamoObject::GetGamemodeFromWorld()
{
	return UGameplayStatics::GetGameMode(GetOuter());
}


void UKamoObject::OnObjectUnSet_Implementation(UObject* obj) {

	// Overwritten in blueprints for any custom logic needed to happen before the object is moved
	UE_LOG(LogKamoRt, Error, TEXT("OnObjectUnSet: Not implemented."));
}

UKamoRootObject::UKamoRootObject() :
	handler_id(NewObject<UKamoID>())
{
}




UKamoChildObject::UKamoChildObject() :
	root_id(NewObject<UKamoID>())
{
	//UE_LOG(LogKamoRt, Display, TEXT("UKamoChildObject: initializieidid"));
}


UKamoMessage::UKamoMessage() :
	sender(NewObject<UKamoID>()),
	payload(NewObject<UKamoState>())
{
}


// Kamo Actor
static EObjectRefMode RefmodeFromString(const FString& refmode, UKamoID* id)
{
	EObjectRefMode object_ref_mode = EObjectRefMode::RM_Undefined;

	if (refmode == "spawn_object")
	{
		object_ref_mode = EObjectRefMode::RM_SpawnObject;
	}
	else if (refmode == "attach_by_tag")
	{
		object_ref_mode = EObjectRefMode::RM_AttachByTag;
	}
	else if (refmode == "attach_manually")
	{
		object_ref_mode = EObjectRefMode::RM_AttachManually;
	}
	else
	{
		UE_LOG(LogKamoRt, Error, TEXT("Invalid 'object_ref_mode' in %s: %s"), *refmode, *id->GetID());
	}

	return object_ref_mode;
}


FString StringFromRefmode(EObjectRefMode object_ref_mode)
{
	switch (object_ref_mode)
	{
	case EObjectRefMode::RM_SpawnObject:
		return "spawn_object";
	case EObjectRefMode::RM_AttachByTag:
		return "attach_by_tag";
	case EObjectRefMode::RM_AttachManually:
		return "attach_manually";
	case EObjectRefMode::RM_Undefined:
		return "(undefined)";
	default:
		return "(unknown)";
	}
}


UKamoActor::UKamoActor() :
	object_ref_mode(EObjectRefMode::RM_SpawnObject),
	actor_is_set(false)
{
	//UE_LOG(LogKamoRt, Display, TEXT("UKamoActor: initializieidid"));
}



void UKamoActor::ProcessOnActorSpawned(AActor* Actor, UKamoRuntime* KamoRuntime)
{
	// If the class map contains a registration for this actor we simply forward this
	// event to the CDO instance for further processing.
	TKamoClassMapEntry ClassInfo = KamoRuntime->GetKamoClassEntryFromActor(Actor);
	if (!ClassInfo.Key.IsEmpty())
	{
		// Get the CDO and forward the event
		UKamoActor* KamoActor = Cast<UKamoActor>(ClassInfo.Value->kamo_class.GetDefaultObject());
		KamoActor->OnActorSpawnedCDO(Actor, KamoRuntime, ClassInfo);
	}
}


void UKamoActor::OnActorSpawnedCDO(AActor* Actor, UKamoRuntime* KamoRuntime, const TKamoClassMapEntry& ClassEntry)
{
	// Note, this is only called for CDO instances to support override of this function.
	UKamoID* RegionId = MapSpawnedActorToRegion(Actor, KamoRuntime);
	KamoRuntime->CreateObject(ClassEntry.Key, RegionId->GetPrimitive(), Actor);
}


void UKamoActor::OnKamoObjectNotFoundCDO(FIdentityActor& InIdentityActor)
{
}


void UKamoActor::OnKamoInstanceAttached_Implementation()
{
}


void UKamoActor::OnKamoInstanceDetached_Implementation()
{
}


void UKamoActor::OnKamoInstanceError_Implementation(const FString& ErrorText)
{
}


void UKamoActor::PreProcessState_Implementation() 
{
	// Read in Kamo core properties for KamoActor
	FString ref_mode_str;
	if (state->GetString("object_ref_mode", ref_mode_str))
	{
		object_ref_mode = RefmodeFromString(ref_mode_str, id);
	}
	else
	{
		UE_LOG(LogKamoRt, Error, TEXT("KamoActor: In %s 'object_ref_mode' missing from state. Continuing with class' default."), *id->GetID());
	}
}


void UKamoActor::OnObjectSet_Implementation(UObject* obj, UKamoRuntime* runtime)
{
	Super::OnObjectSet_Implementation(obj, runtime);
	
	// When manually attached actors get destroyed the Kamo object itself is not destroyd but it must
	// be serialized to DB though before we lose the state forever. We hook into the OnDestroyed event
	// so we can trigger a DB flush.
	GetActor()->OnDestroyed.AddDynamic(this, &UKamoActor::OnActorDestroyed);
}



void UKamoActor::OnActorDestroyed(AActor* destroyed_actor)
{
	if (!actor_is_set)
	{
		UE_LOG(LogKamoRt, Error, TEXT("OnActorDestroyed: The flag 'actor_is_set' isn't set for %s"), *destroyed_actor->GetName());
		return;
	}

	if (destroyed_actor != GetActor())
	{
		UE_LOG(LogKamoRt, Error, TEXT("OnActorDestroyed: Destroyed actor isn't our actor: %s"), *destroyed_actor->GetName());
		return;
	}
	
	// TODO: Add support to specify what happens when actor gets destroyed. The possible scenarios are:
	// 1. Delete Kamo object as well
	// 2. Mark actor as deleted. Level actors will be destroyed on next level load.
	// 3. Do nothing. Kamo object stays as is but without any actor attached.
	//
	// Currently the default behavior is to do 1, 2 and 3 for spawned, attach by tag and manually attached respectively.

	if (object_ref_mode == EObjectRefMode::RM_SpawnObject)
	{
		// This means an actor was assigned at some point but is no more. We delete this Kamo object.
		// If at any point we want to keep track of deleted object, we must flush out the state and
		// move it 
		deleted = true;
	}
	else if (object_ref_mode == EObjectRefMode::RM_AttachByTag)
	{
		// Mark actor as deleted but keep the Kamo object. Next time the level is loaded this
		// actor will be destroyed on startup.
		actor_deleted = true;

		// Flush out state to DB before the actor gets completely destroyed.
		MarkForUpdate();
		if (dirty)
		{
			UKamoRuntime* runtime = GetKamoRuntime();
			if (runtime)
			{
				runtime->SerializeObjects();
			}
		}

	}
	else if (object_ref_mode == EObjectRefMode::RM_AttachManually)
	{
		// Manually attached actor, must flush out state to DB before the actor gets completely destroyed.
		MarkForUpdate();
		if (dirty)
		{
			UKamoRuntime* runtime = GetKamoRuntime();
			if (runtime)
			{
				runtime->SerializeObjects();
			}
		}
	}

	object = nullptr;
}


void UKamoActor::GetSaveGameProperties(UObject* Object, TArray<FString>& Properties)
{
	for (TFieldIterator<FProperty> PropIt(Object->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		if (Property->HasAnyPropertyFlags(CPF_SaveGame))
		{
			Properties.AddUnique(Property->GetName());
		}
	}
}

void UKamoActor::ApplyKamoStateToActor_Implementation(EKamoStateStage stage_stage)
{
	Super::ApplyKamoStateToActor_Implementation(stage_stage);

	AActor* Actor = GetActor();
	bool ImplementsKamoPersistable = Actor->Implements<UKamoPersistable>();
	if (stage_stage == EKamoStateStage::KSS_ActorPass)
	{
		FString object_ref_mode_str;
		if (state->GetString("object_ref_mode", object_ref_mode_str))
		{
			object_ref_mode = RefmodeFromString(object_ref_mode_str, id);
		}

		state->GetBool("actor_deleted", actor_deleted);
		if (actor_deleted)
		{
			Actor->Destroy();
		}
		else
		{
			if (ImplementsKamoPersistable)
			{
				TArray<FString> Properties = IKamoPersistable::Execute_GetKamoPersistedProperties(Actor);
				GetSaveGameProperties(Actor, Properties);
				state->SetPropertiesFromState(Actor, Properties);				
			}
			else
			{
				state->SetPropertiesFromState(Actor, PersistedProperties);
			}

			FTransform transform;
			auto exists = state->GetTransform("transform", transform);

			if (!Actor || !Actor->IsRootComponentMovable() || !exists)
			{
				return;
			}

			FHitResult hit_result;
			Actor->SetActorTransform(transform, false, &hit_result, ETeleportType::TeleportPhysics);
		}
	}
	else if (stage_stage == EKamoStateStage::KSS_ComponentPass)
	{
		// Run components reflection
		for (auto Component : Actor->GetComponents())
		{
			if (Component->Implements<UKamoComponentReflection>())
			{
				IKamoComponentReflection::Execute_ApplyKamoStateToActor(Component, state);
			}
			if (Component->Implements<UKamoPersistable>())
			{
				TArray<FString> Properties = IKamoPersistable::Execute_GetKamoPersistedProperties(Component);
				GetSaveGameProperties(Component, Properties);
				UKamoState* ComponentState = UKamoState::CreateKamoState();
				ComponentState->PopulateFromField(state, Component->GetName());
				ComponentState->SetPropertiesFromState(Component, Properties);
				IKamoPersistable::Execute_KamoAfterLoad(Component);
			}
		}
	
		if (ImplementsKamoPersistable)
		{
			IKamoPersistable::Execute_KamoAfterLoad(Actor);
		}
	}
}


void UKamoActor::UpdateKamoStateFromActor_Implementation() {
	Super::UpdateKamoStateFromActor_Implementation();

	state->SetString("object_ref_mode", StringFromRefmode(object_ref_mode));

	AActor* Actor = GetActor();
	if (Actor)
	{
		auto transform = Actor->GetTransform();
		state->SetTransform("transform", transform);

		// Run components reflection
		for (auto Component : Actor->GetComponents())
		{
			if (Component->Implements<UKamoComponentReflection>())
			{
				IKamoComponentReflection::Execute_UpdateKamoStateFromActor(Component, state);
			}

			if (Component->Implements<UKamoPersistable>())
			{
				IKamoPersistable::Execute_KamoBeforeSave(Component);
				TArray<FString> Properties = IKamoPersistable::Execute_GetKamoPersistedProperties(Component);
				GetSaveGameProperties(Component, Properties);
				UKamoState* ComponentState = UKamoState::CreateKamoState();
				ComponentState->SetStateFromProperties(Component, Properties);
				state->SetObjectField(Component->GetName(), ComponentState);
			}
		}
	}

	// Do a simple flat dump of everything
	TArray<UKamoState*> dump;

	for (auto kv : embedded_objects)
	{
		auto embeded = kv.Value;
		UKamoState* tmp = NewObject<UKamoState>();
		tmp->SetState(embeded.json_state);
		tmp->SetString("_category", embeded.category);
		tmp->SetString("_id", embeded.kamo_id);
		dump.Add(tmp);		
	}

	state->SetKamoStateArray("collection", dump);

	bool ImplementsKamoPersistable = Actor->Implements<UKamoPersistable>();
	if (ImplementsKamoPersistable)
	{
		IKamoPersistable::Execute_KamoBeforeSave(Actor);
		TArray<FString> Properties = IKamoPersistable::Execute_GetKamoPersistedProperties(Actor);
		GetSaveGameProperties(Actor, Properties);
		state->SetStateFromProperties(object, Properties);
	} else
	{
		state->SetStateFromProperties(object, PersistedProperties);		
	}
}


bool UKamoActor::PreCheckIfDirty() 
{
	if (GetActor())
	{
		FTransform state_transform;
		if (!state->GetTransform("transform", state_transform))
		{
			return true;
		}

		auto actor_transform = GetActor()->GetTransform();

		if (!actor_transform.Equals(state_transform, 0.001f))
		{
			return true;
		}
	}

	return Super::PreCheckIfDirty();
}


void UKamoActor::OnMove(const KamoID& target_region_id)
{
	// When a Kamo object is moved to a different region we remove the associated actor from the scene.
	if (AActor* Actor = GetActor())
	{
		Actor->Destroy();
	}
}


UKamoID* UKamoActor::MapSpawnedActorToRegion_Implementation(AActor* new_actor, class UKamoRuntime* current_runtime)
{
	auto region_name = current_runtime->MapActorToRegion(new_actor);
	auto region_id = NewObject<UKamoID>();
	region_id->Init(region_name);
	return region_id;
}


FDelegateHandle UKamoPlayerController::PostLoginEventBinding;


UKamoPlayerController::UKamoPlayerController()
{
	object_ref_mode = EObjectRefMode::RM_AttachManually;

	if (!PostLoginEventBinding.IsValid())
	{
		// Grab login event for PlayerController logic
		PostLoginEventBinding = FGameModeEvents::GameModePostLoginEvent.AddStatic(&UKamoPlayerController::OnGameModePostLogin);
	}
}


void UKamoPlayerController::OnActorSpawnedCDO(AActor* Actor, UKamoRuntime* KamoRuntime, const TKamoClassMapEntry& ClassEntry)
{
	// We override this function to skip the automatic Kamo object creation as we want to map player controller objects to
	// specific kamo id's. This is all done in OnGameModePostLogin and we simply do nothing here.
}


void UKamoPlayerController::OnKamoObjectNotFoundCDO(FIdentityActor& InIdentityActor)
{
	// There is no Kamo player record in the DB matching this Id. Let's create a new one.
	InIdentityActor.CreateKamoObject();
}


void UKamoPlayerController::OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	UKamoRuntime* KamoRuntime = GetKamoRuntime(NewPlayer->GetWorld());
	if (KamoRuntime == nullptr)
	{
		return;
	}

	UE_LOG(LogKamoRt, Display, TEXT("OnGameModePostLogin: %s - %s"), *AActor::GetDebugName(GameMode), *AActor::GetDebugName(NewPlayer));

	if (NewPlayer->Player)
	{
		if (UNetConnection* Connection = Cast<UNetConnection>(NewPlayer->Player))
		{
			FString UniqueId = Connection->PlayerId->ToString();
			
			FString PlayerIdStr = Connection->PlayerId.IsValid() ? *Connection->PlayerId->ToDebugString() : TEXT("nullptr");
			FString Desc = Connection->LowLevelDescribe();
			UE_LOG(LogKamoRt, Display, TEXT("   New player: %s %s"), *PlayerIdStr, *Desc);			

#if !UE_BUILD_SHIPPING
			// Developer Aid: If the OnlineIdentityNull provider is active and we are in PIE, the PlayerId will always be some
			// random string composed of <hostname>-<random guid>. See OnlineIdentityNull::GenerateRandomUserId() where it
			// doesn't want to grant a stable id if we are in PIE.
			FURL InURL(NULL, *Connection->RequestURL, TRAVEL_Absolute);
			if (NewPlayer->GetWorld()->WorldType == EWorldType::PIE && InURL.HasOption(TEXT("Name")))
			{
				// Use the "Name" option from the URL which contains the nickname from ULocalKamoPlayer::GetNickname() if configured so.
				UniqueId = InURL.GetOption(TEXT("Name="), TEXT("_notused"));
			}
#endif
			UE_LOG(LogKamoRt, Display, TEXT("OnGameModePostLogin: Player controller id: %s"), *UniqueId);
			KamoRuntime->AddIdentityActor(UniqueId, NewPlayer);
		}
	}
}

void UKamoPlayerController::OnKamoInstanceDetached_Implementation()
{
	// Unpossess pawn and spawn spectator pawn
	if (APlayerController* PlayerController = Cast<APlayerController>(GetObject()))
	{
		PlayerController->StartSpectatingOnly();
	}
}


void UKamoPlayerController::OnMove(const KamoID& target_region_id)
{
	// When a Kamo player controller object is moved to a different region we ignore
	// it because we are going into spectator mode in response to OnKamoInstanceDetached event.
}


#undef CHECKARG
