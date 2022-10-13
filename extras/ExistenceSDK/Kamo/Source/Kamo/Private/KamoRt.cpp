// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.


#include "KamoRt.h"
#include "KamoRuntime.h" // just for log category, plz fix
#include "KamoBeaconHost.h"

// From Kamo module
#include "KamoSettings.h"
#include "KamoModule.h"
#include "KamoVolume.h"

// Unreal Engine
#include "KamoPersistable.h"
#include "Misc/CoreMisc.h"
#include "Misc/SecureHash.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "SocketSubsystem.h"
#include "OnlineBeaconHost.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

typedef TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>> FCondensedJsonStringWriterFactory;

FKamoRuntimeEvent UKamoRuntime::runtime_event;


// Stats
DECLARE_CYCLE_STAT(TEXT("Tick"), STAT_KamoTick, STATGROUP_Kamo);
DECLARE_CYCLE_STAT(TEXT("MarkForUpdate"), STAT_MarkForUpdate, STATGROUP_Kamo);
DECLARE_CYCLE_STAT(TEXT("SerializeObjects"), STAT_SerializeObjects, STATGROUP_Kamo);
DECLARE_CYCLE_STAT(TEXT("FlushStateToActors"), STAT_FlushStateToActors, STATGROUP_Kamo);
DECLARE_CYCLE_STAT(TEXT("ProcessMessageQueue"), STAT_ProcessMessageQueue, STATGROUP_Kamo);
DECLARE_CYCLE_STAT(TEXT("MoveObject"), STAT_MoveObject, STATGROUP_Kamo);
DECLARE_CYCLE_STAT(TEXT("OnActorSpawned"), STAT_OnActorSpawned, STATGROUP_Kamo);
DECLARE_CYCLE_STAT(TEXT("UpdateKamoStateFromActor"), STAT_UpdateKamoStateFromActor, STATGROUP_Kamo);
DECLARE_CYCLE_STAT(TEXT("FlushToDB"), STAT_FlushToDB, STATGROUP_Kamo);
DECLARE_CYCLE_STAT(TEXT("DeleteFromDB"), STAT_DeleteFromDB, STATGROUP_Kamo);

DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("TotalObjects"), STAT_TotalObjects, STATGROUP_Kamo);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("DirtyObject"), STAT_DirtyObjects, STATGROUP_Kamo);


UKamoRuntime::UKamoRuntime() :
	runtime_is_spawning(false),
	last_refresh_seconds(1000.0f), // High enough number to trigger a heartbeat on first tick.
	power_save_seconds(0.0f),
	is_initialized(false),
	poll_message_queue(true),
	mark_and_sync_elapsed(0.0f),
	message_queue_flush_elapsed(0.0f)
{
}


void UKamoRuntime::BeginDestroy()
{
	Super::BeginDestroy();
	if (is_initialized)
	{
		SerializeObjects();
	}
	UE_LOG(LogKamoRt, Display, TEXT("Runtime BeginDestroy running"));
}

bool UKamoRuntime::IsReadyForFinishDestroy()
{
	// Wait if any serialization is pending
	if (database && database->IsSerializationPending(KamoID()))
	{
		return false; // Wait until all is flushed.
	}

	return Super::IsReadyForFinishDestroy();
}

void UKamoRuntime::FinishDestroy()
{
	UE_LOG(LogKamoRt, Display, TEXT("Runtime FinishDestroy running"));
	Super::FinishDestroy();
}


void UKamoRuntime::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	/*
	The initialization of Kamo Runtime needs to be defered to BeginPlay as the World is
	not fully initialized at this point.

	NOTE: Here is BeginPlay, PostLoadMapWithWorld and OnStartGameInstance events in context:

	EngineLoop::Init
		GameInstance::StartGameInstance
			Engine::Browse
				Engine::LoadMap
					World::BeginPlay
						OnWorldBeginPlay.Broadcast()
					PostLoadMapWithWorld.Broadcast(World)
			FWorldDelegates::OnStartGameInstance.Broadcast(this)


	We run UKamoRuntime::Init() in response to the OnWorldBeginPlay event.
	*/

	UWorld* World = GetWorld();
	WorldBeginPlayHandle = World->OnWorldBeginPlay.AddUObject(this, &UKamoRuntime::HandleWorldBeginPlay);
}


void UKamoRuntime::Deinitialize()
{
	UWorld* World = GetWorld();
	World->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
	ShutdownRuntime();
	Super::Deinitialize();
}


bool UKamoRuntime::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	// Not supporting EWorldType::Editor 
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}


bool UKamoRuntime::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	check(World);

	// Kamo runtime does not run in Stand-alone or Client NetMode
	ENetMode NetMode = World->GetNetMode();
	if (NetMode == NM_Standalone || NetMode == NM_Client)
	{
		UE_LOG(LogKamoRt, Display, TEXT("KamoRuntime not initialized for stand-alone or client world."));
		return false;
	}

	FString WorldName = World->GetName();

	// Before the engine loads in a map, an "Untitled" transitory world is always created. We ignore those as well.
	if (WorldName == TEXT("Untitled"))
	{
		return false;
	}

	// If the current map is on the exclusion list we will not create a Kamo Runtime
	for (const FSoftObjectPath& Map : UKamoProjectSettings::Get()->ExcludedKamoMaps)
	{
		if (WorldName == Map.GetAssetName())
		{
			UE_LOG(LogKamoRt, Display, TEXT("KamoRuntime not initialized because map is on exclusion list: %s"), *WorldName);
			return false;
		}
	}

	return true;
}


void UKamoRuntime::HandleWorldBeginPlay() const
{
	UWorld* World = GetWorld();
	bool bAuth = (World->GetAuthGameMode() != nullptr);

	if (bAuth)
	{
		UKamoRuntime* Runtime = const_cast<UKamoRuntime*>(this);
		Runtime->Init();
	}
	else
	{
		UE_LOG(LogKamoRt, Warning, TEXT("KamoRuntime not initialized because we are not authorative for world %s"), *World->GetMapName());
	}
}






void UKamoRuntime::Tick(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_KamoTick);
	if (!is_initialized)
	{
		return;
	}

	auto settings = UKamoProjectSettings::Get();
	AGameModeBase* gamemode = UGameplayStatics::GetGameMode(GetWorld());

	last_refresh_seconds += DeltaTime;
	power_save_seconds += DeltaTime;

	// Tick drivers
	if (database)
	{
		database->Tick(DeltaTime);
	}
	if (message_queue)
	{
		message_queue->Tick(DeltaTime);
	}

	// Move Kamo actors between regions if needed
	for (auto it = internal_state.CreateIterator(); it; ++it)
	{
		UKamoActor* actor_object = Cast<UKamoActor>(it.Value());
		if (actor_object && actor_object->GetActor())
		{
			auto where_it_should_be = MapActorToRegion(actor_object->GetActor());
			if (where_it_should_be != actor_object->root_id->GetID())
			{
				UE_LOG(LogKamoRt, Display, TEXT("AutoMoving Kamo actor '%s' from '%s' to '%s'."), 
					*actor_object->id->GetID(), *actor_object->root_id->GetID(), *where_it_should_be);
				MoveObject(actor_object->id->GetPrimitive(), KamoID(where_it_should_be), TEXT("ignore"));
			}
		}
	}

	if (gamemode && gamemode->GetNumPlayers() > 0)
	{
		// We have players, reset power save timer
		power_save_seconds = 0.0;
	}

	if (database && !handler.id.IsEmpty())
	{
		if (last_refresh_seconds > settings->server_heartbeat_interval)
		{
			last_refresh_seconds = 0.0f;

			// Prepare some statistics
			TSharedPtr<FJsonObject> stats = MakeShareable(new FJsonObject);
			GatherStats(stats);
			database->RefreshHandler(handler.id, stats);
			
			FString statsString;

			auto Writer = FCondensedJsonStringWriterFactory::Create(&statsString);
			FJsonSerializer::Serialize(stats.ToSharedRef(), Writer);

			UE_LOG(LogKamoRt, VeryVerbose, TEXT("{\"kamo_health\": %s"), *statsString);
		}
	}

	#if PLATFORM_LINUX
		bool power_save = true;
	#else
		bool power_save = !settings->power_save_only_on_linux;
	#endif

	if (IsRunningDedicatedServer() && power_save_seconds > settings->power_saving_after_sec && power_save && settings->power_saving_after_sec > 0.0)
	{
		UE_LOG(LogKamoRt, Display, TEXT("Power saving mode: No players active for %i seconds. Shutting down!"), int(power_save_seconds));
		power_save_seconds = -1000.0; // Effectively turn off power saving
		FGenericPlatformMisc::RequestExit(false);
	}

	// Tick all objects and flush out dead ones from our internal map
	for (auto it = internal_state.CreateIterator(); it; ++it)
	{
		if (!it.Value())
		{
			it.RemoveCurrent();
		}
	}

	internal_state.Compact();
	internal_state.Shrink();	

	// Mark and sync processing
	mark_and_sync_elapsed += DeltaTime;
	if (mark_and_sync_elapsed >= settings->mark_and_sync_rate)
	{
		MarkForUpdate();
		SerializeObjects();
		mark_and_sync_elapsed = 0.0f;
	}

	// Message queue processing
	message_queue_flush_elapsed += DeltaTime;
	if (poll_message_queue && message_queue_flush_elapsed >= settings->message_queue_flush_rate)
	{
		ProcessMessageQueue();
		message_queue_flush_elapsed = 0.0f;
	}

	// Actor state processing
	FlushStateToActors();

	// Update stats
	SET_DWORD_STAT(STAT_TotalObjects, internal_state.Num());

	// Tick Beacon host
	if (KamoBeaconHostObject)
	{
		KamoBeaconHostObject->Update(DeltaTime);
	}

	TickIdentityActors(DeltaTime);
}


