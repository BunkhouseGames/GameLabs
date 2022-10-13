// Copyright Directive Games, Inc. All Rights Reserved.
#include "KamoBeaconHost.h"
#include "KamoSettings.h"


AKamoBeaconHostObject::AKamoBeaconHostObject(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	TimeSinceWorldStateUpdate(100.0),
	TimeSincePlayerInfoUpdate(100.0)

{
	if (GetDefault<UKamoClientSettings>()->KamoBeaconClass)
	{
		ClientBeaconActorClass = GetDefault<UKamoClientSettings>()->KamoBeaconClass;
	}
	else
	{
		ClientBeaconActorClass = AKamoBeaconClient::StaticClass();
	}
	
	// ClientBeaconActorClass = GetDefault<UKamoClientSettings>()->KamoBeaconClass ? GetDefault<UKamoClientSettings>()->KamoBeaconClass : AKamoBeaconClient::StaticClass();
	BeaconTypeName = ClientBeaconActorClass->GetName();
}

bool AKamoBeaconHostObject::Init(UKamoRuntime* InKamoRuntime)
{
	KamoRuntime = InKamoRuntime;

#if !UE_BUILD_SHIPPING
	UE_LOG(LogBeacon, Verbose, TEXT("Init"));
#endif
	return true;
}

void AKamoBeaconHostObject::OnClientConnected(AOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection)
{
	Super::OnClientConnected(NewClientActor, ClientConnection);

	if (ClientConnection)
	{
		FString PlayerIdStr = ClientConnection->PlayerId.IsValid() ? *ClientConnection->PlayerId->ToDebugString() : TEXT("nullptr");
		FString Desc = ClientConnection->LowLevelDescribe();
		UE_LOG(LogBeacon, Display, TEXT("   New player: %s"), *Desc);
	}

	if (KamoWorldState == nullptr)
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Owner = this;
		KamoWorldState = GetWorld()->SpawnActor<AKamoWorldState>(AKamoWorldState::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);
		// Associate with this objects net driver for proper replication
		KamoWorldState->SetNetDriverName(GetNetDriverName());
	}

	AKamoBeaconClient* BeaconClient = Cast<AKamoBeaconClient>(NewClientActor);
	if (BeaconClient != NULL)
	{
		BeaconClient->KamoWorldState = KamoWorldState;
	}
}

AOnlineBeaconClient* AKamoBeaconHostObject::SpawnBeaconActor(UNetConnection* ClientConnection)
{	
	AOnlineBeaconClient* Beacon = Super::SpawnBeaconActor(ClientConnection);
	return Beacon;
}


bool AKamoBeaconHostObject::GetHandlerInfo(const KamoID& HandlerId, FKamoHandlerInfo& HandlerInfo) const
{
	if (UUE4ServerHandler* Handler = KamoRuntime->GetUE4Server(HandlerId))
	{
		HandlerInfo.HandlerId = Handler->id->GetID();
		HandlerInfo.IpAddress = Handler->ip_address;
		HandlerInfo.Port = Handler->port;
		HandlerInfo.MapName = Handler->map_name;
		HandlerInfo.StartTime = Handler->StartTime;
		HandlerInfo.LastRefresh = Handler->LastRefresh;
		HandlerInfo.SecondsFromLastHeartbeat = Handler->seconds_from_last_heartbeat;
		return true;
	}
	else
	{
		return false;
	}
}


