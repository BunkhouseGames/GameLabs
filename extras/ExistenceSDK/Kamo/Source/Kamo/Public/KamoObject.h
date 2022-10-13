// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DataTable.h"
#include "KamoState.h"
#include "KamoStructs.h"

#include "KamoObject.generated.h"


class UKamoRuntime;


/**
 *
 */
UCLASS(Blueprintable, BlueprintType)
class KAMO_API UKamoID : public UObject
{
    GENERATED_BODY()
    
public:

	static UKamoID* New(const KamoID& kamo_id)
	{
		auto ob = NewObject<UKamoID>();
		ob->Init(kamo_id);
		return ob;
	}

    void Init(const KamoID& kamo_id) {
        class_name = kamo_id.class_name;
        unique_id = kamo_id.unique_id;
    }
    
    KamoID GetPrimitive() {
        KamoID id;
        
        id.class_name = class_name;
        id.unique_id = unique_id;
        
        return id;
    }
    
    UFUNCTION(BlueprintPure, Category = "KamoID")
    FString GetID() { return *FString::Printf(TEXT("%s.%s"), *class_name, *unique_id); }
    
    UFUNCTION(BlueprintPure, Category = "KamoID")
    bool IsEmpty() { return unique_id == ""; }
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoID")
    FString class_name;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoID")
    FString unique_id;
};



USTRUCT(BlueprintType)
struct FKamoClassMap : public FTableRowBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "KamoClassMap", meta = (AllowAbstract = true))
    TSubclassOf<class AActor> actor_class;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "KamoClassMap")
    TSubclassOf<class UKamoActor> kamo_class;

    /** Always check if dirty */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "KamoClassMap")
    bool check_if_dirty;

};

typedef TMap<FString, UKamoObject*> TMapInternalState;
typedef TKeyValuePair<FString, FKamoClassMap*> TKamoClassMapEntry;


USTRUCT(BlueprintType)
struct FKamoSubobject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly)
    UKamoID* Id = nullptr;

    UPROPERTY(BlueprintReadOnly)
    TWeakObjectPtr<AActor> Actor;





    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "KamoClassMap")
        TSubclassOf<class UKamoObject> kamo_class;

    /** Always check if dirty */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "KamoClassMap")
        bool check_if_dirty;

};


UENUM(BlueprintType)
enum class EKamoStateStage : uint8
{
	KSS_ActorPass UMETA(DisplayName = "Set state to actor (components may not be initialized)"),
	KSS_ComponentPass UMETA(DisplayName = "Set state to components")
};


UCLASS(Blueprintable, BlueprintType)
class KAMO_API UKamoObject : public UObject
{
	GENERATED_BODY()

protected:

	UKamoObject();


    //~ Begin UObject Interface
    virtual void PostCDOContruct() override;
    //~ End UObject Interface

    static UKamoRuntime* GetKamoRuntime(UWorld* InWorld);
    UKamoRuntime* GetKamoRuntime() const;

    static TArray<UClass*> KamoClasses;

    UFUNCTION(BlueprintPure, Category = "KamoObject")
        static TArray<UClass*> GetClassMap() {
        return KamoClasses;
    }


	UPROPERTY(BlueprintReadOnly, Category = "Kamobject")
	UObject* object;

	FBoolProperty* KamoDirtyProp;
    
public:

    UPROPERTY(BlueprintReadOnly)
    FKamoClassMap kamo_class_entry;


	UObject* GetObject() const { return object; }
	void SetObject(UObject* InObject);

    // Resolve Kamo object references and spawn actors if applicable.
    // This function can be called multiple times with out side effects.
    void ResolveSubobjects(UKamoRuntime* runtime);

    void Init(const KamoObject& obj) {
        id->Init(obj.id);
        state->SetState(obj.state);
    }

    virtual bool IsReadyForFinishDestroy() override;

    void MarkForUpdate();

    KamoObject GetPrimitive() {
        KamoObject obj;
        
        obj.id = id->GetPrimitive();
        obj.state = state->GetStateAsString();
        
        return obj;
    }	

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoObject")
    UKamoID* id;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoObject")
    UKamoState* state;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoObject")
    bool dirty;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoObject")
	bool isNew;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoObject")
    bool deleted;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoObject")
    bool actor_deleted;

