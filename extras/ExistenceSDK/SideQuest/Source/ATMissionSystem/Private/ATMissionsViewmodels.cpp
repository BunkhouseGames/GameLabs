// Fill out your copyright notice in the Description page of Project Settings.


#include "ATMissionsViewmodels.h"








UWorld* UReplicatedObject::GetWorld() const
{
	if (const UObject* MyOuter = GetOuter())
	{
		return MyOuter->GetWorld();
	}
	return nullptr;
}


AActor* UReplicatedObject::GetOwningActor() const
{
	return GetTypedOuter<AActor>();
}


void UReplicatedObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// Add any Blueprint properties
	// This is not required if you do not want the class to be "Blueprintable"
	if (const UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass()))
	{
		BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}
}


bool UReplicatedObject::IsSupportedForNetworking() const
{
	return true;
}


int32 UReplicatedObject::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	check(GetOuter() != nullptr);
	return GetOuter()->GetFunctionCallspace(Function, Stack);
}


bool UReplicatedObject::CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms, FFrame* Stack)
{
	check(!HasAnyFlags(RF_ClassDefaultObject));
	AActor* Owner = GetOwningActor();
	UNetDriver* NetDriver = Owner->GetNetDriver();
	if (NetDriver)
	{
		NetDriver->ProcessRemoteFunction(Owner, Function, Parms, OutParms, Stack, this);
		return true;
	}
	return false;
}


void UReplicatedObject::Destroy()
{
	if (::IsValid(this))
	{
		checkf(GetOwningActor()->HasAuthority() == true, TEXT("Destroy:: Object does not have authority to destroy itself!"));

		OnDestroyed();
		MarkAsGarbage();
	}
}


/*
The GatherChanges template function prepares data for OnChange(insert, update, delete) delegate callback.
*/
template<typename T>
const T* _FindItem(const T& InItem, const TArray<T>& InItemList)
{
	for (const T& Item : InItemList)
	{
		if (IsEqualId(Item, InItem))
		{
			return &Item;
		}
	}
	return nullptr;
}

template<typename T>
void _GatherChanges(const TArray<T>& InNew, const TArray<T>& InOld, TArray<T>& OutInserted, TArray<T>& OutUpdatedNew, TArray<T>& OutUpdatedOld, TArray<T>& OutDeleted)
{
	// Figure out the change and forward as an Insert, Update or a Delete event
	for (const T& Item : InNew)
	{
		if (const T* OldItem = _FindItem<T>(Item, InOld))
		{
			// This mission is is neither new or has been removed so we check if it has been updated
			if (!IsEqual(Item, *OldItem))
			{
				OutUpdatedNew.Add(Item);
				OutUpdatedOld.Add(*OldItem);
			}
		}
		else
		{
			// This is a new mission
			OutInserted.Add(Item);
		}
	}

	// Find deleted missions
	for (const T& OldItem : InOld)
	{
		if (_FindItem<T>(OldItem, InNew) == nullptr)
		{
			OutDeleted.Add(OldItem);
		}
	}
}


template <typename TOwner, typename DelegateType, typename T>
void BroadcastChanges(TOwner* Owner, DelegateType& Delegate, TArray<T> NewList, TArray<T> OldList)
{
	// Figure out the change and forward as an Insert, Update and a Delete event
	TArray<T> Inserted, UpdatedNew, UpdatedOld, Deleted;
	_GatherChanges(NewList, OldList, Inserted, UpdatedNew, UpdatedOld, Deleted);

	UE_LOG(LogATMission, Display, TEXT("%s: BroadcastChanges: New=%i, Modified=%i, Deleted=%i"), *ATMISSION_FUNC_LINE, Inserted.Num(), UpdatedNew.Num(), Deleted.Num());
	Delegate.Broadcast(Owner, Inserted, UpdatedNew, UpdatedOld, Deleted);
}



//	------------- BELOW IS MISSION SPECIFICS, ABOVE IS VIEWMODEL GENERICS AND UOBJECT REPL STUFF
TMap<UClass*, UClass*> UMissionVM::ModelToViewmodel;