void UKamoRuntime::GatherStats(const TSharedPtr < FJsonObject >& stats) const
{
	int32 Seconds;
	float PartialSeconds;
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
	UGameplayStatics::GetAccurateRealTime(GetWorld(), Seconds, PartialSeconds); // Returns time in seconds since the application was started.
#else
	UGameplayStatics::GetAccurateRealTime(Seconds, PartialSeconds); // Returns time in seconds since the application was started.
#endif
	stats->SetNumberField("time_accurate_real_time_seconds", Seconds);
	stats->SetNumberField("time_accurate_real_time_partial_seconds", PartialSeconds);

	stats->SetStringField("platform_name", UGameplayStatics::GetPlatformName()); // Returns the string name of the current platform, to perform different behavior based on platform.
	stats->SetNumberField("time_real_time_seconds", UGameplayStatics::GetRealTimeSeconds(GetWorld())); // Returns time in seconds since world was brought up for play, does NOT stop when game pauses, NOT dilated / clamped
	stats->SetNumberField("time_world_seconds", UGameplayStatics::GetTimeSeconds(GetWorld())); // Returns time in seconds since world was brought up for play, adjusted by time dilationand IS stopped when game pauses
	stats->SetNumberField("time_unpaused_time_seconds", UGameplayStatics::GetUnpausedTimeSeconds(GetWorld())); // Returns time in seconds since world was brought up for play, adjusted by time dilationand IS NOT stopped when game pauses

	stats->SetNumberField("num_kamo_objects", internal_state.Num());
	stats->SetStringField("region_instance_id", region_instance_id);
	stats->SetStringField("map_name", GetWorld()->GetMapName());
	TArray <TSharedPtr<FJsonValue> > regions;
	for (auto region_id : registered_regions)
	{
		regions.Add(MakeShareable(new FJsonValueString(region_id())));
	}
	stats->SetArrayField("registered_regions", regions);

	AGameModeBase* gamemode = UGameplayStatics::GetGameMode(GetWorld());
	if (gamemode)
	{
		stats->SetNumberField("gamemode_num_players", gamemode->GetNumPlayers()); // Returns number of active human players, excluding spectators
		stats->SetNumberField("gamemode_num_spectators", gamemode->GetNumSpectators()); // Returns number of human players currently spectating

		int average_ping = 0;
		TArray <TSharedPtr<FJsonValue> > players;
		if (gamemode->GameState->PlayerArray.Num() != 0)
		{
			TSharedPtr<FJsonObject> player_stats = MakeShareable(new FJsonObject);			
			for (APlayerState* ps : gamemode->GameState->PlayerArray)
			{
				average_ping = average_ping + ps->GetPingInMilliseconds();

				auto KamoObjectIDToJson = [](const UKamoObject* ob) ->TSharedPtr<FJsonValue>
				{
					if (ob && !ob->id->IsEmpty())
					{
						return MakeShared<FJsonValueString>(ob->id->GetID());
					}
					else
					{
						return MakeShared<FJsonValueNull>();
					}
				};

				////player_stats->SetNumberField(FString::FromInt(ps->GetPlayerId()), ps->GetPing());
				player_stats->SetField("kamo_id", KamoObjectIDToJson(GetObjectFromActor(ps->GetOwner())));
				player_stats->SetNumberField("player_id", ps->GetPlayerId());
				player_stats->SetNumberField("ping", ps->GetPingInMilliseconds());
				player_stats->SetNumberField("start_time", ps->GetStartTime());
				player_stats->SetBoolField("is_spectator", ps->IsSpectator());
				player_stats->SetBoolField("is_inactive", ps->IsInactive());
				
				// Note, not all PlayerState is owned by player controllers
				APlayerController* pc = Cast<APlayerController>(ps->GetOwner());
				if (pc)
				{
					auto possessed = GetObjectFromActor(pc->GetPawnOrSpectator());
					player_stats->SetField("pawn_kamo_id", KamoObjectIDToJson(possessed));
					
					UNetConnection* net = pc->GetNetConnection();
					if (net)
					{
						// player_stats->SetStringField("net_describe", net->Describe()); // this is too verbose
						player_stats->SetStringField("net_online_platform", net->GetPlayerOnlinePlatformName().ToString());
						player_stats->SetStringField("net_remote_address", net->LowLevelGetRemoteAddress(true));
						player_stats->SetStringField("net_request_url", net->RequestURL);
						player_stats->SetNumberField("net_uptime", Seconds - int32(net->GetConnectTime()));
					}


				}


				players.Add(MakeShareable(new FJsonValueObject(player_stats)));
			}

			average_ping = average_ping / gamemode->GameState->PlayerArray.Num();
		}
		stats->SetArrayField("players", players);
		stats->SetNumberField("gamemode_average_ping", average_ping);
	}

}


bool UKamoRuntime::IsTickable() const
{
	return is_initialized;
}


TStatId UKamoRuntime::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UKamoRuntime, STATGROUP_Tickables);
}


// Standard init
bool UKamoRuntime::Init()
{
	SCOPE_LOG_TIME_IN_SECONDS(TEXT("Kamo::Init (sec)"), nullptr);

	UWorld* World = GetWorld();

	if (!World->URL.Host.IsEmpty())
	{
		UE_LOG(LogKamoRt, Display, TEXT("Not initializing Kamo Runtime as server is remote: %s"), *World->URL.GetHostPortString());
		return false;
	}

	server_id.class_name = "ue4server";
	server_id.unique_id = FString::FromInt(World->URL.Port);

	kamo_table = GetKamoTable();
	if (!kamo_table)
	{
		return false;
	}
	// TODO: Populate table with 'classmap' entries
	// ...

	FString driver = KamoUtil::get_driver_name();
	UE_LOG(LogKamoRt, Display, TEXT("KamoRuntime::Init - Driver type: %s"), *driver);
	database = IKamoDB::CreateDriver(driver);
	if (!database)
	{
		return false;
	}

	if (!database->CreateSession(KamoUtil::get_tenant_name()))
	{
		return false;
	}

	actor_spawned_delegate = FOnActorSpawned::FDelegate::CreateUObject(
		this, &UKamoRuntime::OnActorSpawned);
	World->AddOnActorSpawnedHandler(actor_spawned_delegate);

	if (GetUE4Server(server_id))
	{
		// Do a convenience cleanup for local development
		if (server_id() == TEXT("ue4server.local"))
		{
			UE_LOG(LogKamoRt, Warning, TEXT("Init: UE4Server object already exists: %s. Doing a courtecy cleanup for local development."), *server_id());
			if (!database->DeleteRootObject(server_id))
			{
				return false;
			}
		}
		else
		{
			UE_LOG(LogKamoRt, Warning, TEXT("Init: UE4Server object already exists: %s. Is it a lingering object? Well, doing a courtecy cleanup anyhow."), *server_id());
			if (!database->DeleteRootObject(server_id))
			{
				return false;
			}
		}
	}

	handler.id = server_id;

	// An IP address from the command line is always used
	handler.ip_address = GetCommandLineArgument("ip_address");
	
	// If not we try to use the local host address
	if (handler.ip_address.IsEmpty())
	{
		// Read the IP from the system
		bool bCanBindAll;
		TSharedPtr<class FInternetAddr> HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBindAll);
		// Now set the port that was configured
		HostAddr->SetPort(World->URL.Port);
		handler.ip_address = HostAddr->ToString(false);
		World->URL.Host = handler.ip_address;
	}

	handler.port = World->URL.Port;
	handler.map_name = World->URL.Map;
	// TODO: Make this random when we can pass it to clients through the backend
	handler.secret = TEXT("759DC32D-B918-410E-8A14-8A61FB811F80");
	// TODO: FIX SERIALIZATION OF THIS. HARDCODED FOR NOW:
	handler.state = FString::Printf(TEXT("{\"ip_address\": \"%s\", \"port\": %i, \"map_name\": \"%s\"}"), *handler.ip_address, handler.port, *handler.map_name);

	if (!RegisterUE4Handler(handler))
	{
		UE_LOG(LogKamoRt, Error, TEXT("Init: Failed to RegisterUE4Server: %s"), *server_id());
		return false;
	}

	// Send me a message to myself just to verify message queue
	{
		TArray<UKamoState*> commands;
		auto command = CreateMessageCommand(KamoID(), KamoID(), "log", "{\"message\": \"Message queue check.\"}");
		commands.Add(command);
		message_queue->SendMessage("", "command", CreateCommandMessagePayload(commands)->GetStateAsString());
		message_queue->SendMessage(handler.inbox_address, "command", CreateCommandMessagePayload(commands)->GetStateAsString());
	}

	// Initialize region instance id if specified.
	region_instance_id = GetCommandLineArgument(TEXT("kamoinstanceid"));

	// Parse in Kamo Volumes Whitelist
	TArray<FString> tokens;
	TArray<FString> switches;
	FCommandLine::Parse(FCommandLine::Get(), tokens, switches);
	for (auto arg : switches)
	{
		TArray<FString> tuple;
		arg.ParseIntoArray(tuple, TEXT("="), true);
		if (tuple.Num() > 1 && tuple[0].Equals(TEXT("kamovolumes"), ESearchCase::IgnoreCase))
		{
			tuple[1].TrimQuotes().ParseIntoArray(region_volume_whitelist, TEXT(" "));
			UE_LOG(LogKamoRt, Display, TEXT("Init: Region volumes whitelist: %s"), *tuple[1].TrimQuotes());
			break;
		}
	}

	TArray<KamoID> regions;
	FString region_name = FormatCurrentRegionName(); // The main level volume

	if (region_volume_whitelist.Num() == 0)
	{
		// The main level volume is implicitly whitelisted.
		UE_LOG(LogKamoRt, Display, TEXT("LoadRegions: Level volumes whitelist not specified. Loading main and all volumes."));		
		regions.Add(KamoID(region_name));
	}
	else if (region_volume_whitelist.Contains("main"))
	{
		// The main level volume can be explicitly stated as 'main' on the command line
		regions.Add(KamoID(region_name));
		UE_LOG(LogKamoRt, Display, TEXT("LoadRegions: Region '%s' prepared for loading."), *region_name);
	}
	else
	{
		UE_LOG(LogKamoRt, Display, TEXT("LoadRegions: The main level volume will NOT be loaded on this server: %s"), *region_name);
	}
	
	// Enumerate region volumes in the current map
	TArray<AActor*> actors;
	UGameplayStatics::GetAllActorsOfClass(World, AKamoVolume::StaticClass(), actors);
	for (auto actor : actors)
	{
		RegionVolume rv;
		rv.volume_name = actor->GetName().ToLower();
		actor->GetActorBounds(false, rv.origin, rv.box_extent);
		region_volumes.Add(rv);
			
		if (region_volume_whitelist.Num() && !region_volume_whitelist.Contains(rv.volume_name))
		{
			UE_LOG(LogKamoRt, Display, TEXT("LoadRegions: Region volume '%s' not loaded as it's not specified in whitelist."), *rv.volume_name);
		}
		else
		{
			// Load and register this region			
			region_name = FormatCurrentRegionName(rv.volume_name);
			regions.Add(KamoID(region_name));
			UE_LOG(LogKamoRt, Display, TEXT("LoadRegions: Region '%s' prepared for loading."), *region_name);
		}
	}

	// Register regions
	for (auto region_id : regions)
	{
		// Automatically create the region if it doesn't exist
		auto check_region = database->GetRootObject(region_id);
		if (check_region.IsEmpty())
		{
			UE_LOG(LogKamoRt, Warning, TEXT("Region %s not found in DB,  attempting to create..."), *region_id());
			auto added = database->AddRootObject(region_id, "{}");
			if (!added)
			{
				UE_LOG(LogKamoRt, Error, TEXT("Failed to auto-create region: %s"), *region_id());

				if (IsRunningDedicatedServer())
				{
					UE_LOG(LogKamoRt, Error, TEXT("Exiting as we are a dedicated server."));
					FGenericPlatformMisc::RequestExit(false);
					return false;
				}
			}
		}

		// Load in the region
		if (LoadAndPossessRegion(region_id))
		{			
			UE_LOG(LogKamoRt, Display, TEXT("Loaded region: %s"), *region_id());
		}
		else if (IsRunningDedicatedServer())
		{
			UE_LOG(LogKamoRt, Error, TEXT("KamoRuntime::LoadAndPossessRegion - Failed to load region: %s. Exiting as we are a dedicated server."), *region_id());
			FGenericPlatformMisc::RequestExit(false);
			return false;
		}
	}

	InitBeaconHost();
	InitializeKamoLevelActors();

	runtime_event.Broadcast(this);

	AddToRoot();
	is_initialized = true;

