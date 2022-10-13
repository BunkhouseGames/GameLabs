// Copyright Directive Games, Inc. All Rights Reserved.

#include "KamoBeaconClient.h"


#include "Net/UnrealNetwork.h"
#include "Net/OnlineEngineInterface.h"

#include "Engine/NetConnection.h"
#include "Engine/LocalPlayer.h"

#include "OnlineBeaconHostObject.h"
#include "Kismet/GameplayStatics.h"

#include "GameFramework/PlayerState.h"


#if WITH_EDITOR
#include "OnlineBeaconHost.h"
#endif // WITH_EDITOR



// TODO: Move this into a top level Kamo module. It's currently duplicated in KamoUtil::get_tenant_name()
namespace
{
    FString GetCommandLineSwitch(const FString& switch_name)
    {
        TArray<FString> tokens;
        TArray<FString> switches;
        FCommandLine::Parse(FCommandLine::Get(), tokens, switches);

        for (auto arg : switches)
        {
            FString left;
            FString right;

            if (arg.Split("=", &left, &right) && left == switch_name)
            {
                return right;
            }
        }

        return "";
    }

    FString get_tenant_name()
    {
        // Read tenant name from commandline
        FString tenant = GetCommandLineSwitch("kamotenant");

        if (tenant.IsEmpty())
        {
            tenant = GetCommandLineSwitch("tenant");
            if (!tenant.IsEmpty())
            {
                UE_LOG(LogBeacon, Warning, TEXT("Using 'tenant' command line argument is deprecated. Please use '-kamotenant=%s'"), *tenant);
            }
        }

        if (tenant != "")
        {
            UE_LOG(LogBeacon, Log, TEXT("tenant name from commandline: %s"), *tenant);
            return tenant;
        }

        /*
        // Read tenant name from project settings
        const auto Settings = UKamoProjectSettings::Get();
        tenant = Settings->kamo_tenant;

        if (Settings->kamo_tenant != "")
        {
            UE_LOG(LogBeacon, Log, TEXT("reading tenant from project settings: %s"), *tenant);
            return Settings->kamo_tenant;
        }
        */

        // If we the tenant name has not been passed in we use the project name
        tenant = FString(FApp::GetProjectName()).ToLower();
        //UE_LOG(LogBeacon, Log, TEXT("default tenant name from project: %s"), *tenant);

        return tenant;
    }
}


FText FKamoHandlerInfo::ToRichText() const
{
	TArray<FString> msg;
	msg.Add(FString::Printf(TEXT("Handler Info:")));
	msg.Add(FString::Printf(TEXT("Handler ID: <RichText.Emphasis> %s </>"), *HandlerId));
	msg.Add(FString::Printf(TEXT("IP Address: <RichText.Emphasis> %s:%i </>"), *IpAddress, Port));
	msg.Add(FString::Printf(TEXT("Map Name: <RichText.Emphasis> %s </>"), *MapName));
	msg.Add(FString::Printf(TEXT("Seconds From Last Heartbeat: <RichText.Emphasis> %i sec </>"), SecondsFromLastHeartbeat));
	msg.Add(FString::Printf(TEXT("Start Time: <RichText.Emphasis> %s </>"), *StartTime.ToString()));
	msg.Add(FString::Printf(TEXT("Last Refresh: <RichText.Emphasis> %s </>"), *LastRefresh.ToString()));
	FString Result = FString::Join(msg, TEXT("\n"));
	
	return FText::FromString(Result);
}

AKamoWorldState::AKamoWorldState(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	bReplicates = true;
	bAlwaysRelevant = true;
	NetDriverName = NAME_BeaconNetDriver;
}

/* uncomment once we have a replicated property
void AKamoWorldState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(AKamoWorldState, ComplicatedNumber);	
}
*/


bool AKamoWorldState::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	UClass* BeaconClass = RealViewer->GetClass();
	return (BeaconClass && BeaconClass->IsChildOf(AKamoBeaconClient::StaticClass()));
}


void AKamoWorldState::MulticastUpdateHandlers_Implementation(const TArray<FKamoHandlerInfo>& InHandlers)
{
	Handlers = InHandlers;
}