    UPROPERTY(BlueprintReadOnly, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoObject")
	bool is_proxy;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoObject")
	bool apply_state_now;

    UPROPERTY(BlueprintReadOnly, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoObject")
    bool check_if_dirty;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KamoObject")
    void UpdateKamoStateFromActor();
    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KamoObject")
    void ApplyKamoStateToActor(EKamoStateStage stage_stage);
	
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KamoObject", meta=(ScriptName="ShouldCheckIfDirty"))
    bool CheckIfDirty();
	
	virtual bool PreCheckIfDirty();
    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KamoObject")
    void HandleInboxCommand(class UKamoCommand* command);
    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KamoObject")
    UKamoState* GetKamoState();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KamoObject")
	bool SendCommand(const FString& command, UKamoState* args);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KamoObject")
	bool OnCommandReceived(const FString& command, UKamoState* args);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KamoObject")
	bool OnRegistered();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KamoObject")
	void OnObjectSet(UObject* obj, UKamoRuntime* runtime);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KamoObject")
	void OnObjectUnSet(UObject* obj);

    UFUNCTION(BlueprintCallable, Category = "KamoObject")
    AGameModeBase* GetGamemodeFromWorld();

    UFUNCTION(BlueprintPure, Category = "KamoObject")
    bool SubobjectKeyExists(const FString& object_key) const;

    UFUNCTION(BlueprintPure, Category = "KamoObject")
    TMap<FString, UKamoObject*> GetSubobjects() const;

    UFUNCTION(BlueprintCallable, Category = "KamoObject")
    void SetSubobject(const FString& object_key, UKamoObject* kamo_object);


    UPROPERTY()
    TMap<FString, FKamoSubobject> Subobjects;

    UPROPERTY()
    TMap<FString, UKamoObject*> kamo_subobjects;

    TMap<FString, KamoID> unresolved_subobjects;

    UFUNCTION(BlueprintCallable, Category = "KamoObject")
    AActor* SpawnSubobject(const FString& object_key, bool use_spawn_target=true);

};


UCLASS(Blueprintable, BlueprintType)
class KAMO_API UKamoRootObject : public UKamoObject
{
    GENERATED_BODY()

    
public:

	UKamoRootObject();

    void Init(const KamoRootObject& obj) {
        UKamoObject::Init(obj);
        handler_id->Init(obj.handler_id);
    }
    
    KamoRootObject GetPrimitive() {
        KamoRootObject obj;
        
        auto kamo_obj = UKamoObject::GetPrimitive();
        
        obj.id = kamo_obj.id;
        obj.state = kamo_obj.state;
        
        obj.handler_id = handler_id->GetPrimitive();
        
        return obj;
    }
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoRootObject")
    UKamoID* handler_id;
};


UCLASS(Blueprintable, BlueprintType)
class KAMO_API UKamoChildObject : public UKamoObject
{
    GENERATED_BODY()
protected:

	UKamoChildObject();

public:
    void Init(const KamoChildObject& obj) 
	{
        UKamoObject::Init(obj);                
        root_id->Init(obj.root_id);
    }
    
    KamoChildObject GetPrimitive() {
        KamoChildObject obj;
        
        auto kamo_obj = UKamoObject::GetPrimitive();
        
        obj.id = kamo_obj.id;
        obj.state = kamo_obj.state;
        
        obj.root_id = root_id->GetPrimitive();
        
        return obj;
    }
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoChildObject")
    UKamoID* root_id;
};

UCLASS(Blueprintable, BlueprintType)
class KAMO_API UKamoHandlerObject : public UKamoObject
{
    GENERATED_BODY()
    
public:
    void Init(const KamoHandlerObject& obj) {
        UKamoObject::Init(obj);
        inbox_address = obj.inbox_address;
    }
    
    KamoHandlerObject GetPrimitive() {
        KamoHandlerObject obj;
        
        auto kamo_obj = UKamoObject::GetPrimitive();
        
        obj.id = kamo_obj.id;
        obj.state = kamo_obj.state;
        
        obj.inbox_address = inbox_address;
        
        return obj;
    }
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoHandlerObject")
    FString inbox_address;
};

UCLASS(Blueprintable, BlueprintType)
class KAMO_API UKamoMessage : public UObject
{
    GENERATED_BODY()
    
public:

	UKamoMessage();

    void Init(const KamoMessage& message) {
        sender->Init(message.sender);
        payload->SetState(message.payload);
        message_type = message.message_type;
    }
    
    KamoMessage GetPrimitive() {
        KamoMessage message;
        
        message.sender = sender->GetPrimitive();
        message.message_type = message_type;
        message.payload = payload->GetStateAsString();
        
        return message;
    }
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoMessage")
    UKamoID* sender;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoMessage")
    FString message_type;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoMessage")
    UKamoState* payload;
};

UCLASS(Blueprintable, BlueprintType)
class KAMO_API UUE4ServerHandler : public UKamoHandlerObject
{
    GENERATED_BODY()
    
public:
    void Init(const UE4ServerHandler& obj) 
	{
        UKamoHandlerObject::Init(obj);
		ip_address = obj.ip_address;
		port = obj.port;
		map_name = obj.map_name;
    }
    
	UE4ServerHandler GetPrimitive()
	{
		UE4ServerHandler obj;
		auto super = Super::GetPrimitive();

		obj.id = super.id;
		obj.inbox_address = super.inbox_address;

		obj.ip_address = ip_address;
		obj.port = port;
		obj.map_name = map_name;

		return obj;
    }
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoUE4ServerHandler")
    FString ip_address;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoUE4ServerHandler")
    int32 port;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoUE4ServerHandler")
    FString map_name;

    UFUNCTION()
    FString GetFormattedServerAddress()
    {
        // Utility function to get a formatted server address
        FString address;
        if (ip_address.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("No IP Address found for handler, assuming local host"));
            address = "127.0.0.1";
        }
        else
        {
            address = ip_address;
        }
        return address + ":" + FString::FromInt(port);
    }

    UPROPERTY(BlueprintReadOnly, Category = "KamoUE4ServerHandler")
    FDateTime StartTime;

    UPROPERTY(BlueprintReadOnly, Category = "KamoUE4ServerHandler")
    FDateTime LastRefresh;

    UPROPERTY(BlueprintReadOnly, Category = "KamoUE4ServerHandler")
    int32 seconds_from_last_heartbeat;
};


UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class EObjectRefMode : uint8
{
    RM_Undefined UMETA(DisplayName = "(undefined)"),
    RM_SpawnObject UMETA(DisplayName = "Spawn actor"),
	RM_AttachByTag UMETA(DisplayName = "Attach to level actor by tag"),
	RM_AttachManually UMETA(DisplayName = "Attach manually")
};

USTRUCT(BlueprintType)
struct FEmbededObject
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "KamoEmbededObject")
	FString category;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "KamoEmbededObject")
	FString kamo_id;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "KamoEmbededObject")
	FString json_state;
};