#if PLATFORM_WINDOWS
	if (IsRunningDedicatedServer())
	{
		FString title = FString::Printf(
			TEXT("Kamo Server: %s - %s [%s:%i]"),
			*KamoUtil::get_tenant_name(), *FormatCurrentRegionName(), *handler.ip_address, handler.port
		);
		::SetConsoleTitleW(*title);
	}
#endif

	return true;
}


FString UKamoRuntime::FormatCurrentRegionName(const FString& volume_name) const
{
	return FormatRegionName(*UWorld::RemovePIEPrefix(GetWorld()->GetMapName()), volume_name, region_instance_id);
}


FString UKamoRuntime::FormatRegionName(const FString& map_name, const FString& volume_name, const FString& instance_id) const
{
	// Note, this format is also used in kamo backend so make sure it's consistent between those two projects.
	FString region_name = FString::Printf(TEXT("region.%s"), *map_name);
	
	if (!volume_name.IsEmpty())
	{
		region_name += FString::Printf(TEXT("^%s"), *volume_name);
	}

	if (!instance_id.IsEmpty())
	{
		region_name += FString::Printf(TEXT("#%s"), *instance_id);
	}

	return region_name.ToLower();
}

void UKamoRuntime::InitBeaconHost()
{
	// https://answers.unrealengine.com/questions/467973/what-are-online-beacons-and-how-do-they-work.html
	
	if (BeaconHost)
	{
		BeaconHost->DestroyBeacon();
		BeaconHost = nullptr;
	}
	/*
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine, UGameInstance::StaticClass());
	GameInstance->AddToRoot();
	FString MapName = TEXT("/Game/Maps/Adalmappid.Adalmappid");
	FName PackageName = FPackageName::GetShortFName(MapName);
	//GameInstance->InitializeForMinimalNetRPC(PackageName);
	GameInstance->InitializeStandalone(PackageName);
	GameInstance->StartGameInstance();
	UWorld* World = GameInstance->GetWorld();
	*/
	UWorld* World = GetWorld();


	// Create the hosting connection
	BeaconHost = World->SpawnActor<AOnlineBeaconHost>(AOnlineBeaconHost::StaticClass());
	if (BeaconHost && BeaconHost->InitHost())
	{
		// Register a class to handle traffic of a specific type
		KamoBeaconHostObject = World->SpawnActor<AKamoBeaconHostObject>(AKamoBeaconHostObject::StaticClass());
		if (KamoBeaconHostObject)
		{
			KamoBeaconHostObject->Init(this);
			BeaconHost->RegisterHost(KamoBeaconHostObject);
			BeaconHost->PauseBeaconRequests(false);
		}
	}
}


void UKamoRuntime::InitializeKamoLevelActors()
{
	// Initialize Kamo level actors
	for (FActorIterator It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		FString unique_id;

		for (const FName& tag : Actor->Tags)
		{
			if (tag.ToString().StartsWith(TEXT("kamoid:")))
			{
				tag.ToString().Split(TEXT(":"), nullptr, &unique_id);
				if (unique_id.IsEmpty())
				{
					UE_LOG(LogKamoRt, Warning, TEXT("KamoRuntime::LoadAndPossessRegion - Can't Kamo-fy level actor as the unique id is empty. Tag: %s"), *tag.ToString());
				}

				break;
			}
		}

		if (unique_id.IsEmpty())
		{
			continue;
		}

		// NOTE: This is pretty much the same code as in OnActorSpawned - consider consolidation.
		auto class_entry = GetKamoClassEntryFromActor(Actor);

		if (class_entry.Key.IsEmpty())
		{
			UE_LOG(LogKamoRt, Warning, TEXT("KamoRuntime::LoadAndPossessRegion - Level actor not in class map: %s."), *Actor->GetName());
			continue;
		}

		// If Kamo object exists, attach it to actor, else create a new Kamo object.
		KamoID actor_id;
		actor_id.class_name = class_entry.Key;
		actor_id.unique_id = unique_id;

		UKamoActor * kamo_ob = Cast<UKamoActor>(GetObject(actor_id, /*fail_silently*/true));
		if (kamo_ob)
		{
			// Duplication AttachActorToKamoInstance logic here
			kamo_ob->SetObject(Actor);
			kamo_ob->OnObjectSet(Actor, this);
			kamo_ob->actor_is_set = true;
			kamo_ob->ApplyKamoStateToActor(EKamoStateStage::KSS_ActorPass);
			kamo_ob->ApplyKamoStateToActor(EKamoStateStage::KSS_ComponentPass);

		}
		else
		{
			// Ask game specific logic where this objects should be.
			UKamoActor* default_actor_obj = Cast<UKamoActor>(class_entry.Value->kamo_class.GetDefaultObject());
			UKamoID* region_id = default_actor_obj->MapSpawnedActorToRegion(Actor, this);
			if (region_id)
			{ 
				UKamoActor* new_object = Cast<UKamoActor>(CreateObject(class_entry.Key, region_id->GetPrimitive(), Actor, actor_id.unique_id));
				if (new_object)
				{
					new_object->object_ref_mode = EObjectRefMode::RM_AttachByTag;
				}
			}
		}
	}
}


FString UKamoRuntime::MapActorToRegion(AActor* actor) const
{
	return MapLocationToRegion(actor->GetActorLocation());
}


FString UKamoRuntime::MapLocationToRegion(const FVector& Location) const
{
	FString volume_name;

	// Find the region volume the actor is in
	for (auto volume : region_volumes)
	{
		const FBox Box(volume.origin - volume.box_extent, volume.origin + volume.box_extent);
		if (Box.IsInsideOrOn(Location))
		{
			volume_name = volume.volume_name;
			break;
		}
	}

	// The actor is in the main region volume
	return FormatCurrentRegionName(volume_name);
}

void UKamoRuntime::ShutdownRuntime()
{
	if (is_initialized)
	{
		SerializeObjects(true); // Flush and commit everything to DB
		runtime_event.Broadcast(nullptr);

		// Unregister this server
		if (!server_id.IsEmpty())
		{
			UnregisterUE4Server(server_id);
		}

		// Clear out the internal state.
		internal_state.Empty();

		is_initialized = false;

		// Mark this instance ready for GC
		RemoveFromRoot();
	}

	if (database)
	{
		database->CloseSession();
		database.Reset();
	}
	
	if (message_queue)
	{
		message_queue->CloseSession();
		message_queue.Reset();
	}

	runtime_event.Clear();
	registered_regions.Empty();
}


bool UKamoRuntime::LoadAndPossessRegion(const KamoID& root_id)
{
	SCOPE_LOG_TIME_IN_SECONDS(TEXT("Kamo::LoadAndPossessRegion (sec)"), nullptr);

	if (!SetHandler(root_id, server_id))
	{
		return false;
	}

	TArray<KamoChildObject> objects = database->FindObjects(root_id, "");
	for (auto object : objects)
	{
		auto state = NewObject<UKamoState>();
		state->SetState(object.state);
		RegisterKamoObject(object.id, root_id, state, nullptr, false, false);
    }

	// TODO: FIX OVERKILL: We call ResolveSubobjects again on all objects to give them proper chance
	// of resolving the object references.
	for (auto& elem : internal_state)
	{
		auto object = elem.Value;
		object->ResolveSubobjects(this);
	}


	UE_LOG(LogKamoRt, Display, TEXT("UKamoRuntime::LoadAndPossessRegion: Loaded %i objects for region: %s"), objects.Num(), *root_id());

	// Register meta info if this is the main level volume.
	if (root_id() == FormatCurrentRegionName())
	{
		//UE_LOG(LogKamoRt, Display, TEXT("UKamoRuntime::LoadAndPossessRegion: I want to add some meta info here: %s"), *root_id());
	}

	registered_regions.Add(root_id);

	return true;
}