void AKamoWorldState::MulticastUpdateRegions_Implementation(const TArray<FKamoRegionInfo>& InRegions)
{
	Regions = InRegions;
}


AKamoBeaconClient::AKamoBeaconClient(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	bOnlyRelevantToOwner = true;
    bAutoTravel = GetDefault<UKamoClientSettings>()->bAutoTravel;

}


AKamoBeaconClient::AKamoBeaconClient()
{
	UE_LOG(LogBeacon, Verbose, TEXT("Kamo beacon instantiated."));
}

void AKamoBeaconClient::BeginDestroy()
{
    Super::BeginDestroy();
}


void AKamoBeaconClient::OnFailure()
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogBeacon, Verbose, TEXT("Kamo beacon connection failure, handling connection timeout."));
#endif
	Super::OnFailure();
}


void AKamoBeaconClient::DestroyBeacon()
{
    Super::DestroyBeacon();
}


void AKamoBeaconClient::OnConnected()
{
    // Note: This is a CLIENT callback.
    Super::OnConnected();

    // If player is logged in using an online subsystem we will call LoginPlayer passing in the
    // player id. This function should normally be overridden in application.
    UWorld* World = GetWorld();
    FString PIEPlayerId;
    
    // Note! The following code is mirrored in KamoClientModule.cpp and KamoBeaconClient.cpp
#if !UE_BUILD_SHIPPING
    // If we are in PIE we pass in a PIE specific player id.
    if (World->WorldType == EWorldType::PIE)
    {
        int32 PIEInstanceID = World->GetPackage()->GetPIEInstanceID();
        if (PIEInstanceID < 0)
        {
            // Honestly don't know what this means
            UE_LOG(LogBeacon, Warning, TEXT("Not logging onto server as my PIE Instance ID is %i"), PIEInstanceID);
            return;
        }
        PIEPlayerId = FString::Printf(TEXT("pieplayer_%i"), PIEInstanceID);
    }
#endif

    ServerLoginPlayer(PIEPlayerId);
    OnBeaconConnected.Broadcast(this);
}


void AKamoBeaconClient::OnRep_KamoWorldState()
{

}


void AKamoBeaconClient::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKamoBeaconClient, KamoWorldState);
}


void AKamoBeaconClient::ServerLoginPlayer_Implementation(const FString& InPIEPlayerId)
{
    if (!BeaconConnection->PlayerId.IsValid())
    {
        UE_LOG(LogBeacon, Warning, TEXT("Login from client beacon has invalid player id."));
        return;
    }

    FString PlayerIdentity = BeaconConnection->PlayerId->ToString();

#if !UE_BUILD_SHIPPING
    if (!InPIEPlayerId.IsEmpty())
    {
        // Override the player identity with the PIE player identity (see OnBeaconConnected_Implementation function above).
        PlayerIdentity = InPIEPlayerId;
    }
#endif
    
    PlayerInfo.PlayerIdentity = PlayerIdentity;
    PlayerInfo.bIsValid = false;
    RefreshAndPushPlayerInfo();
    
    UE_LOG(LogBeacon, Display, TEXT("   New beacon player: %s %s"), *PlayerIdentity, *BeaconConnection->LowLevelDescribe());
}


void AKamoBeaconClient::RefreshAndPushPlayerInfo()
{
    IKamoBeaconServerInterface* BeaconServer = Cast<IKamoBeaconServerInterface>(GetBeaconOwner());
    if (!PlayerInfo.PlayerIdentity.IsEmpty() && BeaconServer)
    {
        FKamoPlayerInfo NewPlayerInfo;
        NewPlayerInfo.bIsValid = BeaconServer->LookupPlayer(PlayerInfo.PlayerIdentity, NewPlayerInfo);
        if (PlayerInfo != NewPlayerInfo)
        {
            PlayerInfo = NewPlayerInfo;
            // The 'PlayerInfo' property is not replicated automatically so we explicitly set it on the client side here:
            ClientSetPlayerInfo(NewPlayerInfo);
        }
    }
}


void AKamoBeaconClient::ServerLaunchServerForMe_Implementation()
{
    if (IKamoBeaconServerInterface* BeaconServer = Cast<IKamoBeaconServerInterface>(GetBeaconOwner()))
    {
        BeaconServer->LaunchServerForPlayer(PlayerInfo);
    }
}