void AKamoBeaconHostObject::Update(float DeltaTime)
{
	// Populate Handlers, simple 
	TimeSinceWorldStateUpdate += DeltaTime;
	TimeSincePlayerInfoUpdate += DeltaTime;

	// NOTE, TEMPORARY HACK/SOLUTION:
	// Poll the state of handlers and regions every 6.5 secods. This is a temporary solution.
	// Eventually any change to handler or region state will be broadcasted on the message bus
	// and we will simply subscribe to those evens and forward them accordingly.
	if (KamoRuntime == nullptr || KamoWorldState == nullptr)
	{
		return;
	}

	const UKamoClientSettings* Settings = GetDefault<UKamoClientSettings>();

	if (TimeSincePlayerInfoUpdate >= Settings->PlayerInfoPollInterval)
	{
		// Refresh player info and push
		for (auto BeaconClient : ClientActors)
		{
			if (AKamoBeaconClient* KamoBeaconClient = Cast<AKamoBeaconClient>(BeaconClient))
			{
				KamoBeaconClient->RefreshAndPushPlayerInfo();
			}
		}
		TimeSincePlayerInfoUpdate = 0.0;
	}

	if (TimeSinceWorldStateUpdate < Settings->WorldStatePollInterval)
	{
		return;
	}

	TimeSinceWorldStateUpdate = 0.0;
	TArray<FKamoHandlerInfo> Handlers;
	int32 NumStaleHandlers = 0;

	for (const KamoRootObject& RootOb : KamoRuntime->database->FindRootObjects(TEXT("ue4server")))
	{
		FKamoHandlerInfo HandlerInfo;
		if (GetHandlerInfo(RootOb.id, HandlerInfo))
		{
			if (HandlerInfo.SecondsFromLastHeartbeat > 30.0)
			{
				++NumStaleHandlers;
			}
			else
			{
				Handlers.Add(HandlerInfo);
			}
		}
		else
		{
			UE_LOG(LogBeacon, Warning, TEXT("Record found in DB but failed to get handler info: %s"), *RootOb.id());
		}
	}

	if (!bStaleHandlersReported && NumStaleHandlers > 0)
	{
		bStaleHandlersReported = true;
		UE_LOG(LogBeacon, Warning, TEXT("Total of %i stale handlers found in DB."), NumStaleHandlers);
	}

	TArray<FKamoRegionInfo> Regions;
	for (const KamoRootObject& RootOb : KamoRuntime->database->FindRootObjects(TEXT("region")))
	{
		FKamoRegionInfo RegionInfo;
		RegionInfo.RegionId = RootOb.id();
		RegionInfo.HandlerId = RootOb.handler_id();
		Regions.Add(RegionInfo);
	}

	if (KamoWorldState)
	{		
		KamoWorldState->MulticastUpdateHandlers(Handlers);
		KamoWorldState->MulticastUpdateRegions(Regions);
	}
}