TArray<KamoID> UKamoRuntime::GetServerRegions() const
{
	return registered_regions;
}

// Object querying
TArray<UKamoRootObject*> UKamoRuntime::FindRootObjects(const FString& class_name) const
{
    TArray<UKamoRootObject*> uobjects;

    auto objects = database->FindRootObjects(class_name);

    for (auto object : objects) {
        auto uobject = NewObject<UKamoRootObject>();

        uobject->Init(object);

        uobjects.Add(uobject);
    }

    return uobjects;
}

TArray<UKamoChildObject*> UKamoRuntime::FindObjects(const KamoID& root_id, const FString& class_name) const {
    TArray<UKamoChildObject*> uobjects;

    auto objects = database->FindObjects(root_id, class_name);

    for (auto object : objects) {
        auto uobject = NewObject<UKamoChildObject>();

        uobject->Init(object);

        uobjects.Add(uobject);
    }

    return uobjects;
}


EKamoLoadChildObject UKamoRuntime::LoadChildObjectFromDB(const KamoID& id, KamoID* root_id)
{
	// NOTE: Migration of logic from async branch, in particular fix #22 and #23: Cleaning
	// up LoadChildObject logic and Kamo Connector
	if (internal_state.Contains(id()))
	{		
		return EKamoLoadChildObject::Success;
	}

	// Make sure the object is in one of our regions
	if (root_id && !root_id->IsEmpty() && !registered_regions.Contains(*root_id))
	{
		return EKamoLoadChildObject::NotOurRegion;
	}

	auto object = database->GetObject(id, /*fail_silently*/true);

	if (object.IsEmpty()) 
	{
		return EKamoLoadChildObject::NotFound;
	}

	if (root_id)
	{
		*root_id = object.root_id;
	}

	// Make sure the object is in one of our regions
	if (registered_regions.Contains(object.root_id))
	{
		auto uobject = NewObject<UKamoChildObject>();
		uobject->Init(object);
		if (RegisterKamoObject(id, object.root_id, uobject->state) != nullptr)
		{
			uobject->ResolveSubobjects(this);
			return EKamoLoadChildObject::Success;
		}
		else
		{
			return EKamoLoadChildObject::RegisterFailed;
		}
	}
	else
	{
		return EKamoLoadChildObject::NotOurRegion;
	}	
}


UKamoObject* UKamoRuntime::GetProxyObject(const KamoID& id)
{
	auto child = database->GetObject(id);
	if (!child.IsEmpty())
	{
		auto uobject = NewObject<UKamoChildObject>();
		uobject->Init(child);
		return RegisterKamoObject(id, child.root_id, uobject->state, nullptr, true);
	}

	auto root = database->GetRootObject(id);
	if (!root.IsEmpty())
	{
		auto uobject = NewObject<UKamoRootObject>();
		uobject->Init(root);
		return RegisterKamoObject(id, KamoID(), uobject->state, nullptr, true);
	}

	return nullptr;
}

bool UKamoRuntime::OverwriteObjectState(const KamoID& id, const FString& state)
{
	auto key = id();

	if (!internal_state.Contains(key)) {
		return false;
	}

	auto object = internal_state[key];

	object->state->SetState(state);
	object->dirty = true;

	return true;
}

UKamoObject* UKamoRuntime::CreateObject(const FString& class_name, const KamoID& root_id, UObject* object, const FString& unique_id)
{
	auto added = false;

	KamoID id;
	id.class_name = class_name;

	if (unique_id.IsEmpty())
	{
		auto guid = FGuid::NewGuid().ToString().ToLower();
		id.unique_id = guid.Left(8);
	}
	else
	{
		id.unique_id = unique_id;
	}

	auto state = NewObject<UKamoState>();
	state->SetString(TEXT("mmo_actor_class"), class_name);
	state->SetString(TEXT("ue4_class"), object->GetClass()->GetPathName());

	auto uobject = RegisterKamoObject(id, root_id, state, object, false, true);
	uobject->ResolveSubobjects(this);


	if (uobject) {
		uobject->isNew = true;
	}

	return uobject;
}

bool UKamoRuntime::MoveObject(const KamoID& id, const KamoID& root_id, const FString& spawn_target)
{
	SCOPE_CYCLE_COUNTER(STAT_MoveObject);

	UE_LOG(LogKamoRt, Display, TEXT("MoveObject %s to %s"), *id(), *root_id());
	if (!id.IsValid() || !root_id.IsValid())
	{
		UE_LOG(LogKamoRt, Error, TEXT("MoveObject: 'id' or 'root_id' is not valid."));
		return false;
	}

	// Prepare move of this object and its subobjects
	UKamoActor* KamoActor = Cast<UKamoActor>(GetObject(id, /*fail_silently*/false));
	if (KamoActor == nullptr)
	{
		return false;
	}

	// Notify Kamo Object of this move
	KamoActor->OnMove(root_id);

	// Specify the spawn target
	KamoActor->GetKamoState()->SetString("spawn_target", spawn_target);

	// Move all subobjects. We can make this fancier later
	TArray<KamoID> MoveIDList;
	MoveIDList.Add(id);
	for (auto SubobjectsKv : KamoActor->GetSubobjects())
	{
		KamoID subobject_id = SubobjectsKv.Value->id->GetPrimitive();
		MoveIDList.Add(subobject_id);
		if (UKamoActor* KamoSubActor = Cast<UKamoActor>(GetObject(subobject_id, /*fail_silently*/true)))
		{
			KamoSubActor->OnMove(root_id);
		}
	}
	
	// Mark and Sync
	MarkForUpdate();
    SerializeObjects();

	for (const KamoID& PendingId : MoveIDList)
	{
		while (database->IsSerializationPending(PendingId))
		{
			FPlatformProcess::Sleep(0.0f);
		}
	}

	// Move
	for (const KamoID& PendingId : MoveIDList)
	{
		if (!database->MoveObject(PendingId, root_id)) {
			UE_LOG(LogKamoRt, Error, TEXT("FAILED TO MOVE OBJECT IN DATABASE"));
			return false;
		}

		// Remove
		if (internal_state.Remove(PendingId()) != 1) {
			UE_LOG(LogKamoRt, Error, TEXT("FAILED TO REMOVE OBJECT FROM INTERNAL STATE"));
			return false;
		}
	}

    // Notify new handler

    auto root_object = database->GetRootObject(root_id);

    // Done if no handler
    if (root_object.handler_id.IsEmpty()) 
	{
        return true;
    }

	TArray<UKamoState*> commands;
    auto command = CreateMessageCommand(id, root_id, "load_childobject_from_db", "");

    // Insert the move command at top
	commands.Insert(command, 0);
	auto payload = CreateCommandMessagePayload(commands);
    auto message_sent = SendMessage(root_object.handler_id, "command", payload->GetStateAsString());
    if (!message_sent) 
	{
        UE_LOG(LogKamoRt, Warning, TEXT("FAILED TO SEND MESSAGE TO NEW HANDLER"));
    }

    return true;
}


// Move object regardless of its location state
bool UKamoRuntime::MoveObjectSafely(const KamoID& id, const KamoID& root_id, const FString& spawn_target)
{
	if (internal_state.Contains(id()))
	{
		// It's a local object so MoveObject will handle this
		return MoveObject(id, root_id, spawn_target);
	}

	// Get object information - it must exist and have a valid root id for us to continue.
	KamoChildObject object = database->GetObject(id);
	if (object.IsEmpty() || object.root_id.IsEmpty())
	{
		return false;
	}

	// If there's an active handler for the object the move must be performed by that handler
	UUE4ServerHandler* Handler = GetUE4ServerForRoot(object.root_id);
	if (Handler && Handler->seconds_from_last_heartbeat < 60)
	{
		// Construct a inbox command as this move operation must be performed on the owning server.
		FString parameters = FString::Printf(TEXT("{\"to\": \"%s\", \"spawn_target\": \"%s\"}"), *root_id(), *spawn_target);
		return SendCommandToObject(id, "move_object", parameters);
	}

	// Do a straight DB move, take care of subobjects and finally notify the target handler if applicable
	if (!database->MoveObject(id, root_id))
	{
		return false;
	}

	// Move all the subobjects
	UKamoState* ObjectState = NewObject<UKamoState>();
	if (ObjectState->SetState(object.state))
	{
		UKamoState* subobjects;
		if (ObjectState->GetKamoState("kamo_subobjects", subobjects))
		{
			for (const FString& key : subobjects->GetKamoStateKeys())
			{
				FString kamo_id_str;
				subobjects->GetString(key, kamo_id_str);
				if (!database->MoveObject(KamoID(kamo_id_str), root_id))
				{
					UE_LOG(LogKamoRt, Warning, TEXT("MoveObjectSafely: Failed to move subobject %s of  %s to %s."), *kamo_id_str, *id(), *root_id());
				}
			}
		}
	}

	// Notify the target handler
	if (!SendCommandToObject(id, "load_childobject_from_db", "{}"))
	{
		UE_LOG(LogKamoRt, Warning, TEXT("MoveObjectSafely: Failed to notify object arrival %s to region %s."), *id(), *root_id());
	}

	return true;
}


UKamoObject* UKamoRuntime::GetObject(const KamoID& id, bool fail_silently) const {
	auto key = id();

	if (!internal_state.Contains(key))
	{
		if (!fail_silently)
		{
			UE_LOG(LogKamoRt, Error, TEXT("UKamoRuntime::GetObject. Object not loaded: %s"), *id());
		}
		return nullptr;
	}

	return internal_state[key];
}