bool AKamoBeaconClient::ServerLaunchServerForMe_Validate()
{
    return PlayerInfo.bIsValid; 
}


void AKamoBeaconClient::ServerMovePlayerToRegion_Implementation(const FString& InRegionID)
{
    if (IKamoBeaconServerInterface* BeaconServer = Cast<IKamoBeaconServerInterface>(GetBeaconOwner()))
    {   
        BeaconServer->MovePlayerToRegion(PlayerInfo.PlayerId, InRegionID, TEXT("playerstart"));
    }
}


bool AKamoBeaconClient::ServerMovePlayerToRegion_Validate(const FString& InRegionID) 
{
    return PlayerInfo.bIsValid; 
}



void AKamoBeaconClient::ClientSetPlayerInfo_Implementation(const FKamoPlayerInfo& InPlayerInfo)
{
#if !UE_BUILD_SHIPPING
    // Only log out significant change. We null out the SecondsFromLastHeartbeat and log out if the new player info is still different
    PlayerInfo.Location.Handler.SecondsFromLastHeartbeat = InPlayerInfo.Location.Handler.SecondsFromLastHeartbeat;
    PlayerInfo.Homeworld.Handler.SecondsFromLastHeartbeat = InPlayerInfo.Homeworld.Handler.SecondsFromLastHeartbeat;
	UE_CLOG(PlayerInfo != InPlayerInfo, LogBeacon, Display, TEXT("Kamo beacon ClientSetPlayerInfo: %s"), *InPlayerInfo.ToString());
#endif // UE_BUILD_SHIPPING

    PlayerInfo = InPlayerInfo;
    OnClientConnected.Broadcast(this);
}


void AKamoBeaconClient::Tick(float DeltaTime)
{
    if (bAutoTravel)
    {
        TravelToPlayerLocation();
    }
}


void AKamoBeaconClient::TravelToPlayerLocation()
{
    if (PlayerInfo.Location.Handler.IpAddress.IsEmpty())
    {
        UE_CLOG(bShowNoHandlerWarning, LogBeacon, Warning, TEXT("Player can't travel to %s because there is no handler (server)."), *PlayerInfo.Location.RegionId);
        bShowNoHandlerWarning = false;
    }
    else
    {
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {            
            FString PlayerServerAddress = FString::Printf(TEXT("%s:%i"), *PlayerInfo.Location.Handler.IpAddress, PlayerInfo.Location.Handler.Port);            
            const FURL& URL = GetWorld()->URL;
            FString CurrentAddress = FString::Printf(TEXT("%s:%i"), *URL.Host, URL.Port);

            if (PlayerServerAddress != AKamoBeaconClient::LastAddress && PlayerServerAddress != CurrentAddress)
            {
                int32 ServerHeartbeat = PlayerInfo.Location.Handler.SecondsFromLastHeartbeat;
                int32 HeartbeatThreshold = GetDefault<UKamoClientSettings>()->AutoTravelServerHeartbeatThreshold;

                if (ServerHeartbeat > HeartbeatThreshold)
                {
                    UE_CLOG(AKamoBeaconClient::DeadServerAddress != PlayerServerAddress, LogBeacon, Warning, 
                        TEXT("Auto-travel to %s ignored as servers last heartbeast was %i seconds ago."), *PlayerServerAddress, ServerHeartbeat);
                    AKamoBeaconClient::DeadServerAddress = PlayerServerAddress;
                }
                else
                {
                    AKamoBeaconClient::LastAddress = PlayerServerAddress;
                    UE_LOG(LogBeacon, Display, TEXT("Auto-travel to player's region: %s at %s"), *PlayerInfo.Location.RegionId, *PlayerServerAddress);                    
                    PC->ClientTravel(PlayerServerAddress, TRAVEL_Absolute);
                }
            }
        }
        else
        {
            UE_LOG(LogBeacon, Display, TEXT("Player can't auto-travel to %s. No player controller available."), *PlayerInfo.Location.RegionId);
        }
    }

}

