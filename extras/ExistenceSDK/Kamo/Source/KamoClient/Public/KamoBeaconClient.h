// Copyright Directive Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ObjectMacros.h"
#include "OnlineBeaconClient.h"

#include "Subsystems/LocalPlayerSubsystem.h"
#include "GameFramework/OnlineReplStructs.h"

#include "KamoBeaconClient.generated.h"

/*
Kamo Core API
*/


USTRUCT(BlueprintType)
struct FKamoHandlerInfo
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString HandlerId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString IpAddress = "";

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Port = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString MapName = "";

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 SecondsFromLastHeartbeat = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FDateTime StartTime = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FDateTime LastRefresh = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> HandledRegions;

	FText ToRichText() const;

	bool operator==(const FKamoHandlerInfo& Other) const
	{
		return
			HandlerId == Other.HandlerId &&
			IpAddress == Other.IpAddress &&
			Port == Other.Port &&
			MapName == Other.MapName &&
			abs(SecondsFromLastHeartbeat - Other.SecondsFromLastHeartbeat) < 7 && // If heartbeast diff is less than 7 seconds we rule it as equal
			StartTime == Other.StartTime &&
			LastRefresh == Other.LastRefresh &&
			HandledRegions == Other.HandledRegions;
	}
};


USTRUCT(BlueprintType)
struct FKamoRegionInfo
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString RegionId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString HandlerId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bShouldBeOnline = false;

	bool operator==(const FKamoRegionInfo& Other) const
	{
		return
			RegionId == Other.RegionId &&
			HandlerId == Other.HandlerId &&
			bShouldBeOnline == Other.bShouldBeOnline;
	}
};


UCLASS(transient, notplaceable)
class KAMOCLIENT_API AKamoWorldState : public AInfo
{
	GENERATED_UCLASS_BODY()

public:

	//~ Begin AActor Interface
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;
	//~ End AActor Interface


	UPROPERTY(BlueprintReadwrite)
	TArray<FKamoHandlerInfo> Handlers;

	UPROPERTY(BlueprintReadwrite)
	TArray<FKamoRegionInfo> Regions;

	UFUNCTION(NetMulticast, reliable)
	virtual void MulticastUpdateHandlers(const TArray<FKamoHandlerInfo>& InHandlers);

	UFUNCTION(NetMulticast, reliable)
	virtual void MulticastUpdateRegions(const TArray<FKamoRegionInfo>& InRegions);

	bool operator==(const AKamoWorldState& Other) const
	{
		return Handlers == Other.Handlers && Regions == Other.Regions;
	}

protected:


};



USTRUCT(BlueprintType)
struct FPlayerLocation
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString RegionId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FKamoHandlerInfo Handler;

	bool operator==(const FPlayerLocation& Other) const
	{
		return RegionId == Other.RegionId && Handler == Other.Handler;
	}
};



/*
Kamo Auxiliary Tools
*/

USTRUCT(BlueprintType)
struct FKamoPlayerInfo
{
	GENERATED_USTRUCT_BODY()

public:

	/* Player Identity is the unique ID of the connected player or user. This is NOT the KamoID of the player.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString PlayerIdentity;

	/*Player ID as a KamoID. Is empty if the player has not been created yet. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString PlayerId;

	/* Location of player or default location if no player record exists */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FPlayerLocation Location;

	/* Location of players homeworld. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FPlayerLocation Homeworld;

	/* True if player info is fetched successfully. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsValid = false;


	FString ToString() const
	{
		return FString::Printf(TEXT("%s: Location=%s Homeworld=%s"), *PlayerId, *Location.RegionId, *Homeworld.RegionId);
	}

	bool operator==(const FKamoPlayerInfo& Other) const
	{
		return
			PlayerIdentity == Other.PlayerIdentity &&
			PlayerId == Other.PlayerId &&
			Location == Other.Location &&
			Homeworld == Other.Homeworld &&
			bIsValid == Other.bIsValid;

	}

	bool operator!=(const FKamoPlayerInfo& Other) const
	{
		return !operator==(Other);
	}
};




UINTERFACE()
class UKamoBeaconServerInterface : public UInterface
{
	GENERATED_BODY()
};


class IKamoBeaconServerInterface
{
	GENERATED_BODY()

public:
	
	virtual bool LookupPlayer(const FString& InPlayerIdentity, FKamoPlayerInfo& OutPlayerInfo) const = 0;
	virtual bool RefreshWorldState(AKamoWorldState* OutWorldState) = 0;
	virtual void LaunchServerForPlayer(const FKamoPlayerInfo& PlayerInfo) = 0;
	virtual void MovePlayerToRegion(const FString& InPlayerID, const FString& InRegionID, const FString& SpawnTarget) = 0;
};



UCLASS(transient, notplaceable, config=Engine)
class KAMOCLIENT_API AKamoBeaconClient : public AOnlineBeaconClient
{
	GENERATED_UCLASS_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClientConnected, AKamoBeaconClient*, InKamoBeaconClient);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeaconConnected, AKamoBeaconClient*, InKamoBeaconClient);

public:

	AKamoBeaconClient();

	virtual void BeginDestroy() override;

	//~ Begin AOnlineBeaconClient Interface
	virtual void OnFailure() override;
	virtual void DestroyBeacon() override;
	virtual void OnConnected() override;
	// Note: There is no point in overriding SetNetConnection as the 'NetConnection' is not initialized at the time of the callback.
	//       See OnlineBeaconHost.cpp(173) or search for "NewClientActor->SetNetConnection"
	//virtual void SetNetConnection(UNetConnection* NetConnection) override;
	//~ End AOnlineBeaconClient Interface


	//~ Begin Replicated State
	UFUNCTION()
	void OnRep_KamoWorldState();

	UPROPERTY(BlueprintReadonly, Replicated, ReplicatedUsing = OnRep_KamoWorldState)
	AKamoWorldState* KamoWorldState;

	UPROPERTY(BlueprintReadonly)
	FKamoPlayerInfo PlayerInfo;  // Note: This property is replicated to the client manually using ClientSetPlayerInfo function.
	//~ EndReplicated State


	static FString PendingDestinationAddress;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = KamoClient, meta = (Tooltip = "Automatically travel to players location server."))
	bool bAutoTravel = false;


	/*
	Server RPC functions
	*/