UKamoChildObject* UKamoRuntime::GetObjectFromActor(AActor* actor) const
{
	UObject* ob = Cast<UObject>(actor);

	for (auto& elem : internal_state)
	{
		if (ob == elem.Value->GetObject())
		{
			return Cast<UKamoChildObject>(elem.Value);
		}
	}

	return nullptr;
}


// TODO: Move this function to KamoObject class.
bool UKamoRuntime::EmbedObject(const KamoID& id, const KamoID& container_id, const FString& category)
{
	if (category.IsEmpty())
	{
		UE_LOG(LogKamoRt, Error, TEXT("UKamoRuntime::EmbedObject(): 'category' must be set."));
		return false;
	}

	UE_LOG(
		LogKamoRt, Display,
		TEXT("UKamoRuntime::EmbedObject(): id:%s, container_id: %s, category: %s"),
		*id(), *container_id(), *category
	);

	// Check for availability and type
	UKamoObject* _ob = GetObject(id);
	UKamoActor* ob = Cast<UKamoActor>(_ob);
	UKamoObject* _container = GetObject(container_id);
	UKamoActor* container = Cast<UKamoActor>(_container);

	if (!_ob || !_container || !ob || !container)
	{
		if (!_ob)
			UE_LOG(LogKamoRt, Warning, TEXT("UKamoRuntime::EmbedObject(): 'id' not found."));
		if (!_container)
			UE_LOG(LogKamoRt, Warning, TEXT("UKamoRuntime::EmbedObject(): 'container' not found."));
		if (!ob)
			UE_LOG(LogKamoRt, Error, TEXT("EmbedObject: 'id' must be KamoActor object: %s"), *id());
		if (!container)
			UE_LOG(LogKamoRt, Error, TEXT("EmbedObject: 'container_id' must be KamoActor object: %s"), *container_id());

		return false;
	}

	if (container->embedded_objects.Contains(id()))
	{
		UE_LOG(LogKamoRt, Error, TEXT("EmbedObject: ERROR! 'id' already embeded: %s"), *id());
		return false;
	}

	ob->UpdateKamoStateFromActor();
	FEmbededObject embeded;
	embeded.category = category;
	embeded.kamo_id = id();
	embeded.json_state = *(ob->GetPrimitive()).state;
	container->embedded_objects[embeded.kamo_id] = embeded;
	//ob->Destroy(); // TODO: Make sure this gets deleted from DB else we may get duplicated items.
	container->dirty = true;

	return true;
}


bool UKamoRuntime::ExtractObject(const KamoID& id, const KamoID& container_id)
{
	UE_LOG(
		LogKamoRt, Display,
		TEXT("KamoFileDB::ExtractObject(): id:%s, container_id: %s"),
		*id(), *container_id()
	);



	return false;
}

// DB synch
void UKamoRuntime::MarkForUpdate()
{
	SCOPE_CYCLE_COUNTER(STAT_MarkForUpdate);
	SET_DWORD_STAT(STAT_DirtyObjects, 0);

    for (auto& elem : internal_state)
    {		
        auto object = elem.Value;
		object->MarkForUpdate();

		if (object->dirty)
		{
			INC_DWORD_STAT(STAT_DirtyObjects);
		}
    }
}

void UKamoRuntime::SerializeObjects(bool commit_to_db)
{
	SCOPE_CYCLE_COUNTER(STAT_SerializeObjects);

	TArray<KamoID> deleted_ids;

	for (auto& elem : internal_state)
	{
		auto object = elem.Value;

		if (!object) {
			continue;
		}

		if (object->deleted)
		{
			deleted_ids.Add(object->id->GetPrimitive());
			auto uactor_object = Cast<UKamoActor>(object);
			if (uactor_object && uactor_object->object_ref_mode == EObjectRefMode::RM_SpawnObject && uactor_object->GetActor())
			{
				uactor_object->GetActor()->Destroy();
			}
		}
		else if (object->dirty || object->isNew)
		{
			{
				// Update Kamo state from actor
				SCOPE_CYCLE_COUNTER(STAT_UpdateKamoStateFromActor);
				if (object->GetObject())
				{
					object->UpdateKamoStateFromActor();
					object->dirty = false;
					object->isNew = false;
				}
			}

			{
				// Flush to DB
				SCOPE_CYCLE_COUNTER(STAT_FlushToDB);
				auto child_ptr = Cast<UKamoChildObject>(object);
				auto root_ptr = Cast<UKamoRootObject>(object);
				auto handler_ptr = Cast<UKamoHandlerObject>(object);

				if (child_ptr)
				{
					database->Set(child_ptr->GetPrimitive());
				}
				else if (root_ptr)
				{
					database->Set(root_ptr->GetPrimitive());
				}
				else if (handler_ptr)
				{
					database->Set(handler_ptr->GetPrimitive());
				}
				else
				{
					UE_LOG(LogKamoRt, Error, TEXT("SerializeObjects: Failed to serialize, unknown kamo object type: %s"), *object->id->GetPrimitive()());
					continue;
				}
			}
		}
	}

    // Delete objects
	for (auto deleted_id : deleted_ids)
	{
		SCOPE_CYCLE_COUNTER(STAT_DeleteFromDB);

		UKamoObject* ob = nullptr;	

		if (internal_state.RemoveAndCopyValue(deleted_id(), ob) != 1)
		{
			UE_LOG(LogKamoRt, Error, TEXT("SerializeObjects: Failed to remove entry from internal state: %s"), *deleted_id());
		}
		else
		{
			if (database->CancelIfPending(deleted_id()))
			{
				UE_LOG(LogKamoRt, Verbose, TEXT("SerializeObjects: Object was pending DB write but got deleted: %s."), *deleted_id());
			}
			else if (!ob->isNew)
			{
				bool delete_ok;
				auto child_object = Cast<UKamoChildObject>(ob);
				if (child_object)
				{
					delete_ok = database->DeleteChildObject(child_object->root_id->GetPrimitive(), deleted_id());
				}
				else
				{
					delete_ok = database->DeleteObject(deleted_id());
				}

				if (!delete_ok)
				{
					UE_LOG(LogKamoRt, Error, TEXT("SerializeObjects: Failed to remove object from DB: %s."), *deleted_id());
					UE_CLOG(ob != nullptr, LogKamoRt, Error, TEXT("... Object %s: dirty:%s,  isNew: %s "),
						*ob->id->GetID(), ob->dirty ? TEXT("yes") : TEXT("no"), ob->isNew ? TEXT("yes") : TEXT("no"));
				}
			}
		}
    }

	while (commit_to_db && database->IsSerializationPending(KamoID()))
	{
		FPlatformProcess::Sleep(0.0f);
	}
}


void UKamoRuntime::FlushStateToActors()
{
	SCOPE_CYCLE_COUNTER(STAT_FlushStateToActors);

	for (auto& elem : internal_state)
	{
		auto object = elem.Value;

		if (object->apply_state_now)
		{
			object->apply_state_now = false;
			object->ApplyKamoStateToActor(EKamoStateStage::KSS_ComponentPass);
		}
	}
}


// Loading and unloading regions
TSet<UKamoChildObject*> UKamoRuntime::GetObjectsInRegion(const KamoID& region_id)
{
	TSet<UKamoChildObject*> objects;

	for (auto& elem : internal_state)
	{
		UKamoChildObject* childob = Cast<UKamoChildObject>(elem.Value);
		if (childob && childob->root_id->GetPrimitive() == region_id)
		{
			objects.Add(childob);
		}
	}

	return objects;
}


bool UKamoRuntime::UnloadRegion(const KamoID& region_id)
{
	if (!registered_regions.Contains(region_id))
	{
		UE_LOG(LogKamoRt, Warning, TEXT("UnloadRegion: Region not registered in first place: %s"), *region_id());
		return true;
	}

	SET_DWORD_STAT(STAT_DirtyObjects, 0);
	auto objects = GetObjectsInRegion(region_id);
	for (auto ob : objects)
	{
		ob->MarkForUpdate();
		if (ob->dirty)
		{
			INC_DWORD_STAT(STAT_DirtyObjects);
		}
	}
	
	SerializeObjects(true);  // TODO: Only serialize objects of this particular region. 

	// Remove the objects
	for (auto ob : objects)
	{
		if (internal_state.Remove(ob->id->GetID()) != 1)
		{
			UE_LOG(LogKamoRt, Warning, TEXT("UnloadRegion: Failed to remove object from internal state: %s"), *ob->id->GetID());
		}
	}

	registered_regions.Remove(region_id);
	auto empty_handler = KamoID();
	bool ok = SetHandler(region_id, empty_handler);
	return ok;
}


// Handler
bool UKamoRuntime::RegisterUE4Handler(UE4ServerHandler& ue4handler)
{
	message_queue = IKamoMQ::CreateDriver(KamoUtil::get_driver_name());
	if (message_queue)
	{
		message_queue->CreateSession(KamoUtil::get_tenant_name(), ue4handler.id());
		message_queue->CreateMessageQueue();
		ue4handler.inbox_address = message_queue->GetSessionURL();
	}

    return database->AddHandlerObject(ue4handler);
}

bool UKamoRuntime::UnregisterUE4Server(const KamoID& handler_id) {

	auto tmp = registered_regions;
	for (auto region_id : tmp)
	{
		UnloadRegion(region_id);
	}

	message_queue->DeleteMessageQueue();
	return database->DeleteHandlerObject(handler_id);
}

bool UKamoRuntime::SetHandler(const KamoID& root_id, const KamoID& handler_id)
{
    auto success = database->SetHandler(root_id, handler_id);

    if (!success)
	{
        UE_LOG(LogKamoRt, Error, TEXT("FAILED TO SET HANDLER '%s' FOR ROOT OBJECT '%s'"), *handler_id(), *root_id());
    }

    return success;
}