FString AKamoBeaconClient::LastAddress;
FString AKamoBeaconClient::DeadServerAddress;


namespace FromKamoUtil
{
    static FString GetCommandLineSwitch(const FString& SwitchName)
    {
        TArray<FString> tokens;
        TArray<FString> switches;
        FCommandLine::Parse(FCommandLine::Get(), tokens, switches);

        for (auto arg : switches)
        {
            FString left;
            FString right;

            if (arg.Split("=", &left, &right) && left == SwitchName)
            {
                return right;
            }
        }

        return "";
    }
}


void UKamoClientBeaconSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Note: Cannot call 'CreateClientBeacon()' here as actors cannot be spawned into the world at this point.
    // Instead the beacon is created on the first Tick.

    // The current world URL (or server address) is not available to blueprints so it is published here
    // for convenience:
    const FURL& URL = GetWorld()->URL;

}


void UKamoClientBeaconSubsystem::CreateClientBeacon()
{
    FString KamoBeaconURL = FromKamoUtil::GetCommandLineSwitch("kamourl");
    if (!KamoBeaconURL.IsEmpty())
    {
        UE_LOG(LogBeacon, Display, TEXT("Kamo Beacon URL from command line: %s"), *KamoBeaconURL);
    }
#if WITH_EDITOR
    else if (GEngine->IsEditor() && GetDefault<UKamoClientSettings>()->bEditorUsesLocalhostBeacon)
    {
        // Default to localhost using the currently configured beacon port in the beacon host class
        KamoBeaconURL = FString::Printf(TEXT("tcp://127.0.0.1:%i"), GetDefault<AOnlineBeaconHost>()->ListenPort);
        UE_LOG(LogBeacon, Display, TEXT("Kamo Beacon URL defaulting to localhost: %s"), *KamoBeaconURL);
    }
#endif // WITH_EDITOR
    else
    {
        // This is (most likely) a shipping build, get server url from client settings:
        const FString* URLForTenant = GetDefault<UKamoClientSettings>()->KamoBeaconURL.Find(get_tenant_name());
        if (!URLForTenant)
        {
            UE_LOG(LogBeacon, Warning, TEXT("Can't initialize Kamo Beacon, No URL specified for tenant %s in settings."), *get_tenant_name());
        }
        else
        {
            KamoBeaconURL = *URLForTenant;
        }
        
        if (KamoBeaconURL.IsEmpty())
        {
            UE_LOG(LogBeacon, Warning, TEXT("Can't initialize Kamo Beacon, URL not on command line or in settings."));
        }
        else
        {
            UE_LOG(LogBeacon, Display, TEXT("Kamo Beacon URL from project settings: %s"), *KamoBeaconURL);
        }
    }

    if (KamoBeaconURL.IsEmpty())
    {
        bQuitTrying = true;
        return;
    }

    UWorld* World = GetWorld();
    FURL URL(nullptr, *KamoBeaconURL, TRAVEL_Absolute);

    if (GetDefault<UKamoClientSettings>()->KamoBeaconClass != nullptr)
    {
        Beacon = World->SpawnActor<AKamoBeaconClient>(GetDefault<UKamoClientSettings>()->KamoBeaconClass);
    }
    else
    {
        Beacon = World->SpawnActor<AKamoBeaconClient>(AKamoBeaconClient::StaticClass());
    }
    
    Beacon->Subsystem = this;

    if (Beacon)
    {
        UE_LOG(LogBeacon, Display, TEXT("Initialize Kamo Beacon on URL: %s"), *URL.ToString());
        if (!Cast<AKamoBeaconClient>(Beacon)->InitClient(URL))
        {
            UE_LOG(LogBeacon, Error, TEXT(
                "The Kamo Online Beacon failed to initialize for URL %s. Check the logs above for more information.\n"
                "Note that the BeaconNetDriver must be enabled when using Online Beacons.\n"
                "If there is a LogNet message above saying CreateNamedNetDriver failed for BeaconNetDriver then add the following lines to DefaultEngine.ini:\n"
                "[/Script/Engine.Engine]\n"
                "+NetDriverDefinitions=(DefName=\"BeaconNetDriver\",DriverClassName=\"OnlineSubsystemUtils.IpNetDriver\")"
            ), *URL.ToString());
        }
        // Leave the Beacon instance as is. Its connection state will be "Invalid = 0, Connection is invalid, possibly uninitialized."
    }


    UViewmodelBaseProjectInfo* vm = NewObject<UViewmodelBaseProjectInfo>(GetWorld());

    if (GetDefault<UKamoClientSettings>()->ProjectInfoWidget)
    {
        UWidgetBaseProjectInfo* infoWidget = CreateWidget<UWidgetBaseProjectInfo>(GetWorld(), GetDefault<UKamoClientSettings>()->ProjectInfoWidget);

        infoWidget->SetViewmodel(vm);
        infoWidget->AddToViewport();
    }
}


