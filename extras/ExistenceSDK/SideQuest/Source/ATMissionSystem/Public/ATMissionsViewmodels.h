// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ATMissions.h"

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"

#include "ATMissionsViewmodels.generated.h"



/*
Base class for replicated UObjects
*/
UCLASS()
class ATMISSIONSYSTEM_API UReplicatedObject : public UObject
{
	GENERATED_BODY()
public:

	// Allows the Object to get a valid UWorld from it's outer.
	virtual UWorld* GetWorld() const override;

	UFUNCTION(BlueprintPure, Category = "My Object")
	AActor* GetOwningActor() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool IsSupportedForNetworking() const override;

	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;

	// Call "Remote" (aka, RPC) functions through the actors NetDriver
	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms, FFrame* Stack) override;

	/*
	* Optional
	* Since this is a replicated object, typically only the Server should create and destroy these
	* Provide a custom destroy function to ensure these conditions are met.
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "My Object")
	void Destroy();

protected:
	
	virtual void OnDestroyed() {};
};



//	------------- BELOW IS MISSION SPECIFICS, ABOVE IS VIEWMODEL GENERICS AND UOBJECT REPL STUFF
class UMissionsContainerVM;


USTRUCT(Blueprintable)
struct ATMISSIONSYSTEM_API FMissionInfo
{
	GENERATED_BODY()

		UPROPERTY(BlueprintReadWrite)
		FString mission_id;

	UPROPERTY(BlueprintReadWrite)
		FText caption;

	UPROPERTY(BlueprintReadWrite)
		bool is_linear;

	UPROPERTY(BlueprintReadWrite)
		bool is_active = false;

	UPROPERTY(BlueprintReadWrite)
		bool is_completed = false;

	UPROPERTY(BlueprintReadWrite)
		bool failed = false;

	FString ToStr() const
	{
		return FString::Printf(TEXT("[%s] %s: l=%d, a=%d, c=%d, f=%d"), *mission_id, *caption.ToString(), is_linear, is_active, is_completed, failed);
	}

	friend bool IsEqualId(const FMissionInfo& a, const FMissionInfo& b)
	{
		return a.mission_id == b.mission_id;
	}

	friend bool IsEqual(const FMissionInfo& a, const FMissionInfo& b)
	{
		return a.mission_id == b.mission_id &&
			a.caption.IdenticalTo(b.caption) &&
			a.is_linear == b.is_linear &&
			a.is_active == b.is_active &&
			a.is_completed == b.is_completed &&
			a.failed == b.failed;
	}

};


USTRUCT(Blueprintable)
struct ATMISSIONSYSTEM_API FObjectiveVM
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString objective_id;

	UPROPERTY(BlueprintReadWrite)
	bool is_active = false;

	UPROPERTY(BlueprintReadWrite)
	bool is_completed = false;

	UPROPERTY(BlueprintReadWrite)
	bool failed = false;

	UPROPERTY(BlueprintReadWrite)
	TArray<UMissionPluginVM*> plugins;

	UPROPERTY(BlueprintReadOnly, transient)
	UMissionVM* MissionVM = nullptr;

	FString ToStr() const
	{
		return FString::Printf(TEXT("[%s] a=%d, c=%d, f=%d"), *objective_id, is_active, is_completed, failed);
	}

	friend bool IsEqualId(const FObjectiveVM& a, const FObjectiveVM& b)
	{
		return a.objective_id == b.objective_id;
	}

	friend bool IsEqual(const FObjectiveVM& a, const FObjectiveVM& b)
	{
		return
			a.objective_id == b.objective_id &&
			a.is_active == b.is_active &&
			a.is_completed == b.is_completed &&
			a.plugins == b.plugins;

	}
};


UCLASS(Blueprintable)
class UMissionVM : public UReplicatedObject
{
	GENERATED_BODY()


	/**
	Called when Viewmodel state is changed.
	The 'Hint' may refer to the name of the property that changed.
	*/
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMissionInfoChanged, UMissionVM*, Owner, const FMissionInfo&, NewValue, const FMissionInfo&, OldValue);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnObjectivesChanged, UMissionVM*, Owner, const TArray<FObjectiveVM>&, Inserted, const TArray<FObjectiveVM>&, UpdatedNew, const TArray<FObjectiveVM>&, UpdatedOld, const TArray<FObjectiveVM>&, Deleted);


