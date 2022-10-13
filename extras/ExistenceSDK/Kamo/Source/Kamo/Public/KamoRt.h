// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "Engine/DataTable.h"
#include "Tickable.h"

#include "KamoStructs.h"
#include "KamoState.h"
#include "KamoObject.h"
#include "KamoDB.h"
#include "KamoMQ.h"
#include "KamoRuntimeInterface.h"

#include "KamoRt.generated.h"


DECLARE_EVENT_OneParam(FKamoModule, FKamoRuntimeEvent, UKamoRuntime*)



struct FIdentityActor
{
	KamoID Id;
	TKamoClassMapEntry KamoClassInfo;
	bool bIsAttached = false;
	float Timeout = 4.0;
	float TimeElapsedWaiting = 0.0;
	TWeakObjectPtr<AActor> Actor;
	TWeakObjectPtr<UKamoActor> KamoActor;
	TWeakObjectPtr<UKamoRuntime> KamoRuntime;

	void Tick(float DeltaTime, bool& bOutRemoveEntry);
	void AttachIdentityActor(bool& bOutRemoveEntry);
	void CreateKamoObject();
};


/**
 * 
 */
UCLASS()
class KAMO_API UKamoRuntime : 
	public UTickableWorldSubsystem
{
	GENERATED_BODY()

   
	bool runtime_is_spawning;
    
public:

	UKamoRuntime();

	//~ Begin UObject Interface
	virtual void BeginDestroy() override;
	virtual bool IsReadyForFinishDestroy() override;
	virtual void FinishDestroy() override;
	//~ End UObject Interface

	//~ Begin UTickableWorldSubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	//~ End UTickableWorldSubsystem Interface

	void HandleWorldBeginPlay() const;
	FDelegateHandle WorldBeginPlayHandle;


	/**
	 * FTickableGameObject overrides
	 */
	void Tick(float DeltaTime) override;
	bool IsTickable() const override;
	TStatId GetStatId() const override;

	void ShutdownRuntime();

    // Initialization for different backends.
	bool Init();

	static FKamoRuntimeEvent& OnKamoRuntime() { return runtime_event; }


    // Runtime initialization

	/** Load in a region from DB, registers a handler for it and load all child objects */
    bool LoadAndPossessRegion(const KamoID& root_id);

	// Runtime processing
	void TickRuntime();
    
    // Object querying
    TArray<UKamoRootObject*> FindRootObjects(const FString& class_name) const;
    TArray<UKamoChildObject*> FindObjects(const KamoID& root_id, const FString& class_name) const;
    
    // Object manipulation
	EKamoLoadChildObject LoadChildObjectFromDB(const KamoID& id, KamoID* root_id = nullptr);
    bool OverwriteObjectState(const KamoID& id, const FString& state);
	UKamoObject* CreateObject(const FString& class_name, const KamoID& root_id, UObject* object=nullptr, const FString& unique_id="");
	bool MoveObject(const KamoID& id, const KamoID& root_id, const FString& spawn_target);
	bool MoveObjectSafely(const KamoID& id, const KamoID& root_id, const FString& spawn_target);
	UKamoObject* GetObject(const KamoID& id, bool fail_silently=false) const;
	UKamoChildObject* GetObjectFromActor(AActor* actor) const;

    
	/*
	Delete the kamo object 'id' from the region and embed it into 'container_id' using the
	tag 'category'.
	Note, category names prefixed by underscore are reserved for internal use.
	*/	bool EmbedObject(const KamoID& id, const KamoID& container_id, const FString& category);
	
	/*
	Extract the object 'id' from 'container_id' and spawn it into the world as Kamo object.
	*/
	bool ExtractObject(const KamoID& id, const KamoID& container_id);

	// Proxy objects - Returns a proxy to a Kamo object which can be used for querying state and sending
	// messages.
	UKamoObject* GetProxyObject(const KamoID& id);
    
    // DB synch
    void MarkForUpdate();
    void SerializeObjects(bool commit_to_db=false);  // If 'commit_to_db' then the calling thread blocks until all is flushed to DB.

	// Stuff
	void FlushStateToActors();

	// Loading and unloading regions
	TSet<UKamoChildObject*> GetObjectsInRegion(const KamoID& region_id);
	bool UnloadRegion(const KamoID& region_id);

    // Handler registry
    bool RegisterUE4Handler(UE4ServerHandler& ue4handler);  // TODO: Refactor this into proper class hierarchy
	bool UnregisterUE4Server(const KamoID& handler_id);
    bool SetHandler(const KamoID& root_id, const KamoID& handler_id);
    UUE4ServerHandler* GetUE4Server(const KamoID& handler_id);
	UUE4ServerHandler* GetUE4ServerForRoot(const KamoID& id);
	UUE4ServerHandler* GetUE4ServerForChild(const KamoID& id);
	
	TArray<KamoID> GetServerRegions() const;
	
	UE4ServerHandler handler;
	float last_refresh_seconds;
	float power_save_seconds;

    
    // Message queue
	void ProcessMessageQueue();
	bool SendMessage(const KamoID& handler_id, const FString& message_type, const FString& payload);
    void ProcessMessage(const KamoMessage& message);
    
    // World
    UKamoObject* RegisterKamoObject(const KamoID& id, const KamoID& root_id, UKamoState* state, UObject* object=nullptr, bool is_proxy=false, bool skip_apply_state=false);
	bool SpawnActorForKamoObject(UKamoActor* kamo_object, FTransform* transform=nullptr, bool finalize=true);
	void FinalizeSpawnActorForKamoObject(UKamoActor* kamo_object, FTransform* transform);
    bool RefreshKamoObject(const KamoID& id, UKamoState* state);
	void OnActorSpawned(AActor* actor);
	FOnActorSpawned::FDelegate actor_spawned_delegate;

	// Utilities
	static FString GetCommandLineArgument(const FString& argument, const FString& default_value = FString());
	TArray<KamoID> GetUE4ServerRegions() const;
	static UKamoID* GetKamoIDFromString(const FString& kamo_id);	
	static UKamoState* CreateMessageCommand(const KamoID& kamo_id, const KamoID& root_id, const FString& command, const FString& parameters);
	static UKamoState* CreateCommandMessagePayload(TArray<UKamoState*> commands);
	bool SendCommandToObject(const KamoID& id, const FString& command, const FString& parameters);
	bool SendCommandsToObject(const KamoID& id, TArray<UKamoState*> commands);

	FString FormatCurrentRegionName(const FString& volume_name=FString()) const;
	FString FormatRegionName(const FString& map_name, const FString& volume_name, const FString& instance_id) const;
	void InitBeaconHost();
	void InitializeKamoLevelActors();

	static UDataTable* GetKamoTable();
	TKamoClassMapEntry GetDefaultKamoClassMapEntry() const;
	TKamoClassMapEntry GetKamoClassEntryFromActor(AActor* actor) const;
	FKamoClassMap GetKamoClass(const FString& class_name) const;

	FKamoClassMap& GetClassMap(const FString& class_name) const;
	FKamoClassMap& GetClassMap(AActor* actor) const;
	//TSubclassOf<class AActor> actor_class
	//TSubclassOf<class UKamoObject> kamo_class
	//FKamoClassMap& GetClassMap(const FString& class_name) const;
	void GatherStats(const TSharedPtr < FJsonObject >& stats) const;


	UPROPERTY()
	class AOnlineBeaconHost* BeaconHost;

	UPROPERTY()
	class AKamoBeaconHostObject* KamoBeaconHostObject;

	// Variables
	UPROPERTY()
	TMap<FString, UKamoObject*> internal_state;

	UPROPERTY()
	UDataTable* kamo_table;

	TMap<FString, FKamoClassMap> classmap;


	// Identity Actors
public:

	bool AddIdentityActor(const FString& InUniqueId, AActor* InActor);

	void MoveIdentityObjectIfPIE(const KamoID& InObjectId, const KamoID& InRootId);
	bool bPIEMoveDone = false; // We must only do one move per PIE session

private:

	void TickIdentityActors(float DeltaTime);
	TMap<KamoID, FIdentityActor> IdentityActors;


public:

	// Moar stuff

	TUniquePtr<IKamoDB> database;
	TUniquePtr<IKamoMQ> message_queue;

	static FKamoRuntimeEvent runtime_event;

	// Registered regions
	struct RegionVolume
	{
		FString volume_name;
		FVector origin;
		FVector box_extent;
	};

	TArray<FString> region_volume_whitelist;
	TArray<RegionVolume> region_volumes;
	TArray<KamoID> registered_regions;

	FString region_instance_id; // Kamo Level Instance ID

	FString MapActorToRegion(AActor* actor) const;
	FString MapLocationToRegion(const FVector& Location) const;



	bool is_initialized;
	KamoID server_id;
	bool poll_message_queue;


	// Track how long since certain elements were processed
	float mark_and_sync_elapsed;
	float message_queue_flush_elapsed;
};