void UKamoClientBeaconSubsystem::Deinitialize()
{
    Super::Deinitialize();

    if (Beacon)
    {
        Beacon->DestroyBeacon();
        Beacon = nullptr;
    }

    AKamoBeaconClient::LastAddress = "";
}


bool UKamoClientBeaconSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    if (IsRunningDedicatedServer())
    {
        return false;
    }

    UWorld* World = Cast<UWorld>(Outer);
    check(World);

    // Note that the world context is only found for the client world
    for (const FWorldContext& Context : GEngine->GetWorldContexts())
    {
        if (Context.WorldType == EWorldType::Game)
        {
            return !Context.RunAsDedicated;
        }
        else if ((Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Editor) && Context.PIEPrefix == World->StreamingLevelsPrefix)
        {
            return !Context.RunAsDedicated;
        }
    }

    return false;

}


TStatId UKamoClientBeaconSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UKamoRuntime, STATGROUP_Tickables);
}


void UKamoClientBeaconSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UWorld* World = GetWorld();
    if (World && Beacon == nullptr && !bQuitTrying)
    {
        CreateClientBeacon();
    }

    if (Beacon)
    {
        Beacon->Tick(DeltaTime);
    }
}


FString UViewmodelBaseProjectInfo::GetProjectVersion()
{
    FString AppVersion;

    GConfig->GetString(
        TEXT("/Script/EngineSettings.GeneralProjectSettings"),
        TEXT("ProjectVersion"),
        AppVersion,
        GGameIni
    );

    return AppVersion;
}

void UViewmodelBaseProjectInfo::ToggleAutoTravel()
{

    UKamoClientBeaconSubsystem* KamoBeaconSubsystem = GetWorld()->GetSubsystem<UKamoClientBeaconSubsystem>();

    KamoBeaconSubsystem->Beacon->bAutoTravel = !KamoBeaconSubsystem->Beacon->bAutoTravel;

}

void UViewmodelBaseProjectInfo::MovePlayerToRegion(const FString& InRegionID)
{
    UKamoClientBeaconSubsystem* KamoBeaconSubsystem = GetWorld()->GetSubsystem<UKamoClientBeaconSubsystem>();
    if (!KamoBeaconSubsystem->Beacon->PlayerInfo.bIsValid || InRegionID.IsEmpty())
    {
        UE_CLOG(!KamoBeaconSubsystem->Beacon->PlayerInfo.bIsValid, LogBeacon, Warning, TEXT("MovePlayerToRegion: Player Info not valid."));
        UE_CLOG(InRegionID.IsEmpty(), LogBeacon, Warning, TEXT("MovePlayerToRegion: 'InRegionID' is empty."));
        return;
    }
    
    KamoBeaconSubsystem->Beacon->ServerMovePlayerToRegion(InRegionID);

}

TArray<FKamoRegionInfo>UViewmodelBaseProjectInfo::GetRegions()
{
    UKamoClientBeaconSubsystem* KamoBeaconSubsystem = GetWorld()->GetSubsystem<UKamoClientBeaconSubsystem>();

    if (!KamoBeaconSubsystem->Beacon->KamoWorldState)
    {
        TArray<FKamoRegionInfo> empty_region_info;
        return empty_region_info;
    }
    return KamoBeaconSubsystem->Beacon->KamoWorldState->Regions;

}

