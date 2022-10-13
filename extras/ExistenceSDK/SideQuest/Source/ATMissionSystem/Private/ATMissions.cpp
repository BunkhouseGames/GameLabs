// Fill out your copyright notice in the Description page of Project Settings.


#include "ATMissions.h"
#include "Engine/World.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Net/UnrealNetwork.h"

#include "ATMissionsViewmodels.h"

#include "KamoState.h"

DEFINE_LOG_CATEGORY(LogATMission);


void UObjectiveState::AddPlugin(UMissionPlugin* Plugin)
{
	if (!Plugin)
	{
		UE_LOG(LogATMission, Warning, TEXT("'Plugin' variable is unset."), *ATMISSION_FUNC_LINE);
		return;
	}

	plugins.Add(Plugin);
}

UMissionSubsystem::UMissionSubsystem()	
{
}


UMission::UMission()	
{
	// Add head objective
	UObjectiveState* Objective = AddObjective(TEXT("built_in_head_objective"));
	Objective->is_active = true;
}


void UMission::SetDirty()
{
	OnMissionStateChanged.Broadcast(this);

	if (ParentContainer)
	{
		ParentContainer->OnMissionModified(this);
	}
}

UObjectiveState* UMission::GetHeadObjective()
{
	return objectives[0];
}


UObjectiveState* UMission::AddObjective(const FString& ObjectiveId)
{
	UObjectiveState* NewObjective = NewObject<UObjectiveState>();
	NewObjective->objective_id = ObjectiveId;
	objectives.Add(NewObjective);
	return NewObjective;
}


FString UMission::SaveToJson()
{

	if (UMissionsContainer* MC = NewObject<UMissionsContainer>())
	{
		MC->missions.Add(this);
		return MC->SaveAsJsonText();
	}

	return TEXT("");
}


void UMission::PostInitialize()
{
	if (IsInitialized)
	{
		UE_LOG(LogATMission, Warning, TEXT("Mission %s already post-initialized"), *ATMISSION_FUNC_LINE, *mission_id);
		return;
	}

	auto InitializePlugins = [this](UObjectiveState* Objective)
	{
		Objective->Mission = this;

		// In non-linear missions, all objectives are by default active
		if (!is_linear)
		{
			Objective->is_active = true;
		}

		for (UMissionPlugin* Plugin : Objective->plugins)
		{
 			Plugin->Objective = Objective;
			if (Objective->is_active)
			{
				Plugin->OnObjectiveActivated();
			}
		}

		UE_LOG(LogATMission, Display, TEXT("%s: Initialized Objective: %s"), *ATMISSION_FUNC_LINE, *Objective->objective_id);
	};

	// In linear missions with user defined objectives, the first objective needs to be activated automatically
	if (is_linear && objectives.Num() > 1)
	{
		UE_LOG(LogATMission, Display, TEXT("%s: Activate Objective: %s as first in linear chain"), *ATMISSION_FUNC_LINE, *objectives[1]->objective_id);
		objectives[1]->is_active = true;
	}


	for (UObjectiveState* Objective : objectives)
	{
		InitializePlugins(Objective);
	}	

	IsInitialized = true;
	SetDirty();
}

void UMissionPlugin::FinishObjective(bool Failed)
{
	Objective->Mission->FinishObjective(Objective, Failed);
}

UMissionsContainer* UMissionPlugin::GetMissionsContainer()
{
	return Objective->Mission->ParentContainer;
}