UCLASS(Blueprintable, BlueprintType)
class KAMO_API UKamoActor : public UKamoChildObject
{
	GENERATED_BODY()

protected:

	UKamoActor();
	void GetSaveGameProperties(UObject* Object, TArray<FString>& Properties);

public:
    void OnActorSet();

    static void ProcessOnActorSpawned(AActor* Actor, UKamoRuntime* KamoRuntime);
    
    // CDO only callback functions to allow for per class customization
    virtual void OnActorSpawnedCDO(AActor* Actor, UKamoRuntime* KamoRuntime, const TKamoClassMapEntry& ClassEntry);
    virtual void OnKamoObjectNotFoundCDO(struct FIdentityActor& InIdentityActor);

    // IdentityActor callbacks
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KamoActor")
    void OnKamoInstanceAttached();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KamoActor")
    void OnKamoInstanceDetached();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KamoActor")
    void OnKamoInstanceError(const FString& ErrorText);

    AActor* GetActor() const
	{
		return Cast<AActor>(object);
	}

    template <class T>
    T* GetActorOfClass() const
    {
        return Cast<T>(object);
    }

    UFUNCTION(Category = "KamoActor")
    void OnActorDestroyed(AActor* destroyed_actor);

    virtual void OnObjectSet_Implementation(UObject* obj, UKamoRuntime* runtime);
    virtual void ApplyKamoStateToActor_Implementation(EKamoStateStage stage_stage) override;
	virtual void UpdateKamoStateFromActor_Implementation() override;
	virtual bool PreCheckIfDirty() override;
    virtual void OnMove(const KamoID& target_region_id);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KamoActor")
	UKamoID* MapSpawnedActorToRegion(AActor* new_actor, UKamoRuntime* current_runtime);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KamoActor")
	void PreProcessState();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Enum)
	EObjectRefMode object_ref_mode;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoActor")
	TMap<FString, FEmbededObject> embedded_objects;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ExposeOnSpawn = "true"), Category = "KamoActor")
	TArray<FString> PersistedProperties;

	bool actor_is_set; // Is true if actor had been assigned at some point.

};



DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(FString, FGetUniqueIdDelegate, AGameModeBase*, GameMode, APlayerController*, NewPlayer);


UCLASS(Blueprintable, BlueprintType)
class KAMO_API UKamoPlayerController : public UKamoActor
{
    GENERATED_BODY()

protected:

    UKamoPlayerController();

public:

    // CDO only callback functions to allow for per class customization
    virtual void OnActorSpawnedCDO(AActor* Actor, UKamoRuntime* KamoRuntime, const TKamoClassMapEntry& ClassEntry) override;
    virtual void OnKamoObjectNotFoundCDO(struct FIdentityActor& InIdentityActor) override;

    static FDelegateHandle PostLoginEventBinding;
    static void OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer);

    virtual void OnKamoInstanceDetached_Implementation() override;

    virtual void OnMove(const KamoID& target_region_id) override;
};


UINTERFACE(Blueprintable)
class KAMO_API UKamoObjectInterface : public UInterface
{
	GENERATED_BODY()
};

class KAMO_API IKamoObjectInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "KamoObject")
	void UpdateKamoStateFromObject(UKamoState* KamoState);

	UFUNCTION(BlueprintNativeEvent, Category = "KamoObject")
	void ApplyKamoStateToObject(EKamoStateStage Stage, UKamoState* KamoState);
};