FString UViewmodelBaseProjectInfo::GetPlayerRegion()
{
    UKamoClientBeaconSubsystem* KamoBeaconSubsystem = GetWorld()->GetSubsystem<UKamoClientBeaconSubsystem>();
    return KamoBeaconSubsystem->Beacon->PlayerInfo.Location.RegionId;
}

FString UViewmodelBaseProjectInfo::GetBeaconStatus()
{
    return "Beacon Status";
}

FString UViewmodelBaseProjectInfo::GetPlayerID()
{
    UKamoClientBeaconSubsystem* KamoBeaconSubsystem = GetWorld()->GetSubsystem<UKamoClientBeaconSubsystem>();
    return KamoBeaconSubsystem->Beacon->PlayerInfo.PlayerId;
}

FString UViewmodelBaseProjectInfo::GetSecondsSinceLastHeartbeat()
{
    UKamoClientBeaconSubsystem* KamoBeaconSubsystem = GetWorld()->GetSubsystem<UKamoClientBeaconSubsystem>();
    return FString::FromInt(KamoBeaconSubsystem->Beacon->PlayerInfo.Location.Handler.SecondsFromLastHeartbeat) + " sec";
}

FString UViewmodelBaseProjectInfo::GetPlayerServerAddress()
{
    UKamoClientBeaconSubsystem* KamoBeaconSubsystem = GetWorld()->GetSubsystem<UKamoClientBeaconSubsystem>();

    FString ipaddress = KamoBeaconSubsystem->Beacon->PlayerInfo.Location.Handler.IpAddress;
    FString port = FString::FromInt(KamoBeaconSubsystem->Beacon->PlayerInfo.Location.Handler.Port);

    return ipaddress + ":" + port;
}

FString UViewmodelBaseProjectInfo::GetBeaconServerAddress()
{
    UKamoClientBeaconSubsystem* KamoBeaconSubsystem = GetWorld()->GetSubsystem<UKamoClientBeaconSubsystem>();
    FString BeaconServerAddress;

    if (KamoBeaconSubsystem->Beacon->GetNetConnection())
    {
        BeaconServerAddress = KamoBeaconSubsystem->Beacon->GetNetConnection()->URL.ToString();
    }
    
   return BeaconServerAddress;
}

FString UViewmodelBaseProjectInfo::GetShouldAutoTravel()
{
    UKamoClientBeaconSubsystem* KamoBeaconSubsystem = GetWorld()->GetSubsystem<UKamoClientBeaconSubsystem>();
    return KamoBeaconSubsystem->Beacon->bAutoTravel ? "True" : "False";
}

TMap<FString, FString> UViewmodelBaseProjectInfo::GetProjectInfo()
{

    UKamoClientBeaconSubsystem* KamoBeaconSubsystem = GetWorld()->GetSubsystem<UKamoClientBeaconSubsystem>();

    TMap<FString, FString> ProjectInfo;
    ProjectInfo.Add(TEXT("Version"), GetProjectVersion());
    ProjectInfo.Add(TEXT("Target Region"), GetPlayerRegion());
    ProjectInfo.Add(TEXT("Last heartbeat"), GetSecondsSinceLastHeartbeat());
    ProjectInfo.Add(TEXT("Server Address"), GetPlayerServerAddress());
    ProjectInfo.Add(TEXT("Beacon Address"), GetBeaconServerAddress());
    ProjectInfo.Add(TEXT("ID"), GetPlayerID());
    ProjectInfo.Add(TEXT("Auto Travel"), GetShouldAutoTravel());

    return ProjectInfo;
}

void UWidgetBaseProjectInfo::SetViewmodel(UViewmodelBaseProjectInfo* viewmodel)
{
    ProjectInfoViewmodel = viewmodel;
}

UKamoClientSettings::UKamoClientSettings()
{
    KamoBeaconClass = AKamoBeaconClient::StaticClass();
}


void UKamoClientSettings::PostInitProperties()
{
    Super::PostInitProperties();

#if WITH_EDITOR
    if (IsTemplate())
    {
        ImportConsoleVariableValues();
    }
#endif // WITH_EDITOR
}

#if WITH_EDITOR

void UKamoClientSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property)
    {
        ExportValuesToConsoleVariables(PropertyChangedEvent.Property);
    }
}

#endif // WITH_EDITOR



