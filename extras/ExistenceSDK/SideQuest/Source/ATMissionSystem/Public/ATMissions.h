// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Templates/SharedPointer.h"

// Temp hack for component reflection
#include "IKamoComponentReflection.h"

#if WITH_EDITOR
#include "Factories/Factory.h"
#include "AssetTypeActions_Base.h"
#endif // WITH_EDITOR

#include "ATMissions.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogATMission, Log, All);

// Borrowed from VaRest
#define ATMISSION_FUNC (FString(__func__))              // Current Class Name + Function Name where this is called
#define ATMISSION_LINE (FString::FromInt(__LINE__))         // Current Line Number in the code where this is called
#define ATMISSION_FUNC_LINE (ATMISSION_FUNC + "(" + ATMISSION_LINE + ")") // Current Class and Line Number where this is called!


UINTERFACE(MinimalAPI)
class UMissionEventsInterface : public UInterface
{
	GENERATED_BODY()

};


class IMissionEventsInterface : public IInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent)
	void OnMissionModified(UMission* Mission);
};







UCLASS(BlueprintType)
class ATMISSIONSYSTEM_API UObjectiveState : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString objective_id;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool is_active=false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool is_completed = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool failed = false;

	UPROPERTY(BlueprintReadOnly, instanced)
	TArray<UMissionPlugin*> plugins;

	UPROPERTY(BlueprintReadOnly, transient)
	UMission* Mission = nullptr;

	UFUNCTION(BlueprintCallable, Category = "ATMission")
	void AddPlugin(UMissionPlugin* Plugin);
};



USTRUCT(BlueprintType)
struct ATMISSIONSYSTEM_API FObjective
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		FString objective_id;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		bool is_active = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		bool is_completed = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		bool failed = false;

	UPROPERTY(BlueprintReadOnly, instanced)
		TArray<UMissionPlugin*> plugins;

	UPROPERTY(BlueprintReadOnly, transient)
		UMission* Mission = nullptr;

	//UFUNCTION(BlueprintCallable, Category = "ATMission")
		//void AddPlugin(UMissionPlugin* Plugin);
};



UCLASS(BlueprintType)
class ATMISSIONSYSTEM_API UMissionPlugin : public UObject
{
	GENERATED_BODY()

public:

	UMissionPlugin();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ATMission")
	void OnActive(UObject* obj);

	UFUNCTION(BlueprintCallable)
	void FinishObjective(bool Failed);
	
	// This is ... Idempotent
	UFUNCTION(BlueprintImplementableEvent)
	void OnObjectiveActivated();

	UFUNCTION(BlueprintImplementableEvent)
	void OnObjectiveCompleted();

	UFUNCTION(BlueprintImplementableEvent)
	void OnMissionFinished();

	UFUNCTION(BlueprintCallable)
	UMissionsContainer* GetMissionsContainer();

	UPROPERTY(BlueprintReadOnly, Transient)
	UObjectiveState* Objective;


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	UClass* GetMissionPluginVMClass();
};


UCLASS(Blueprintable)
class ATMISSIONSYSTEM_API UMissionPluginTrigger : public UMissionPlugin
{
	GENERATED_BODY()

public:

	UMissionPluginTrigger();
};



UCLASS(Blueprintable)
class ATMISSIONSYSTEM_API UMission : public UObject
{
	GENERATED_BODY()

public:

	/** delegate type for mission change events ( Params: UMission* Mission ) */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnMissionStateChanged, UMission*);

	UMission();
		

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString mission_id;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText caption;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool is_linear;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> participants;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString availability;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, instanced)
	TArray<UObjectiveState*> objectives;

	UFUNCTION(BlueprintCallable)
	FString SaveToJson();

	/*
	Initializes mission for usage. Sets up all plugins and objectives.
	*/
	UFUNCTION(BlueprintCallable)
	void PostInitialize();

	void FinishObjective(UObjectiveState* Objective, bool Failed);

	bool IsInitialized = false;
	
	UPROPERTY(BlueprintReadOnly, Transient)
	UMissionsContainer* ParentContainer;


	FOnMissionStateChanged OnMissionStateChanged;

	UFUNCTION()
	void SetDirty();

	UFUNCTION(BlueprintCallable)
	UObjectiveState* GetHeadObjective();

	UFUNCTION(BlueprintCallable)
	UObjectiveState* AddObjective(const FString& ObjectiveId);


};


// HACK: This struct is just to wrap the missions array for Json serialization 
// due to incorrectly implemented serialization in KamoState and other tidbits.
USTRUCT(Blueprintable)
struct ATMISSIONSYSTEM_API FMissionsList
{
	GENERATED_BODY()

		UPROPERTY(BlueprintReadOnly, instanced)
		TArray<UMission*> missions;
};



USTRUCT(Blueprintable)
struct ATMISSIONSYSTEM_API FNotification
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString Id;
		
	UPROPERTY(BlueprintReadWrite)
	FString MessageType;

	UPROPERTY(BlueprintReadWrite)
	FString IdType;

	UPROPERTY(BlueprintReadWrite)
	FString AnalyticsEvent;

	FString ToStr() const
	{
		return FString::Printf(TEXT("[%s] Message Type: %s, IDType: %s, Analytics Event: %s"), *Id, *MessageType, *IdType, *AnalyticsEvent);
	}

	friend bool IsEqualId(const FNotification& a, const FNotification& b)
	{
		return a.Id == b.Id;
	}

	friend bool IsEqual(const FNotification& a, const FNotification& b)
	{
		return a.Id == b.Id &&
			a.MessageType == b.MessageType &&
			a.IdType == b.IdType &&
			a.AnalyticsEvent == b.AnalyticsEvent;
	}
};



/*
A placeholder for serializing missions using the Json serializer.
The Json serializer doesn't support sublass type at the root so all mission objects need to 
be placed under this mission container class.
*/
UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class ATMISSIONSYSTEM_API UMissionsContainer : public UActorComponent, public IKamoComponentReflection
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Transient)
	TScriptInterface<IMissionEventsInterface> MissionEventsInterface;


	/*
	Initializes all missions for usage by setting up plugins and objectives.
	This function should be called 
	*/
	UFUNCTION(BlueprintCallable)
	void PostInitialize();

	UPROPERTY(BlueprintReadWrite, Instanced)
	TArray<UMission*> missions;

	UPROPERTY(BlueprintReadWrite)
	FMissionsList MissionsList;


	UPROPERTY(BlueprintReadWrite, Transient)
	TArray<FNotification> Notifications;
	

	UFUNCTION(BlueprintCallable, Category = "KamoObject")
	static UMissionsContainer* CreateFromJsonText(const FString& JsonText);

	UFUNCTION(BlueprintCallable, Category = "KamoObject")
	FString SaveAsJsonText() const;

	/*
	Adds a mission to the mission container. The mission will be initialized.
	This is more streamlined than adding a mission into the missions array and calling PostInitialize.
	*/
	UFUNCTION(BlueprintCallable)
	void AddMission(UMission* Mission);

	void OnMissionModified(UMission* Mission);

	// Kamo reflections - TEMP HACK
	virtual void UpdateKamoStateFromActor_Implementation(UKamoState* state) override;
	virtual void ApplyKamoStateToActor_Implementation(UKamoState* state) override;

protected:

	void PostInitializeMission(UMission* Mission);

};


UCLASS(Blueprintable)
class ATMISSIONSYSTEM_API UMissionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UMissionSubsystem();

	// Begin USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// End USubsystem

	static UMissionSubsystem* Get(UObject* WorldContextObject);

};

