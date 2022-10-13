// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.


#include "KamoBP.h"
#include "KamoModule.h"
#include "KamoSettings.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif


DEFINE_LOG_CATEGORY(LogKamoBP);

#define CHECKARG(ob, errmsg, ret) {if (!(ob)) {UE_LOG(LogKamoBP, Error, errmsg); return ret;}}


UKamoBP* UKamoBP::GetKamoWorld(UWorld* world)
{
	if (!FKamoModule::IsAvailable())
	{
		UE_LOG(LogKamoBP, Error, TEXT("Kamo module not loaded, runtime not available."));
		return nullptr;
	}

	auto rt = FKamoModule::Get().GetKamoRuntime(world);
	if (!rt)
	{
		UE_LOG(LogKamoBP, Error, TEXT("Calling GetKamoRuntime failed."));
		return nullptr;
	}

	auto kamo_bp = NewObject<UKamoBP>();
	kamo_bp->runtime = rt;

	return kamo_bp;
}

AGameModeBase* UKamoBP::GetGameMode()
{
	return runtime->GetWorld()->GetAuthGameMode();
}


UDataTable* UKamoBP::GetKamoTable()
{
	return UKamoRuntime::GetKamoTable();
}


UKamoBP* UKamoBP::GetKamoWorldFromActor(AActor* actor)
{
	CHECKARG(actor, TEXT("GetKamoWorldFromActor: 'actor' must be set."), nullptr);

	return GetKamoWorld(actor->GetWorld());
}


UKamoBP* UKamoBP::GetKamoWorldFromKamoObject(UKamoActor* kamo_object)
{
	CHECKARG(kamo_object, TEXT("GetKamoWorldFromKamoObject: 'kamo_object' must be set."), nullptr);
	CHECKARG(kamo_object->GetObject(), TEXT("GetKamoWorldFromKamoObject: 'kamo_object' has no actor."), nullptr);

	return GetKamoWorld(kamo_object->GetActor()->GetWorld());
}


bool UKamoBP::MoveObject(UKamoID* id, UKamoID* root_id, const FString& spawn_target)
{
	CHECKARG(id && !id->IsEmpty(), TEXT("MoveObject: 'id' must be set."), false);
	CHECKARG(root_id && !root_id->IsEmpty(), TEXT("MoveObject: 'root_id' must be set."), false);

    return runtime->MoveObject(id->GetPrimitive(), root_id->GetPrimitive(), spawn_target);
}


UKamoChildObject* UKamoBP::GetObjectFromActor(AActor* actor) const
{
	CHECKARG(actor, TEXT("GetObjectFromActor: 'actor' must be set."), nullptr);
	return runtime->GetObjectFromActor(actor);
}


UKamoChildObject* UKamoBP::GetObjectFromActorNow(AActor* actor)
{
	CHECKARG(actor, TEXT("GetObjectFromActorNow: 'actor' must be set."), nullptr);
	UKamoBP* KamoBP = GetKamoWorld(actor->GetWorld());
	CHECKARG(KamoBP, TEXT("GetObjectFromActorNow: No KamoBP available."), nullptr);
	return KamoBP->GetObjectFromActor(actor);
}


TMap<FString, UKamoObject*> UKamoBP::GetInternalState() 
{
    return runtime->internal_state;
}


bool UKamoBP::SendMessage(UKamoID* handler_id, const FString& message_type, UKamoState* payload) 
{
	CHECKARG(handler_id && !handler_id->IsEmpty(), TEXT("SendMessage: 'handler_id' must be set."), false);
	CHECKARG(payload, TEXT("SendMessage: 'payload' must be set."), false);

	return runtime->SendMessage(handler_id->GetPrimitive(), message_type, payload->GetStateAsString());
}


UUE4ServerHandler* UKamoBP::GetUE4Server(UKamoID* handler_id) 
{
	CHECKARG(handler_id && !handler_id->IsEmpty(), TEXT("GetUE4Server: 'handler_id' must be set."), nullptr);

	return runtime->GetUE4Server(handler_id->GetPrimitive());
}

UUE4ServerHandler* UKamoBP::GetUE4ServerForRoot(UKamoID* id) 
{
	CHECKARG(id && !id->IsEmpty(), TEXT("GetUE4ServerForRoot: 'id' must be set."), nullptr);

	return runtime->GetUE4ServerForRoot(id->GetPrimitive());
}

UUE4ServerHandler* UKamoBP::GetUE4ServerForChild(UKamoID* id) 
{
	CHECKARG(id && !id->IsEmpty(), TEXT("GetUE4ServerForChild: 'id' must be set."), nullptr);

	return runtime->GetUE4ServerForChild(id->GetPrimitive());
}


AActor* UKamoBP::SpawnActorForKamoObject(UKamoActor* kamo_object)
{
	runtime->SpawnActorForKamoObject(kamo_object);
	return Cast<AActor>(kamo_object->GetObject());
}