void UMission::FinishObjective(UObjectiveState* Objective, bool Failed)
{
	if (!IsInitialized)
	{
		UE_LOG(LogATMission, Error, TEXT("Mission %s not post-initialized"), *ATMISSION_FUNC_LINE, *mission_id);
		return;
	}

	// Objective must be active and not completed 
	if (Objective->is_completed || !Objective->is_active)
	{
		UE_LOG(LogATMission, Warning, TEXT("%s: Objective %s is either completed or not active"), *ATMISSION_FUNC_LINE, *Objective->objective_id);
		return;
	}

	Objective->is_completed = true;
	Objective->failed = Failed;

	UE_LOG(LogATMission, Display, TEXT("%s: Objective Completed: %s"), *ATMISSION_FUNC_LINE, *Objective->objective_id);
	// Notify all plugins of this state change
	for (auto Plugin : Objective->plugins)
	{
		Plugin->OnObjectiveCompleted();
	}

	if (Objective != GetHeadObjective())
	{
		// If linear activate the next objective
		if (is_linear)
		{
			for (int i = 0; i < objectives.Num() - 1; i++)
			{
				if (objectives[i] == Objective)
				{
					// activate the next one
					objectives[i + 1]->is_active = true;
					// Notify plugins

						// Notify all plugins of this state change
					for (auto Plugin : objectives[i + 1]->plugins)
					{
						Plugin->OnObjectiveCompleted();
					}
					break;
				}

			}
		}

		// check if all objectives are completed
		bool AllCompleted = true;
		for (auto Ob : objectives)
		{
			if (Ob != GetHeadObjective() && !Ob->is_completed)
			{
				AllCompleted = false;
				break;
			}
		}

		if (AllCompleted)
		{
			// This is probably not the right place to do this :) we can remove this in the future once the missions blueprints have been refactored so that its easier to grab this event
			// TODO, remove this code and add it to the blueprints
			FString missionType, missionId;

			this->mission_id.Split(".", &missionType, &missionId);

			FNotification MissionCompletedEvent;
			MissionCompletedEvent.Id = this->mission_id;
			MissionCompletedEvent.IdType = "mission";
			MissionCompletedEvent.MessageType = "completed";
			MissionCompletedEvent.AnalyticsEvent = "mission:" + missionType + ":" + "succeeded";
			this->ParentContainer->Notifications.Add(MissionCompletedEvent);

			// Mark mission as finished and notify 
			FinishObjective(GetHeadObjective(), Failed);
		}
	}
	SetDirty();
}


UMissionsContainer* UMissionsContainer::CreateFromJsonText(const FString& JsonText)
{
	if (UMissionsContainer* MC = NewObject<UMissionsContainer>())
	{
		TSharedPtr<FJsonObject> JsonObject;
		if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(*JsonText), JsonObject))
		{
			if (FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), UMissionsContainer::StaticClass(), MC))
			{
				return MC;
			}
		}
	}

	return nullptr;
}


FString UMissionsContainer::SaveAsJsonText() const
{
	FString JsonText;
	FJsonObjectConverter::UStructToJsonObjectString(GetClass(), this, JsonText, 0, 0);

	return JsonText;
}


void UMissionsContainer::OnMissionModified(UMission* Mission)
{
	if (GetOwner()->HasAuthority())
	{
		// Notify interested party if applicable
		if (MissionEventsInterface.GetObject())
		{
			MissionEventsInterface->Execute_OnMissionModified(MissionEventsInterface.GetObject(), Mission);
		}
	}
	else
	{
		UE_LOG(LogATMission, Error, TEXT("%s: Missions should not be modified on client, mission_id: %s"), *ATMISSION_FUNC_LINE, *Mission->mission_id);
	}
}

void UMissionsContainer::PostInitialize()
{
	for (UMission* Mission : missions)
	{
		PostInitializeMission(Mission);
	}
}


void UMissionsContainer::AddMission(UMission* Mission)
{
	missions.Add(Mission);
	PostInitializeMission(Mission);
}


void UMissionsContainer::PostInitializeMission(UMission* Mission)
{
	if (!Mission->IsInitialized)
	{
		Mission->ParentContainer = this;
		Mission->PostInitialize();
	}
}


void UMissionsContainer::UpdateKamoStateFromActor_Implementation(UKamoState* state)
{
	state->SetStateFromProperty(this, "missions");
}