UUE4ServerHandler* UKamoRuntime::GetUE4Server(const KamoID& handler_id)
{
	if (handler_id.class_name != "ue4server")
	{
		UE_LOG(LogKamoRt, Error, TEXT("GetUE4Server: Handler must be 'ue4server' but is: %s"), *handler_id());
		return nullptr;
	}

	auto ue4handler = database->GetHandlerInfo(handler_id);

    if (ue4handler.IsEmpty())
	{
        return nullptr;
    }

	// HACK: This polymorphism isn't working. Explicitly assigning works
	// but this must be refactored to look good.
	UE4ServerHandler ue4;
	ue4.id = ue4handler.id;
	ue4.state = ue4handler.state;
	ue4.inbox_address = ue4handler.inbox_address;

	// Fetch next three fields from the state
	TSharedPtr<FJsonObject> json_object;
	TSharedRef< TJsonReader<> > json_reader = TJsonReaderFactory<>::Create(*ue4.state);
	if (!FJsonSerializer::Deserialize(json_reader, json_object))
	{
		UE_LOG(LogKamoRt, Error, TEXT("GetUE4Server: Can't parse json: %s"), *ue4.state);
		return nullptr;
	}
	ue4.ip_address = json_object->GetStringField("ip_address");
	ue4.port = json_object->GetIntegerField("port");
	ue4.map_name = json_object->GetStringField("map_name");
	auto ue4_handler = NewObject<UUE4ServerHandler>();
	ue4_handler->Init(ue4);

	// Read in the rest
	FDateTime::ParseIso8601(*json_object->GetStringField("start_time"), ue4_handler->StartTime);

	// Figure out 'seconds_from_last_heartbeat' using 'last_refresh'.
	if (FDateTime::ParseIso8601(*json_object->GetStringField("last_refresh"), ue4_handler->LastRefresh))
	{
		ue4_handler->seconds_from_last_heartbeat = (FDateTime::UtcNow() - ue4_handler->LastRefresh).GetTotalSeconds();
	}

    return ue4_handler;
}

UUE4ServerHandler* UKamoRuntime::GetUE4ServerForRoot(const KamoID& id) {
	auto object = database->GetRootObject(id);

	if (object.handler_id.IsEmpty()) {
		return nullptr;
	}

	return GetUE4Server(object.handler_id);
}

UUE4ServerHandler* UKamoRuntime::GetUE4ServerForChild(const KamoID& id) {
	auto object = database->GetObject(id);

	if (object.root_id.IsEmpty()) {
		return nullptr;
	}

	return GetUE4ServerForRoot(object.root_id);
}


// Message Queue
void UKamoRuntime::ProcessMessageQueue()
{
	SCOPE_CYCLE_COUNTER(STAT_ProcessMessageQueue);

	if (server_id.IsEmpty() || !database.IsValid() || !message_queue.IsValid())
	{
		UE_LOG(LogKamoRt, Error, TEXT("ProcessMessageQueue: Not initialized."));
	}
	else
	{
		KamoMessage message;
		while (message_queue->ReceiveMessage(message))
		{
			ProcessMessage(message);
		}
	}
}


bool UKamoRuntime::SendMessage(const KamoID& handler_id, const FString& message_type, const FString& payload) {
    auto ue4handler = database->GetHandlerInfo(handler_id);

    return message_queue->SendMessage(ue4handler.inbox_address, message_type, payload);
}


void UKamoRuntime::ProcessMessage(const KamoMessage& message) 
{
	if (message.message_type != "command")
	{
		// Never happens actually
		return;
	}

    TArray<UKamoState*> command_states;

	UKamoMessage* umessage = NewObject<UKamoMessage>();
	umessage->Init(message);
    if (!umessage->payload->GetKamoStateArray("commands", command_states))
	{
        UE_LOG(LogKamoRt, Warning, TEXT("'commands' NOT FOUND"));
        return;
    }

    for (auto command_state : command_states)
	{
        FString command;
        FString kamo_id;
		FString region_id;
		UKamoState* parameters;

        const auto command_valid = command_state->GetString("command", command);
        const auto kamo_id_valid = command_state->GetString("kamo_id", kamo_id);
        const auto region_id_valid = command_state->GetString("region_id", region_id);
		const auto parameters_valid = command_state->GetKamoState("parameters", parameters);

        if (!command_valid)
		{
            UE_LOG(LogKamoRt, Warning, TEXT("'command' NOT FOUND"));
            continue;
        }

        UE_LOG(LogKamoRt, Display, TEXT("Processing command: %s"), *command);

		if (command == "load_childobject_from_db")
		{
			if (!kamo_id_valid) {
				UE_LOG(LogKamoRt, Warning, TEXT("'kamo_id' NOT FOUND"));
				continue;
			}

			if (!region_id_valid) {
				UE_LOG(LogKamoRt, Warning, TEXT("'region_id' NOT set"));
				continue;
			}

			auto id = KamoID(kamo_id);
			KamoID root_id(region_id);
			LoadChildObjectFromDB(id, &root_id);
			continue;
		}
		else if (command == "move_object")
		{
			if (!kamo_id_valid) {
				UE_LOG(LogKamoRt, Warning, TEXT("'kamo_id' NOT FOUND"));
				continue;
			}

			if (!parameters)
			{
				UE_LOG(LogKamoRt, Warning, TEXT("'parameters' NOT FOUND"));
				continue;
			}

			FString to;
			if (!parameters->GetString("to", to))
			{
				UE_LOG(LogKamoRt, Warning, TEXT("'to' PARAMETER NOT FOUND"));
				continue;
			}

			FString spawn_target = TEXT("ignore");
			parameters->GetString("spawn_target", spawn_target);
			MoveObject(KamoID(kamo_id), KamoID(to), spawn_target);
			continue;
		}

		if (region_id_valid && kamo_id_valid)
		{
			auto ob  = GetObject(kamo_id);
			if (ob == nullptr)
			{
				UE_LOG(LogKamoRt, Error, TEXT("'kamo_id' NOT FOUND in internal state"));
				continue;
			}
			if (ob->OnCommandReceived(command, parameters))
			{
				// Command processed succesfully.
				continue;
			}
		}
        else if (command == "apply_state")
		{
			auto id = KamoID(kamo_id);
			auto ob = GetObject(id);
			if (ob == nullptr)
			{
				UE_LOG(LogKamoRt, Error, TEXT("Command 'apply_state': 'kamo_id' parameter not found."));
				continue;
			}

			if (!parameters)
			{
				UE_LOG(LogKamoRt, Warning, TEXT("Command 'apply_state': 'parameters' not found"));
				continue;
			}

			UKamoState* state;
			if (!parameters->GetKamoState("state", state))
			{
				UE_LOG(LogKamoRt, Error, TEXT("Command 'apply_state': 'state' parameter not found. id: %s"), *id());
				continue;
			}

			if (!OverwriteObjectState(kamo_id, state->GetStateAsString()))
			{
				UE_LOG(LogKamoRt, Error, TEXT("Command 'apply_state': Failed to override object state, id: %s"), *id());
				continue;
			}

			ob->ApplyKamoStateToActor(EKamoStateStage::KSS_ActorPass);
			ob->ApplyKamoStateToActor(EKamoStateStage::KSS_ComponentPass);
        }

		else if (command == "flash")
		{
			auto id = KamoID(kamo_id);
			auto ob = GetObject(id);
			if (ob == nullptr)
			{
				UE_LOG(LogKamoRt, Warning, TEXT("Command 'flash': 'kamo_id' parameter not found."));
				continue;
			}
			auto kamo_actor = Cast <UKamoActor>(ob);
			if (kamo_actor == nullptr)
			{
				UE_LOG(LogKamoRt, Warning, TEXT("Command 'flash': Kamo object is not Actor."));
				continue;
			}
			// Note, let's ignore the fact that this is a flash command
			kamo_actor->OnCommandReceived(command, parameters);
		}
		else if (command == "delete_object")
		{
			auto id = KamoID(kamo_id);
			auto ob = GetObject(id);
			if (ob == nullptr)
			{
				UE_LOG(LogKamoRt, Warning, TEXT("Command 'delete_object': 'kamo_id' parameter not found."));
				continue;
			}
			ob->deleted = true;
		}
		else if (command == "flush_to_db")
		{
			SerializeObjects();
		}
		else if (command == "exit_process")
		{
            FGenericPlatformMisc::RequestExit(false);
        }
		else if (command == "log" && parameters_valid)
		{
			FString msg = "(none)";
			if (parameters && parameters_valid)
			{
				parameters->GetString("message", msg);
			}
			UE_LOG(LogKamoRt, Display, TEXT("Log message from %s: %s"), *umessage->sender->GetID(), *msg);
		}
		else
		{
			UE_LOG(LogKamoRt, Warning, TEXT("Command '%s' not implemented."), *command);
		}
    }
}


// World