/*
FKamoClassMap UKamoBP::GetKamoClass(const FString& class_name)
{
	return runtime->GetKamoClass(class_name);
}
*/

UKamoID* UKamoBP::GetKamoIDFromString(const FString& kamo_id) 
{
	return UKamoRuntime::GetKamoIDFromString(kamo_id);
}

FString UKamoBP::GetCommandLineArgument(const FString& argument) 
{
	return UKamoRuntime::GetCommandLineArgument(argument);
}

UKamoID* UKamoBP::GetUE4ServerID() 
{
	return UKamoID::New(runtime->server_id);
}

TArray<UKamoID*> UKamoBP::GetUE4ServerRegions() 
{
	TArray<UKamoID*> regions;

	for (auto region_id : runtime->GetServerRegions())
	{
		regions.Add(UKamoID::New(region_id));
	}

	return regions;
}


UKamoState* UKamoBP::CreateMessageCommand(UKamoID* kamo_id, UKamoID* root_id, const FString& command, const FString& parameters) 
{
	CHECKARG(kamo_id && !kamo_id->IsEmpty(), TEXT("CreateMessageCommand: 'kamo_id' must be set."), nullptr);
	CHECKARG(root_id && !root_id->IsEmpty(), TEXT("CreateMessageCommand: 'root_id' must be set."), nullptr);

	return UKamoRuntime::CreateMessageCommand(kamo_id->GetPrimitive(), root_id->GetPrimitive(), command, parameters);
}

UKamoState* UKamoBP::CreateCommandMessagePayload(TArray<UKamoState*> commands) 
{
	return UKamoRuntime::CreateCommandMessagePayload(commands);
}

bool UKamoBP::SendCommandToObject(UKamoID* id, const FString& command, const FString& parameters) 
{
	CHECKARG(id && !id->IsEmpty(), TEXT("SendCommandToObject: 'id' must be set."), false);

	return runtime->SendCommandToObject(id->GetPrimitive(), command, parameters);
}


bool UKamoBP::SendCommandsToObject(UKamoID* id, TArray<UKamoState*> commands)
{
	CHECKARG(id && !id->IsEmpty(), TEXT("SendCommandsToObject: 'id' must be set."), false);

	return runtime->SendCommandsToObject(id->GetPrimitive(), commands);
}


bool UKamoBP::EmbedObject(UKamoID* id, UKamoID* container_id, const FString& category)
{
	CHECKARG(id && !id->IsEmpty(), TEXT("EmbedObject: 'id' must be set."), false);
	CHECKARG(container_id && !container_id->IsEmpty(), TEXT("EmbedObject: 'container_id' must be set."), false);

	return runtime->EmbedObject(id->GetPrimitive(), container_id->GetPrimitive(), category);
}

bool UKamoBP::ExtractObject(UKamoID* id, UKamoID* container_id)
{
	CHECKARG(id && !id->IsEmpty(), TEXT("ExtractObject: 'id' must be set."), false);
	CHECKARG(container_id && !container_id->IsEmpty(), TEXT("ExtractObject: 'container_id' must be set."), false);

	return runtime->ExtractObject(id->GetPrimitive(), container_id->GetPrimitive());
}


UKamoID* UKamoBP::FormatRegionName(const FString& map_name, const FString& volume_name, const FString& instance_id)
{	
	KamoID kamo_id(runtime->FormatRegionName(map_name, volume_name, instance_id));
	return UKamoID::New(kamo_id);
}

void UKamoBP::SetEditorUserSettingsBool(FString Key, bool Value)
{
	GConfig->SetBool(TEXT("KamoEd"), *Key, Value, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

void UKamoBP::SetEditorUserSettingsString(FString Key, FString Value)
{
	GConfig->SetString(TEXT("KamoEd"), *Key, *Value, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

void UKamoBP::GetEditorUserSettingsBool(FString Key, bool& Found, bool& Value)
{
	Found = GConfig->GetBool(TEXT("KamoEd"), *Key, Value, GEditorPerProjectIni);
}

void UKamoBP::GetEditorUserSettingsString(FString Key, bool& Found, FString& Value)
{
	Found = GConfig->GetString(TEXT("KamoEd"), *Key, Value, GEditorPerProjectIni);
}

UKamoProjectSettings* UKamoBP::GetKamoSettings()
{
	return UKamoProjectSettings::Get();
}

UKamoState* UKamoBP::KamoStateFromJsonString(const FString& jsonString)
{
	auto kamoState = NewObject<UKamoState>();
	kamoState->SetState(jsonString);
	return kamoState;
}

void UKamoBP::WinSetConsoleTitle(const FString& title)
{
#if PLATFORM_WINDOWS
	::SetConsoleTitleW(*title);
#endif
}

#undef CHECKARG
