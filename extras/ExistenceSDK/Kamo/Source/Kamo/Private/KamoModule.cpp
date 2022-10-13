// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#include "KamoModule.h"
#include "Kamo.h"
#include "KamoActor.h"
#include "KamoSettings.h"

#if WITH_EDITOR
#include "Editor.h" // for FEditorDelegates
#include "AssetRegistryModule.h"
#include "LevelEditor.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "EditorUtilityWidget.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#endif

#include "Misc/CoreMisc.h"

DEFINE_LOG_CATEGORY(LogKamoModule);


FKamoModule::FKamoModule() :
	is_initialized(false)
{
}


FKamoModule::~FKamoModule()
{
}


void FKamoModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	const FString Commandline = FCommandLine::Get();
	const auto bIsCookCommandlet = Commandline.Contains(TEXT("cookcommandlet")) || Commandline.Contains(TEXT("run=cook"));
	if (!bIsCookCommandlet)
	{
		// Add all the event delegates to handle game start, end and level change
		FCoreDelegates::OnPostEngineInit.AddRaw(this, &FKamoModule::OnPostEngineInit);
		WorldLoadEventBinding = FCoreUObjectDelegates::PostLoadMapWithWorld.AddRaw(this, &FKamoModule::OnPostLoadMapWithWorld);
		WorldBeginTearDownBinding = FWorldDelegates::OnWorldBeginTearDown.AddRaw(this, &FKamoModule::OnWorldBeginTearDown);

		// Receive event when Kamo runtime is created
		KamoRuntimeEventBinding = UKamoRuntime::OnKamoRuntime().AddRaw(this, &FKamoModule::OnKamoRuntime);

#if WITH_EDITOR
		if (!IsRunningDedicatedServer())
		{
			TickDelegate = FTickerDelegate::CreateRaw(this, &FKamoModule::Tick);
			TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(TickDelegate);

			// Bind events on PIE start/end to open/close camera
			PieBeginEventBinding = FEditorDelegates::BeginPIE.AddRaw(this, &FKamoModule::OnPieBegin);
			PieStartedEventBinding = FEditorDelegates::PostPIEStarted.AddRaw(this, &FKamoModule::OnPieStarted);
			PieEndedEventBinding = FEditorDelegates::PrePIEEnded.AddRaw(this, &FKamoModule::OnPieEnded);

			// Load the asset registry module to listen for updates
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
			AssetRegistryModule.Get().OnAssetRenamed().AddRaw(this, &FKamoModule::OnAssetRenamed);
			AssetRegistryModule.Get().OnAssetRemoved().AddRaw(this, &FKamoModule::OnAssetRemoved);

			FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		}
#endif

		//UE_LOG(LogKamoModule, Verbose, TEXT("Kamo module hooked up to world events."));
		is_initialized = true;
	}

	FCoreUObjectDelegates::GetPreGarbageCollectDelegate().AddRaw(this, &FKamoModule::OnPreGarbageCollection);
	FCoreUObjectDelegates::GetPostGarbageCollect().AddRaw(this, &FKamoModule::OnPostGarbageCollection);
}
#if WITH_EDITOR
bool FKamoModule::Tick(float DeltaTime)
{
	/*
	* Okay, this one is a bit of a doozy... We are using the Python Scripting Plugin in the editor
	* but it has a problem. It is initialized very late, or in its first Tick and this has the side effect
	* that any python blueprint function calls in EditorUtilityWidgets are invalid every time the editor
	* starts up.
	* The workaround here is to wait a couple of ticks (not the first tick since the Python plugin might not have
	* gotten its Tick yet) and then refresh our editor widget, which fixes up the connections.
	* This is bad, and I am ashamed for this code. Hopefully Epic will fix their Python plugin Initialization
	*/
	numTicks++;
	if (numTicks >= 2) {
		for (const FString& editorWidget : GetDefault<UKamoProjectSettings>()->editorWidgets)
		{
			FString blueprintName(editorWidget);
			//UE_LOG(LogKamoModule, Warning, TEXT("Refreshing connections in Editor Widget %s"), *blueprintName);
			UBlueprint* bp = FindObject<UBlueprint>(nullptr, *blueprintName, false);
			if (bp != nullptr) {
				FBlueprintEditorUtils::RefreshAllNodes(bp);
				FKismetEditorUtilities::CompileBlueprint(bp);
			}
		}
		FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
	}
	return true;
}