UKamoObject* UKamoRuntime::RegisterKamoObject(const KamoID& id, const KamoID& root_id, UKamoState* state, UObject* object, bool is_proxy, bool skip_apply_state)
{

	auto skip_refresh = false;
    auto should_spawn = true;

    if (!kamo_table)
    {
 		UE_LOG(LogKamoRt, Error, TEXT("No kamo table"));
		return nullptr;
    }

    FString context_str;
    FKamoClassMap* kamo_class_entry = kamo_table->FindRow<FKamoClassMap>(FName(*id.class_name), context_str);

	if (!kamo_class_entry)
	{
		if (object && object->Implements<UKamoPersistable>())
		{
			kamo_class_entry = GetDefaultKamoClassMapEntry().Value;
		} else if (id.class_name.Equals(TEXT("generic")))
		{
			kamo_class_entry = GetDefaultKamoClassMapEntry().Value;
		} else
		{
			UE_LOG(LogKamoRt, Warning, TEXT("No class entry configured for kamo entry: \"%s.%s\""), *id.class_name, *id.unique_id);
			return nullptr;
		}
    }
	else if (!kamo_class_entry->kamo_class)
	{
		UE_LOG(LogKamoRt, Error, TEXT("No kamo class configured for kamo entry: \"%s.%s\""), *id.class_name, *id.unique_id);
		return nullptr;
	}

	auto actor = Cast<AActor>(object);

    FString actor_class;
    if (!state->GetString(TEXT("mmo_actor_class"), actor_class)) {
        if (actor) {
            skip_refresh = true;
        }
        else {
            should_spawn = true;
        }
    }

	// Create an instance of KamoObject and assign an ID to it
    auto uobject = NewObject<UKamoObject>(GetWorld(), kamo_class_entry->kamo_class);
	auto uid = NewObject<UKamoID>(GetWorld());
	uid->Init(id);

	// Applying the data to the newly created kamo object
	uobject->id = uid;
	uobject->state = state;
	uobject->SetObject(object);
	// uobject->OnObjectAttached(object);
	uobject->check_if_dirty = kamo_class_entry->check_if_dirty; // TODO: Use CDO instead
	uobject->kamo_class_entry = *kamo_class_entry;


	auto uchild_object = Cast<UKamoChildObject>(uobject);

    if (uchild_object) {
		auto uroot_id = NewObject<UKamoID>(GetWorld());
		uroot_id->Init(root_id);

		uchild_object->root_id = uroot_id;
    }

	uobject->is_proxy = is_proxy;
	if (is_proxy)
	{
		internal_state.Add(id(), uobject);
		uobject->ResolveSubobjects(this);
		return uobject;
	}

	auto ukamoactor_object = Cast <UKamoActor>(uobject);
	if (ukamoactor_object)
	{
		if (!skip_apply_state)
		{
			// Allow KamoActor to initialize its core state from Kamo object.
			ukamoactor_object->PreProcessState();
		}

		should_spawn = ukamoactor_object->object_ref_mode == EObjectRefMode::RM_SpawnObject;
	}

	// Check if the kamo object is a kamo actor
	auto uactor_object = Cast<UKamoActor>(uobject);
	bool finish_spawning_actor = false;
	FTransform transform = FTransform::Identity;

	if (uactor_object && actor)
	{
		uactor_object->SetObject(actor);
		uactor_object->OnObjectSet(actor, this);
		uactor_object->actor_is_set = true;
	}
    else if (should_spawn && uactor_object)
	{
    	state->GetTransform(TEXT("transform"), transform);
		finish_spawning_actor = SpawnActorForKamoObject(uactor_object, &transform, false);
    }

    internal_state.Add(id(), uobject);

	// Applies the state from the kamo object to the created object
    if (!skip_refresh)
	{
		if (!skip_apply_state && uobject->GetObject())
		{
			uobject->ApplyKamoStateToActor(EKamoStateStage::KSS_ActorPass);
			uobject->apply_state_now = true;
		}
    }

	if (finish_spawning_actor)
	{
		FinalizeSpawnActorForKamoObject(uactor_object, &transform);
	}


	// Notify the kamo object that it has been registered
	uobject->OnRegistered();

	return uobject;
}