public:

	UMissionVM();

	void Initialize(UMission* TheMission);

	UPROPERTY(Replicated, BlueprintReadWrite, ReplicatedUsing = OnRep_MissionInfo)
	FMissionInfo mission_info;

	UPROPERTY(BlueprintAssignable, Category = "ViewModel")
	FOnMissionInfoChanged OnMissionInfoChanged;

	UFUNCTION()
	void OnRep_MissionInfo(const FMissionInfo& OldValue);

	UPROPERTY(Replicated, BlueprintReadWrite, ReplicatedUsing = OnRep_Objectives)
	TArray<FObjectiveVM> Objectives;

	UPROPERTY(BlueprintAssignable, Category = "ViewModel")
	FOnObjectivesChanged OnObjectivesChanged;

	UFUNCTION()
	void OnRep_Objectives(const TArray<FObjectiveVM>& OldValue);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void OnMissionChanged(UMission* Mission);

	UFUNCTION(BlueprintCallable)
	static UClass* GetVMClass(UMission* Mission);

	// Model to ViewModel class map
	static TMap<UClass*, UClass*> ModelToViewmodel;

	friend bool IsEqualId(const UMissionVM* a, const UMissionVM* b)
	{
		return a->mission_info.mission_id == b->mission_info.mission_id;
	}

	friend bool IsEqual(const UMissionVM* a, const UMissionVM* b)
	{
		return IsEqual(a->mission_info, b->mission_info);
	}


private:
	friend class UMissionsContainerVM;
	
	UPROPERTY(Replicated)
	UMissionsContainerVM* ComponentOwnerPrivate;

	UPROPERTY(Transient)
	UMission* Mission;

	void Reflect();

};



UCLASS(Blueprintable)
class UMissionPluginVM : public UReplicatedObject
{
	GENERATED_BODY()


public:

	UMissionPluginVM();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable)
	static UClass* GetVMClass(UMissionPlugin* Mission);

	// Model to ViewModel class map
	static TMap<UClass*, UClass*> ModelToViewmodel;

};



UCLASS(Blueprintable)
class UMissionPluginTriggerVM : public UReplicatedObject
{
	GENERATED_BODY()


public:

};






UCLASS(meta = (BlueprintSpawnableComponent))
class ATMISSIONSYSTEM_API UMissionsContainerVM : public UActorComponent
{
	GENERATED_BODY()

	/** 
	Called when Viewmodel state is changed. 
	The 'Hint' may refer to the name of the property that changed.
	*/
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnMissionInfoListChanged, UMissionsContainerVM*, Owner, const TArray<FMissionInfo>&, Inserted, const TArray<FMissionInfo>&, UpdatedNew, const TArray<FMissionInfo>&, UpdatedOld, const TArray<FMissionInfo>&, Deleted);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnMissionsListChanged, UMissionsContainerVM*, Owner, const TArray<UMissionVM*>&, Inserted, const TArray<UMissionVM*>&, UpdatedNew, const TArray<UMissionVM*>&, UpdatedOld, const TArray<UMissionVM*>&, Deleted);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnNotification, UMissionsContainerVM*, Owner, const TArray<FNotification>&, Notifications, int32, Latest);

public:

	UPROPERTY(BlueprintAssignable, Category = "ViewModel")
	FOnMissionInfoListChanged OnMissionInfoListChanged;


	UPROPERTY(BlueprintAssignable, Category = "ViewModel")
	FOnMissionsListChanged OnMissionsListChanged;

	UPROPERTY(BlueprintAssignable, Category = "ViewModel")
	FOnNotification OnNotification;

	UMissionsContainerVM(const FObjectInitializer& OI);

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	UFUNCTION(Reliable, Server, BlueprintCallable)
	void OpenViewer(const FString& MissionId);


	UFUNCTION(Reliable, Server, BlueprintCallable)
	void CloseViewer(const FString& MissionId);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	void OnRep_MissionInfoList(const TArray<FMissionInfo>& OldValue);

	UFUNCTION()
	void OnRep_Missions(const TArray<UMissionVM*>& OldValue);

	UFUNCTION()
	void OnRep_Notifications(const TArray<FNotification>& OldValue);


	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags);

	UPROPERTY(Replicated, BlueprintReadWrite, ReplicatedUsing = OnRep_MissionInfoList)
	TArray<FMissionInfo>  MissionInfoList;


	UPROPERTY(Replicated, BlueprintReadWrite, ReplicatedUsing = OnRep_Missions)
	TArray<UMissionVM*>  Missions;

	UPROPERTY(Replicated, BlueprintReadWrite, ReplicatedUsing = OnRep_Notifications)
	TArray<FNotification> Notifications;
	
	// Reset all missions in this container (debug)
	UFUNCTION(Reliable, Server, BlueprintCallable)
	void ResetMissions();

	UFUNCTION(Exec, meta = (OverrideNativeName = "missions.reset"))
	void ResetMissionsCmd();


private:

	UPROPERTY()
	UMissionsContainer* MissionsContainer;

};