UMissionVM::UMissionVM()
{
	ModelToViewmodel.Add(UMission::StaticClass()) = GetClass();
}


UClass* UMissionVM::GetVMClass(UMission* Mission)
{
	return ModelToViewmodel.Contains(Mission->GetClass()) ? ModelToViewmodel[Mission->GetClass()] : UMissionVM::StaticClass();
}


void UMissionVM::Initialize(UMission* TheMission)	
{
	Mission = TheMission;
	Mission->OnMissionStateChanged.AddUObject(this, &UMissionVM::OnMissionChanged);
	Reflect();
}


void UMissionVM::OnRep_MissionInfo(const FMissionInfo& OldValue)
{
	UE_LOG(LogATMission, Display, TEXT("%s: OnMissionInfoChanged:\n\tOld: %s\n\tNew: %s"), *ATMISSION_FUNC_LINE, *OldValue.ToStr(), *mission_info.ToStr());
	OnMissionInfoChanged.Broadcast(this, mission_info, OldValue);
}


void UMissionVM::OnRep_Objectives(const TArray<FObjectiveVM>& OldValue)
{
	BroadcastChanges(this, OnObjectivesChanged, Objectives, OldValue);
}


void UMissionVM::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UMissionVM, ComponentOwnerPrivate, COND_InitialOnly);
	DOREPLIFETIME(UMissionVM, mission_info);
	DOREPLIFETIME(UMissionVM, Objectives);
}


void UMissionVM::OnMissionChanged(UMission*)
{
	Reflect();
}


void UMissionVM::Reflect()
{
	mission_info.mission_id = Mission->mission_id;
	mission_info.caption = Mission->caption;
	mission_info.is_linear = Mission->is_linear;
	mission_info.is_active = Mission->GetHeadObjective()->is_active;
	mission_info.is_completed = Mission->GetHeadObjective()->is_completed;
	mission_info.failed = Mission->GetHeadObjective()->failed;

	Objectives.Reset();
	for (auto Objective : Mission->objectives)
	{
		FObjectiveVM ObjectiveVM;
		ObjectiveVM.MissionVM = this;
		ObjectiveVM.objective_id = Objective->objective_id;
		ObjectiveVM.is_active = Objective->is_active;
		ObjectiveVM.is_completed = Objective->is_completed;
		ObjectiveVM.failed = Objective->failed;

		for (auto Plugin : Objective->plugins)
		{
			UMissionPluginVM* MissionPluginVM = NewObject<UMissionPluginVM>(this, Plugin->GetMissionPluginVMClass());
			ObjectiveVM.plugins.Add(MissionPluginVM);
		}

		Objectives.Add(ObjectiveVM);
	}

}


TMap<UClass*, UClass*> UMissionPluginVM::ModelToViewmodel;


UMissionPluginVM::UMissionPluginVM()
{
	ModelToViewmodel.Add(UMission::StaticClass()) = GetClass();
}


UClass* UMissionPluginVM::GetVMClass(UMissionPlugin* Plugin)
{
	return ModelToViewmodel.Contains(Plugin->GetClass()) ? ModelToViewmodel[Plugin->GetClass()] : UMissionPluginVM::StaticClass();
}


void UMissionPluginVM::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// DOREPLIFETIME(UMissionPluginVM, xxx);
}



UMissionsContainerVM::UMissionsContainerVM(const FObjectInitializer& OI)
	: Super(OI)
{
	// Component must be replicated to replicate sub-objects
	SetIsReplicatedByDefault(true);

	// Get ticks (yes bad way of polling the state of our sister compononent)
	PrimaryComponentTick.bCanEverTick = true;
}


void UMissionsContainerVM::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UMissionsContainerVM, MissionInfoList);
	DOREPLIFETIME(UMissionsContainerVM, Missions);
	DOREPLIFETIME(UMissionsContainerVM, Notifications);
}


void UMissionsContainerVM::OnRep_MissionInfoList(const TArray<FMissionInfo>& OldValue)
{
	BroadcastChanges(this, OnMissionInfoListChanged, MissionInfoList, OldValue);
}