bool UKamoRuntime::SpawnActorForKamoObject(UKamoActor* kamo_object, FTransform* transform, bool finalize)
{
	if (kamo_object->GetObject())
	{
		UE_LOG(LogKamoRt, Error, TEXT("SpawnActorForKamoObject failed, actor already set: %s"), *kamo_object->GetObject()->GetName());
		return false;
	}

	FTransform tf = FTransform::Identity;
	if (!transform)
	{
		transform = &tf;
		kamo_object->state->GetTransform(TEXT("transform"), *transform);
	}

	bool finish_spawning_actor = false;
	runtime_is_spawning = true;
	auto spawn_class = kamo_object->kamo_class_entry.actor_class;
	FString ue4_class_path;
	if (kamo_object->state->GetString(TEXT("ue4_class"), ue4_class_path))
	{
		if (auto loaded_class = LoadClass<AActor>(nullptr, *ue4_class_path))
		{
			spawn_class = loaded_class;
		}
		else
		{
			UE_LOG(LogKamoRt, Warning, TEXT("Failed to load actor class from [%s]"), *ue4_class_path);
		}
	}

	if (auto spawned = GetWorld()->SpawnActorDeferred<AActor>(spawn_class, *transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
	{
		kamo_object->SetObject(spawned);
		kamo_object->actor_is_set = true;
		finish_spawning_actor = true;
	}
	else
	{
		UE_LOG(LogKamoRt, Warning, TEXT("Failed to spawn actor from [%s]"), *spawn_class->GetName());
	}

	runtime_is_spawning = false;

	if (!finish_spawning_actor)
	{
		return false;
	}

	if (finalize)
	{
		kamo_object->ApplyKamoStateToActor(EKamoStateStage::KSS_ActorPass);
		kamo_object->apply_state_now = true;
		FinalizeSpawnActorForKamoObject(kamo_object, transform);
	}

	return true;
}


void UKamoRuntime::FinalizeSpawnActorForKamoObject(UKamoActor* kamo_object, FTransform* transform)
{
	UGameplayStatics::FinishSpawningActor(kamo_object->GetActor(), *transform);
	kamo_object->OnObjectSet(kamo_object->GetObject(), this);
}

void UKamoRuntime::OnActorSpawned(AActor* actor)
{
	SCOPE_CYCLE_COUNTER(STAT_OnActorSpawned);
	if (!runtime_is_spawning)
	{
		UKamoActor::ProcessOnActorSpawned(actor, this);
	}
}

// Utilities

FString UKamoRuntime::GetCommandLineArgument(const FString& argument, const FString& default_value)
{
	auto complete_argument = "-" + argument;

	FString command_line = FCommandLine::Get();

	TArray<FString> args;
	FString delim = " ";

	command_line.ParseIntoArray(args, *delim);

	FString value_delim = "=";

	for (auto arg : args) {
		FString left;
		FString right;

		if (!arg.Split(value_delim, &left, &right) || left != complete_argument) {
			continue;
		}

		return right;
	}

	return default_value;
}



UKamoID* UKamoRuntime::GetKamoIDFromString(const FString& kamo_id) {
	auto id = KamoID(kamo_id);

	auto uid = NewObject<UKamoID>();
	uid->Init(id);

	return uid;
}


UDataTable* UKamoRuntime::GetKamoTable()
{
	// Data table is usually defined in project settings but can be overridden using the 'kamoclassmap' command line argument.

	UDataTable* data_table = nullptr;
	FString class_map_path = GetCommandLineArgument(TEXT("kamoclassmap"));
	if (!class_map_path.IsEmpty())
	{
		data_table = LoadObject<UDataTable>(NULL, *FString::Printf(TEXT("DataTable'%s'"), *class_map_path));
	}
	else
	{ 
		data_table = Cast<UDataTable>(UKamoProjectSettings::Get()->kamo_table.TryLoad());
	}

	if (!data_table)
	{
		UE_LOG(LogKamoRt, Error, TEXT("KamoRuntime::GetKamoTable - No class map specified in project settings for Kamo."));
		if (IsRunningDedicatedServer())
		{
			UE_LOG(LogKamoRt, Error, TEXT("Dedicated server can't continue without class map, shutting down!"));
			FGenericPlatformMisc::RequestExit(false);
		}
	}
	else if (!data_table->GetRowStruct()->IsChildOf(FKamoClassMap::StaticStruct()))
	{
#if WITH_EDITORONLY_DATA
		UE_LOG(LogKamoRt, Warning, TEXT(
			"KamoRuntime::Init - Expected class map data table with row structure 'FKamoClassMap' but got '%s'."),
			*data_table->GetRowStructName().ToString()
		);
#else
		UE_LOG(LogKamoRt, Warning, TEXT("Init: Expected class map data table with row structure 'FKamoClassMap'."));
#endif
		data_table = NewObject<UDataTable>();
	}
	else
	{
		UE_LOG(LogKamoRt, Display, TEXT("KamoRuntime::Init - Found %i kamo classes in the class table"), data_table->GetRowNames().Num());
	}

	return data_table;
}


TKamoClassMapEntry UKamoRuntime::GetDefaultKamoClassMapEntry() const
{
	static FKamoClassMap KamoClassMap;
	KamoClassMap.actor_class = AActor::StaticClass();
	KamoClassMap.kamo_class = UKamoActor::StaticClass();
	KamoClassMap.check_if_dirty = true;
	return TKamoClassMapEntry("generic", &KamoClassMap);
}

TKamoClassMapEntry UKamoRuntime::GetKamoClassEntryFromActor(AActor* actor) const 
{	
	FName bestRowName = NAME_None;
	FKamoClassMap* bestEntry = nullptr;
	auto bestClassDistance = INT_MAX;
	const auto actor_class = actor->GetClass();
	const auto row_names = kamo_table->GetRowNames();	
	
	for (auto row_name : row_names) 
	{
		static const FString context(TEXT("KamoRuntime"));
		auto entry = kamo_table->FindRow<FKamoClassMap>(row_name, context);

		if (entry->actor_class == actor_class) 
		{
			// exact match
			bestRowName = row_name;
			bestEntry = entry;
			break;
		}

		if (!actor_class->IsChildOf(entry->actor_class))
		{
			// ignore if there's no relationship
			continue;
		}
		
		auto classDistance = 0;
		auto currentClass = actor_class;
		while (currentClass && currentClass != entry->actor_class)
		{
			++classDistance;
			currentClass = currentClass->GetSuperClass();
		}

		if (classDistance < bestClassDistance)
		{
			bestClassDistance = classDistance;
			bestRowName = row_name;
			bestEntry = entry;
		}
	}

	if (bestEntry)
	{
		UE_LOG(LogKamoRt, Verbose, TEXT("Class entry found for actor class [%s]: [%s]"), 
			*actor_class->GetName(), *bestRowName.ToString());
		return TKamoClassMapEntry(bestRowName.ToString().ToLower(), bestEntry);
	}
	else
	{
		if (actor->Implements<UKamoPersistable>())
		{
			UE_LOG(LogKamoRt, Display, TEXT("No class entry found but actor class %s but it does implement IKamoPersistable"), *actor->StaticClass()->GetName());
			return GetDefaultKamoClassMapEntry();
		}
		return {};
	}
}

FKamoClassMap UKamoRuntime::GetKamoClass(const FString& class_name) const {
	FString context_str;
	auto entry = kamo_table->FindRow<FKamoClassMap>(FName(*class_name), context_str);

	if (!entry) {
		return FKamoClassMap();
	}

	return *entry;
}

UKamoState* UKamoRuntime::CreateMessageCommand(const KamoID& kamo_id, const KamoID& root_id, const FString& command, const FString& parameters) {
	auto command_object = NewObject<UKamoState>();
	auto params = NewObject<UKamoState>();

	params->SetState(parameters);

	command_object->SetString("command", command);
	if (!kamo_id.IsEmpty())
	{
		command_object->SetString("kamo_id", kamo_id());
	}
	if (!root_id.IsEmpty())
	{
		command_object->SetString("region_id", root_id());  // Note: 'root_id' was renamed to 'region_id'.
	}
	command_object->SetKamoState("parameters", params);

	return command_object;
}

UKamoState* UKamoRuntime::CreateCommandMessagePayload(TArray<UKamoState*> commands) {
	auto payload = NewObject<UKamoState>();

	payload->SetKamoStateArray("commands", commands);

	return payload;
}

bool UKamoRuntime::SendCommandToObject(const KamoID& id, const FString& command, const FString& parameters) {
	KamoID root_id = id;
	KamoID message_root_id;

	auto child_object = database->GetObject(id);

	if (!child_object.IsEmpty()) {
		root_id = child_object.root_id;
		message_root_id = child_object.root_id;
	}

	auto root_object = database->GetRootObject(root_id);

	if (root_object.IsEmpty()) {
		return false;
	}

	auto command_object = CreateMessageCommand(id, message_root_id, command, parameters);

	TArray<UKamoState*> commands;
	commands.Add(command_object);

	auto payload = CreateCommandMessagePayload(commands);

	return SendMessage(root_object.handler_id, "command", payload->GetStateAsString());
}

bool UKamoRuntime::SendCommandsToObject(const KamoID& id, TArray<UKamoState*> commands) {
	KamoID root_id = id;

	auto child_object = database->GetObject(id);

	if (!child_object.IsEmpty()) {
		root_id = child_object.root_id;
	}

	auto root_object = database->GetRootObject(root_id);

	if (root_object.IsEmpty()) {
		return false;
	}

	auto payload = CreateCommandMessagePayload(commands);

	return SendMessage(root_object.handler_id, "command", payload->GetStateAsString());
}


bool UKamoRuntime::AddIdentityActor(const FString& InUniqueId, AActor* InActor)
{
	check(InActor);
	TKamoClassMapEntry KamoClassInfo = GetKamoClassEntryFromActor(InActor);
	if (KamoClassInfo.Key.IsEmpty())
	{
		UE_LOG(LogKamoRt, Warning, TEXT("AddIdentityActor: No Kamo class defined for %s. UniqueId: %s"), *InActor->GetName(), *InUniqueId);
		return false;
	}

	KamoID Id;
	Id.class_name = KamoClassInfo.Key;
	Id.unique_id = InUniqueId;

	if (IdentityActors.Contains(Id))
	{
		UE_LOG(LogKamoRt, Warning, TEXT("AddAttachCandidate: Candidate already exists for %s"), *Id());
		return false;
	}
	else
	{
		FIdentityActor& IdentityActor = IdentityActors.Add(Id);
		IdentityActor.Id = Id;
		IdentityActor.KamoClassInfo = KamoClassInfo;
		IdentityActor.Actor = InActor;
		IdentityActor.KamoRuntime = this;
		bool bIgnore = false;
		IdentityActor.AttachIdentityActor(bIgnore);
		return true;
	}
}


void UKamoRuntime::TickIdentityActors(float DeltaTime)
{
	TArray<KamoID> ToRemove;

	for (auto It = IdentityActors.CreateIterator(); It; ++It)
	{
		bool bRemoveEntry = false;
		It.Value().Tick(DeltaTime, bRemoveEntry);
		if (bRemoveEntry)
		{
			ToRemove.Add(It.Key());
		}
	}

	for (const KamoID& Id : ToRemove)
	{
		IdentityActors.Remove(Id);
	}
}


void FIdentityActor::Tick(float DeltaTime, bool& bOutRemoveEntry)
{
	TimeElapsedWaiting += DeltaTime;
	AttachIdentityActor(bOutRemoveEntry);
	if (bOutRemoveEntry)
	{
		return;
	}
	
	if (!bIsAttached)
	{
		// Try load from DB
		KamoID RootId;
		EKamoLoadChildObject result = KamoRuntime->LoadChildObjectFromDB(Id, &RootId);

		if (result == EKamoLoadChildObject::Success)
		{
			// Object was found and loaded, now we simply attach.
			AttachIdentityActor(bOutRemoveEntry);
		}
		else if (result == EKamoLoadChildObject::NotOurRegion)
		{
			// The object does exist in the DB but is not in our region.

			// If we are running in PIE we do a courtesy move.
			KamoRuntime->MoveIdentityObjectIfPIE(Id, RootId);

			//  We will continue to check for the object periodically and attach if it happens to pop into our region.
				
			if (TimeElapsedWaiting > Timeout)
			{
				// Remove the identity actor record and notify a timeout
				bOutRemoveEntry = true;
				//todo: identity_event_error.Broadcast(this, result);
			}
		}
		else if (result == EKamoLoadChildObject::RegisterFailed)
		{
			// Possibly an unrecoverable error.
			bOutRemoveEntry = true;
			//todo: identity_event_error.Broadcast(this, result);
		}
		else if (result == EKamoLoadChildObject::NotFound)
		{
			UKamoActor* KamoActorCDO = Cast<UKamoActor>(KamoClassInfo.Value->kamo_class.GetDefaultObject());
			KamoActorCDO->OnKamoObjectNotFoundCDO(*this);
		}
	}
}


void FIdentityActor::AttachIdentityActor(bool& bOutRemoveEntry)
{
	if (bIsAttached)
	{
		// Check if we have been detached
		if (!Actor.IsValid() || KamoRuntime->GetObjectFromActor(Actor.Get()) == nullptr)
		{
			bIsAttached = false;
			KamoActor->OnKamoInstanceDetached();
			KamoActor.Reset();
		}
	}
	else
	{
		if (!Actor.IsValid())
		{
			UE_LOG(LogKamoRt, Warning, TEXT("AttachIdentityActor: Actor instance is/has been destroyed for object: %s"), *Id());
			bOutRemoveEntry = true;
		}
		else
		{
			UE_CLOG(KamoActor.IsValid(), LogKamoRt, Warning, TEXT("AttachIdentityActor: Already have KamoActor attached."));

			KamoActor = Cast<UKamoActor>(KamoRuntime->GetObject(Id, /*fail_silently*/true));
			if (KamoActor.IsValid())
			{
				bIsAttached = true;
				UE_CLOG(KamoActor->GetObject() != nullptr, LogKamoRt, Error,
					TEXT("AttachIdentityActor: Already have actor attached: %s. New Actor: %s"), *KamoActor->GetObject()->GetName(), *Actor.Get()->GetName()
				);


				// todo: This chunk below should be in KamoRuntime
				KamoActor->SetObject(Actor.Get());
				KamoActor->OnObjectSet(Actor.Get(), KamoRuntime.Get());
				KamoActor->actor_is_set = true;
				KamoActor->ApplyKamoStateToActor(EKamoStateStage::KSS_ActorPass);
				KamoActor->ApplyKamoStateToActor(EKamoStateStage::KSS_ComponentPass);
				KamoActor->ResolveSubobjects(KamoRuntime.Get());
				KamoActor->OnKamoInstanceAttached();
				UE_LOG(LogKamoRt, Display, TEXT("AttachIdentityActor: Actor %s attached to Kamo object: %s"), *Actor.Get()->GetName(), *Id());
			}
		}
	}
}


void FIdentityActor::CreateKamoObject()
{
	check(!bIsAttached);

	KamoID RegionId(KamoRuntime->MapActorToRegion(Actor.Get()));
	KamoActor = Cast<UKamoActor>(KamoRuntime->CreateObject(Id.class_name, RegionId, Actor.Get(), Id.unique_id));

	// todo: see if and why we do this attachment here instead of letting AttachIdentityActor do the work!!!
	if (KamoActor.IsValid())
	{
		bIsAttached = true;
		KamoActor->OnKamoInstanceAttached();
	}
}


void UKamoRuntime::MoveIdentityObjectIfPIE(const KamoID & InObjectId, const KamoID & InRootId)
{
#if WITH_EDITOR
	if (!bPIEMoveDone && GetWorld()->IsPlayInEditor() && UKamoProjectSettings::Get()->DoCourtesyMoveInPIE)
	{
		bPIEMoveDone = true;
		APlayerController* PC = GetWorld()->GetFirstPlayerController();

		// Move the object to a convenient location		
		FVector Location(0.0);
		if (PC)
		{
			Location = PC->GetSpawnLocation();
		}
		else if(AActor* PlayerStart = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()))
		{
			Location = PlayerStart->GetActorLocation();
		}

		FString RegionId = MapLocationToRegion(Location);
		KamoID TargetLocationId(RegionId);

		MoveObjectSafely(InObjectId, RegionId, "playerstart");
		/*
		// Re-attach to player controller
		if (PC)
		{
			AddIdentityActor(InObjectId.unique_id, PC);
		}
		*/
	}
#endif
}