	/** Set Player Identity. Todo: Integrate with online identity service.*/
	UFUNCTION(Server, Reliable, BlueprintCallable)
	virtual void ServerLoginPlayer(const FString& InPIEPlayerId);

	/** Launch server for me. If possible make sure there's a server for me. */
	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	virtual void ServerLaunchServerForMe();	

	/** Move me to a new region */
	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	virtual void ServerMovePlayerToRegion(const FString& InRegionID);
	
	
	/*
	Server side functions
	*/
	void RefreshAndPushPlayerInfo();


	/*
	Client RPC functions
	*/
	UFUNCTION(client, reliable)
	virtual void ClientSetPlayerInfo(const FKamoPlayerInfo& InPlayerInfo);


	/*
	Client side callback events
	*/

	UPROPERTY()
	UKamoClientBeaconSubsystem* Subsystem;


	/*
	Client tick
	*/
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void TravelToPlayerLocation();

	// Auto-travel flags
	static FString LastAddress; // Must be a process singleton (or put in a Game or Engine subsystem)
	static FString DeadServerAddress; // Used to identity a potentially dead server and only log out a warning once.
	bool  bShowNoHandlerWarning = true; // Log out "no handler" warning once per beacon session.

	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnClientConnected OnClientConnected;

	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnBeaconConnected OnBeaconConnected;

};


UCLASS(Blueprintable, defaultconfig)
class KAMOCLIENT_API UKamoClientBeaconSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

	// Begin USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;

	// End USubsystem


	void CreateClientBeacon();

	UPROPERTY(BlueprintReadOnly)
	AKamoBeaconClient* Beacon;

	bool bQuitTrying = false; // Quit trying to create beacon


	FDelegateHandle WorldLoadEventBinding;
	void OnPostLoadMapWithWorld(UWorld* world);

	FDelegateHandle PostLoginEventBinding;
	void OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer);


};

UCLASS()
class KAMOCLIENT_API UWidgetBaseProjectInfo : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION()
	void SetViewmodel(UViewmodelBaseProjectInfo* viewmodel);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ProjectInfo)
	UViewmodelBaseProjectInfo* ProjectInfoViewmodel;

};

UCLASS()
class KAMOCLIENT_API UViewmodelBaseProjectInfo : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	FString GetProjectVersion();

	UFUNCTION(BlueprintCallable)
	FString GetPlayerRegion();

	UFUNCTION(BlueprintCallable)
	FString GetBeaconStatus();

	UFUNCTION(BlueprintCallable)
	FString GetPlayerID();

	UFUNCTION(BlueprintCallable)
	FString GetSecondsSinceLastHeartbeat();

	UFUNCTION(BlueprintCallable)
	FString GetPlayerServerAddress();

	UFUNCTION(BlueprintCallable)
	FString GetBeaconServerAddress();

	UFUNCTION(BlueprintCallable)
	FString GetShouldAutoTravel();

	UFUNCTION(BlueprintCallable)
	void ToggleAutoTravel();

	UFUNCTION(BlueprintCallable)
	void MovePlayerToRegion(const FString& InRegionID);

	UFUNCTION(BlueprintCallable)
	TArray<FKamoRegionInfo> GetRegions();

	UFUNCTION(BlueprintCallable)
	TMap<FString, FString> GetProjectInfo();


};

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Kamo Client"))
class KAMOCLIENT_API UKamoClientSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UKamoClientSettings();

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = KamoClient, meta = (Tooltip = "Kamo Beacon URL. Key is tenant, value is URL. Use the form 'tcp://hostname:port/'."))
	TMap<FString, FString> KamoBeaconURL;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = KamoClient, meta = (Tooltip = "Automatically travel to players location server."))
	bool bAutoTravel = true;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = KamoClient, meta = (Tooltip = "Auto-travel only if servers last heartbeat was within the threshold."))
	int32 AutoTravelServerHeartbeatThreshold = 30;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = KamoClient, meta = (Tooltip = "Class name of Kamo Player  used for lookup."))
	FString KamoPlayerClassName = "player";

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = KamoClient)
	TSubclassOf<class UWidgetBaseProjectInfo> ProjectInfoWidget;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = KamoClient, meta = (Tooltip = "World state poll interval - TEMPORARY SOLUTION."))
	float WorldStatePollInterval = 6.5;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = KamoClient, meta = (Tooltip = "Player Info poll interval - TEMPORARY SOLUTION."))
	float PlayerInfoPollInterval = 0.42;


#if WITH_EDITORONLY_DATA
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = KamoClient, meta = (Tooltip = "When in Editor, use localhost as Beacon server."))
	bool bEditorUsesLocalhostBeacon = true;
#endif

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = KamoClient)
	TSubclassOf<AKamoBeaconClient> KamoBeaconClass;

	virtual void PostInitProperties() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};