// TODO: Move this to auxiliary tools as this should be app specific
bool AKamoBeaconHostObject::LookupPlayer(const FString& InPlayerIdentity, FKamoPlayerInfo& OutPlayerInfo) const
{
	if (KamoRuntime == nullptr)
	{
		return false;
	}

	/*

	If player exists in the DB:
		-	Look up the region the player is located in and populate 'RegionHandler' property.
		-	Using the 'homeworld_region' property in the player record look up that region
			and populate the 'HomeworldHandler' property.
		-	If the player is located in its homeworld then 'RegionHandler' can simply be assigned
			into 'HomeworldHandler'.
			
	If no player exists in the DB:
		-	Get a default region from the config, look it up and populate 'RegionHandler' property.
			Leave 'PlayerId' and 'HomeworldHandler' empty.

	Indicate interest in having a server handler for a region (applies to both Region and Homeworld):
		-	If no handler is available for the region then flag an interest by setting the region
			property 'server_requested_at' to the current UTC date. This will be picked up by the
			K|Node daemon and a server launched if all goes well. The timestamp is to give the daemon
			a chance to ignore the request if it's more than couple of minutes old.

	*/
	

	OutPlayerInfo.PlayerIdentity = InPlayerIdentity;
	OutPlayerInfo.PlayerId = FString::Printf(TEXT("%s.%s"), *GetDefault<UKamoClientSettings>()->KamoPlayerClassName , *InPlayerIdentity);

	// Region lookup lambda
	auto Lookup = [this](FPlayerLocation& OutLocation)
	{
		KamoRootObject RegionObject = KamoRuntime->database->GetRootObject(KamoID(OutLocation.RegionId));
		if (RegionObject.IsEmpty())
		{
			UE_LOG(LogBeacon, Error, TEXT("Region id points to nothing [%s]"), *OutLocation.RegionId);
		}
		else
		{
			if (RegionObject.handler_id.IsEmpty())
			{
				// No server running for this region. Let's indicate some interest in having one.
				// Note that this direct DB writing is possible because there is no handler for this region.
				// There is however a slight possibility of a race condition but it is of no consequence.
				auto state = NewObject<UKamoState>();
				state->SetState(RegionObject.state);
				FString UTCNow = FDateTime::UtcNow().ToIso8601().Replace(TEXT(":"), TEXT("-"));
				state->SetString("server_requested_at", UTCNow);				
				KamoRuntime->database->UpdateRootObject(RegionObject.id, state->GetStateAsString());
			}
			else
			{
				UUE4ServerHandler* Handler = KamoRuntime->GetUE4Server(KamoID(RegionObject.handler_id));
				if (Handler)
				{
					OutLocation.Handler.HandlerId = Handler->id->GetPrimitive()();
					OutLocation.Handler.IpAddress = Handler->ip_address;
					OutLocation.Handler.Port = Handler->port;
					OutLocation.Handler.MapName = Handler->map_name;
					OutLocation.Handler.StartTime = Handler->StartTime;
					OutLocation.Handler.LastRefresh = Handler->LastRefresh;
					OutLocation.Handler.SecondsFromLastHeartbeat = Handler->seconds_from_last_heartbeat;
					//OutLocation.Handler.HandledRegions
				}
			}
		}

	};

	// NOTE: Not taking advantage of looking into current runtime internal_state but rather just dipping into the DB
	KamoChildObject KamoPlayer = KamoRuntime->database->GetObject(KamoID(OutPlayerInfo.PlayerId), /*fail_silently*/true);
	if (!KamoPlayer.IsEmpty())
	{
		OutPlayerInfo.Location.RegionId = KamoPlayer.root_id();
		auto state = NewObject<UKamoState>();
		state->SetState(KamoPlayer.state);
		state->GetString("howeworld_region_id", OutPlayerInfo.Homeworld.RegionId);
	}
	else
	{
		OutPlayerInfo.Location.RegionId = UKamoProjectSettings::Get()->DefaultPlayerRegion;

#if !UE_BUILD_SHIPPING
		// If player does not exists and there is no default player region specified and the incoming player is a PIE player
		// we just pick the currently running world as the default player region for development convenience.
		if (OutPlayerInfo.Location.RegionId.IsEmpty() && InPlayerIdentity.StartsWith(TEXT("pieplayer")))
		{
			OutPlayerInfo.Location.RegionId = KamoRuntime->FormatCurrentRegionName();
			UE_LOG(LogBeacon, Display, TEXT("No default region for new players is specified in settings. Using current one as a fallback: %s"), *OutPlayerInfo.Location.RegionId);			
		}
#endif
	}

	// Lookup player location and players homeworld
	Lookup(OutPlayerInfo.Location);

	if (OutPlayerInfo.Homeworld.RegionId == OutPlayerInfo.Location.RegionId)
	{
		OutPlayerInfo.Homeworld = OutPlayerInfo.Location;
	}
	else if (!OutPlayerInfo.Homeworld.RegionId.IsEmpty())
	{
		Lookup(OutPlayerInfo.Homeworld);
	}

	return true;
}


bool AKamoBeaconHostObject::RefreshWorldState(AKamoWorldState* OutWorldState)
{
	return false;
}


void AKamoBeaconHostObject::LaunchServerForPlayer(const FKamoPlayerInfo& PlayerInfo)
{
	//
}


void AKamoBeaconHostObject::MovePlayerToRegion(const FString& InPlayerID, const FString& InRegionID, const FString& SpawnTarget)
{
	KamoRuntime->MoveObjectSafely(KamoID(InPlayerID), KamoID(InRegionID), SpawnTarget);
}