#endif 

void FKamoModule::ShutdownModule()
{
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
	FCoreUObjectDelegates::GetPreGarbageCollectDelegate().RemoveAll(this);
	FCoreUObjectDelegates::GetPostGarbageCollect().RemoveAll(this);

	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	if (is_initialized)
	{
		is_initialized = false;

		for (auto& elem : runtime_map)
		{
			// TODO: The runtime object is destroyed by the engine so we can't do this here. Check if there's
			// any case we actually need to handle here.
			// elem.Value->ShutdownRuntime();
		}

		runtime_map.Empty();

		if (GEngine)
		{
			GEngine->OnWorldAdded().Remove(WorldAddedEventBinding);
			GEngine->OnWorldDestroyed().Remove(WorldDestroyedEventBinding);
			FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(WorldLoadEventBinding);
			FWorldDelegates::OnWorldBeginTearDown.Remove(WorldBeginTearDownBinding);

#if WITH_EDITOR
			FEditorDelegates::PostPIEStarted.Remove(PieStartedEventBinding);
			FEditorDelegates::PrePIEEnded.Remove(PieEndedEventBinding);
#endif
		}
	}
}


UKamoRuntime* FKamoModule::GetKamoRuntime(UWorld* world)
{
	return world->GetSubsystem<UKamoRuntime>();

	if (!world || !world->IsValidLowLevel())
	{
		UE_LOG(LogKamoModule, Error, TEXT("GetKamoRuntime called with invalid world"));
		return nullptr;
	}

	auto Settings = UKamoProjectSettings::Get();

	if (runtime_map.Contains(world))
	{
		return runtime_map[world];
	}
	else if (Settings->jit_create_runtime)
	{
		if (bIsCollectingGarbage)
		{
			UE_LOG(LogKamoModule, Warning, TEXT("GetKamoRuntime: JIT runtime creation is disabled during garbage collection."));
			return nullptr;
		}
		else
		{
			return CreateRuntime(world);
		}		
	}
	else
	{
		UE_LOG(LogKamoModule, Warning, TEXT("GetKamoRuntime: CreateRuntime has not yet been called for world: %s."), *world->GetFullName());
		return nullptr;
	}
}


void FKamoModule::OnWorldCreated(UWorld* world)
{
	//UE_LOG(LogKamoModule, Display, TEXT("OnWorldCreated: world: %s"), *world->GetFullName());
	ProcessNewWorld(world);
}


void FKamoModule::OnPostLoadMapWithWorld(UWorld* world)
{
	if (world)
	{
		if (UKamoRuntime* KamoRuntime = world->GetSubsystem<UKamoRuntime>())
		{
			//KamoRuntime->OnPostLoadMapWithWorld();
		}
		//UE_LOG(LogKamoModule, Display, TEXT("OnPostLoadMapWithWorld: world: %s"), *world->GetFullName());
		//ProcessNewWorld(world);
	}
}


void FKamoModule::ProcessNewWorld(UWorld* world)
{
#if WITH_EDITORONLY_DATA
	const bool bIsGameInst = !IsRunningCommandlet() && world->IsGameWorld();
	if (bIsGameInst)
#endif
	{
		CreateRuntime(world);
	}
}


UKamoRuntime* FKamoModule::CreateRuntime(UWorld* world)
{
	return nullptr;

	/*
	if (!world)
	{
		return nullptr;
	}
	
	if (runtime_map.Contains(world))
	{
		UE_LOG(LogKamoModule, Warning, TEXT("CreateRuntime called with active runtime. Why? World: %s"), *world->GetFullName());
	}
	else
	{
		auto runtime_instance = NewObject<UKamoRuntime>();
		runtime_map.Add(world, runtime_instance);
		if (!runtime_instance->Init(world))
		{
			runtime_map.Remove(world);
			return nullptr;
		}		
	}

	return runtime_map[world];
	*/
}