void UMissionsContainerVM::OnRep_Missions(const TArray<UMissionVM*>& OldValue)
{
	BroadcastChanges(this, OnMissionsListChanged, Missions, OldValue);
}


void UMissionsContainerVM::OnRep_Notifications(const TArray<FNotification>& OldValue)
{
	int32 Latest = OldValue.Num();
	for (int i = Latest; i < Notifications.Num(); i++)
	{
		UE_LOG(LogATMission, Display, TEXT("%s: Notification: %s"), *ATMISSION_FUNC_LINE, *Notifications[i].ToStr());
	}
	OnNotification.Broadcast(this, Notifications, Latest);
}


bool UMissionsContainerVM::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	// Specify all members here below that should be replicated
	bWroteSomething |= Channel->ReplicateSubobjectList(Missions, *Bunch, *RepFlags);

	return bWroteSomething;
}


void UMissionsContainerVM::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority())
	{
		MissionsContainer = Cast<UMissionsContainer>(GetOwner()->GetComponentByClass(UMissionsContainer::StaticClass()));
	}
}


void UMissionsContainerVM::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner()->HasAuthority())
	{
		// Should not even be here
		return;
	}

	// Update our Mission info list
	MissionInfoList.Reset();
	Notifications.Reset();
	if (MissionsContainer)
	{
		for (UMission* Mission : MissionsContainer->missions)
		{
			// Update the mission info list
			FMissionInfo Info;
			Info.mission_id = Mission->mission_id;
			Info.caption = Mission->caption;
			Info.is_linear = Mission->is_linear;
			Info.is_active = Mission->GetHeadObjective()->is_active;
			Info.is_completed = Mission->GetHeadObjective()->is_completed;
			Info.failed = Mission->GetHeadObjective()->failed;
			MissionInfoList.Add(Info);
		}

		for (int Index = 0; Index < MissionsContainer->Notifications.Num(); Index++)
		{
			Notifications.Add(MissionsContainer->Notifications[Index]);
		}
	}
}

void UMissionsContainerVM::OpenViewer_Implementation(const FString& MissionId)
{
	if (!MissionsContainer)
	{
		UE_LOG(LogATMission, Display, TEXT("%s: 'MissionsContainer' not available."), *ATMISSION_FUNC_LINE);
		return;
	}

	// Find the mission
	UMission* Mission = nullptr;
	for (UMission* M : MissionsContainer->missions)
	{		
		if (M->mission_id == MissionId)
		{
			// Make sure it's not already open
			for (UMissionVM* MissionVM : Missions)
			{
				if (MissionVM->mission_info.mission_id == MissionId)
				{
					// Already open
					UE_LOG(LogATMission, Display, TEXT("%s: Mission already open: %s"), *ATMISSION_FUNC_LINE, *MissionId);
					return;
				}
			}
			Mission = M;
			break;
		}
	}

	if (!Mission)
	{
		UE_LOG(LogATMission, Display, TEXT("%s: Mission not found: %s"), *ATMISSION_FUNC_LINE, *MissionId);
		return;
	}
	
	UMissionVM* MissionVM = NewObject<UMissionVM>(this, UMissionVM::GetVMClass(Mission));
	MissionVM->Initialize(Mission);
	Missions.Add(MissionVM);
}


void UMissionsContainerVM::CloseViewer_Implementation(const FString& MissionId)
{
	for (UMissionVM* MissionVM : Missions)
	{
		if (MissionVM->mission_info.mission_id == MissionId)
		{
			Missions.Remove(MissionVM);
			return;
		}
	}

	UE_LOG(LogATMission, Display, TEXT("%s: Mission not open or doesn't exist: %s"), *ATMISSION_FUNC_LINE, *MissionId);
}


void UMissionsContainerVM::ResetMissions_Implementation ()
{
	UE_LOG(LogATMission, Display, TEXT("%s: Resetting all %i missions."), *ATMISSION_FUNC_LINE, MissionsContainer->missions.Num());
	MissionsContainer->missions.Reset();
}


void UMissionsContainerVM::ResetMissionsCmd()
{
	ResetMissions();
}