// Converts from an array of json values to an array of UObjects.
// Note: The 'OutArray' will only contain successfully created and serialized objects.
template<typename OutObjectType>
static bool JsonArrayToUStruct(const TArray<TSharedPtr<FJsonValue>>& JsonArray, TArray<OutObjectType*>& OutArray, UObject* Outer = nullptr, int64 CheckFlags = 0, int64 SkipFlags = 0)
{
	OutArray.Empty();
	for (int32 i = 0; i < JsonArray.Num(); ++i)
	{
		const auto& JsonValue = JsonArray[i];
		if (JsonValue->Type != EJson::Object)
		{
			UE_LOG(LogATMission, Warning, TEXT("%s: Array element [%i] was not an object. The type id is %i"), *ATMISSION_FUNC_LINE, i, (int)JsonValue->Type);
			continue;
		}

		if (!Outer)
		{
			Outer = GetTransientPackage();
		}

		const FString ObjectClassNameKey = "_ClassName";
		TSharedPtr<FJsonObject> Obj = JsonValue->AsObject();		

		// If a specific subclass was stored in the Json, use that instead of the ElementClass
		UClass* ElementClass = OutObjectType::StaticClass();
		FString ClassString = Obj->GetStringField(ObjectClassNameKey);		
		if (!ClassString.IsEmpty())
		{
			Obj->RemoveField(ObjectClassNameKey);
			UClass* FoundClass = FindObject<UClass>(ANY_PACKAGE, *ClassString);
			if (FoundClass)
			{
				ElementClass = FoundClass;
			}
		}

		UObject* createdObj = StaticAllocateObject(ElementClass, Outer, NAME_None, EObjectFlags::RF_NoFlags, EInternalObjectFlags::None, false);
		(*ElementClass->ClassConstructor)(FObjectInitializer(createdObj, ElementClass->ClassDefaultObject, EObjectInitializerOptions::None));

		check(Obj.IsValid()); // should not fail if Type == EJson::Object

		if (!FJsonObjectConverter::JsonObjectToUStruct(Obj.ToSharedRef(), createdObj->GetClass(), createdObj, CheckFlags, SkipFlags))
		{
			UE_LOG(LogATMission, Error, TEXT("%s: FJsonObjectConverter::JsonObjectToUStruct failed for sumthin"), *ATMISSION_FUNC_LINE);
			continue;
		}

		OutArray.Add((OutObjectType*)createdObj);
	}
	return true;
}

void UMissionsContainer::ApplyKamoStateToActor_Implementation(UKamoState* state)
{
	const TSharedPtr< FJsonObject >& JsonObject = state->GetJsonObjectState();
	TArray<TSharedPtr<FJsonValue>> JsonArray = JsonObject->GetArrayField("missions");
	JsonArrayToUStruct(JsonArray, missions, this);
	PostInitialize();
}


void UMissionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UGameInstance* gi = GetGameInstance();
	UWorld* world = gi->GetWorld();
	world->IsGameWorld();
	UE_LOG(LogATMission, Display, TEXT("%s: Map loaded: %s"), *ATMISSION_FUNC_LINE, *world->GetMapName());

	// BELOW IS JUST DEVELOPMENT TEST CODE. IN CASE YOU WERE WONDERING.
	/*
	FString JsonText;
	UMissionsContainer* MC = nullptr;

	if (FFileHelper::LoadFileToString(JsonText, TEXT("c:\\temp\\atmission.json")))
	{
		MC = UMissionsContainer::CreateFromJsonText(JsonText);
	}

	
	if (MC && MC->missions.Num())
	{
		UMission* Ob = MC->missions[0];
		wwd
		FPlugin Plugin;
		Plugin.classname = TEXT("trigger");
		Plugin.type = EMissionPluginType::precondition;
		Plugin.plugin = NewObject<UMissionPluginTrigger>();
		//Ob->MissionState.objectives[0].plugins.Add(Plugin);
		//TSharedPtr<FJsonObject> JsonObject = Ob->SaveStateToJsonObject();
		
		JsonText = MC->SaveAsJsonText();
			
		FFileHelper::SaveStringToFile(JsonText, TEXT("c:\\temp\\atmission_out.json"));
	}
	*/
}


void UMissionSubsystem::Deinitialize()
{
	Super::Deinitialize();
}


UMissionSubsystem* UMissionSubsystem::Get(UObject* WorldContextObject)
{
	const auto World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	if (World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE))
	{
		return World->GetGameInstance()->GetSubsystem<UMissionSubsystem>();
	}

	return CastChecked<UMissionSubsystem>(UMissionSubsystem::StaticClass()->GetDefaultObject());
}



void UMissionPlugin::OnActive_Implementation(UObject* obj)
{

}

UMissionPlugin::UMissionPlugin()
{
	//UE_LOG(LogATMission, Display, TEXT("umISSIONSPLUGIN LOADING"));
}

UClass* UMissionPlugin::GetMissionPluginVMClass_Implementation()
{
	return UMissionPluginVM::StaticClass();	
}


UMissionPluginTrigger::UMissionPluginTrigger()
{
	
}