void FKamoModule::OnWorldDestroyed(UWorld* world)
{
	//UE_LOG(LogKamoModule, Display, TEXT("OnWorldDestroyed: world: %s"), *world->GetFullName());
	auto rt = runtime_map.Find(world);
	
	if (rt)
	{
		(*rt)->ShutdownRuntime();
		runtime_map.Remove(world);
	}
}


void FKamoModule::OnWorldBeginTearDown(UWorld* world)
{
	//UE_LOG(LogKamoModule, Display, TEXT("OnWorldBeginTearDown: world: %s"), *world->GetFullName());
	ShutdownRuntimeForWorld(world);
}


void FKamoModule::OnKamoRuntime(UKamoRuntime* kamo_runtime)
{
	if (kamo_runtime)
	{
		// Register reference kamo actor to class map
		FKamoClassMap ref;
		ref.kamo_class = AKamoReferenceActor::StaticClass();
		ref.actor_class = UKamoActor::StaticClass();
		kamo_runtime->classmap.Add(TEXT("_ref_class"), ref);
	}
}


void FKamoModule::ShutdownRuntimeForWorld(UWorld* world)
{
	const auto rt = runtime_map.Find(world);

	if (rt)
	{
		runtime_map.Remove(world);
		(*rt)->ShutdownRuntime();
	}
}


#if WITH_EDITOR


void FKamoModule::TestAction()
{
	UE_LOG(LogKamoModule, Warning, TEXT("It Works!!!"));
	FSoftObjectPtr kamoed(FSoftObjectPath(TEXT("EditorUtilityWidgetBlueprint'/Kamo/Tools/EDBP_KamoEdBase.EDBP_KamoEdBase'")));
	auto kamowidget = kamoed.LoadSynchronous();
	UE_LOG(LogKamoModule, Display, TEXT("Wheee"));
	//Cast<UEditorUtilityWidgetBlueprint>(kamowidget)->GetCreatedWidget()->Run();
}


void FKamoModule::OnPieBegin(bool bIsSimulating)
{
	//UE_LOG(LogKamoModule, Display, TEXT("OnPieBegin"));

	if (!bIsSimulating)
	{

	}
}

void FKamoModule::OnPieStarted(bool bIsSimulating)
{
	// Handle the PIE world as a normal game world
	if (auto WorldContext = GEditor->GetPIEWorldContext())
	{
		UWorld* PieWorld = WorldContext->World();
		//UE_LOG(LogKamoModule, Display, TEXT("OnPieStarted: world: %s"), *PieWorld->GetFullName());
		if (PieWorld)
		{
			ProcessNewWorld(PieWorld);
		}
	}
}

void FKamoModule::OnPieEnded(bool bIsSimulating)
{
	if (auto WorldContext = GEditor->GetPIEWorldContext())
	{
		UWorld* PieWorld = WorldContext->World();
		//UE_LOG(LogKamoModule, Display, TEXT("OnPieEnded: world: %s"), *PieWorld->GetFullName());
		if (!bIsSimulating && PieWorld)
		{
			OnWorldDestroyed(PieWorld);
		}
	}
}


void FKamoModule::OnAssetRenamed(const FAssetData& asset_data, const FString& name)
{
	//UE_LOG(LogKamoModule, Error, TEXT("OnAssetRenamed: full name: %s, new/old name: %s"), *asset_data.GetFullName(), *name);
}


void FKamoModule::OnAssetRemoved(const FAssetData& asset_data)
{
	//UE_LOG(LogKamoModule, Error, TEXT("OnAssetRemoved: full name: %s"), *asset_data.GetFullName());
}

#endif // WITH_EDITOR

FString FKamoModule::GetKamoTenantName() const
{
	return KamoUtil::get_tenant_name();
}

void FKamoModule::OnPreGarbageCollection()
{
	bIsCollectingGarbage = true;
}

void FKamoModule::OnPostGarbageCollection()
{
	bIsCollectingGarbage = false;
}

void FKamoModule::OnPostEngineInit()
{
	WorldAddedEventBinding = GEngine->OnWorldAdded().AddRaw(this, &FKamoModule::OnWorldCreated);
	WorldDestroyedEventBinding = GEngine->OnWorldDestroyed().AddRaw(this, &FKamoModule::OnWorldDestroyed);
}

IMPLEMENT_MODULE(FKamoModule, Kamo)
