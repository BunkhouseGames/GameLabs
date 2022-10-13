// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "IKamoModule.h"
#include "KamoRt.h"

#include "Stats/Stats.h"

#include "Framework/Commands/Commands.h"

#if WITH_EDITOR
#include "EditorStyleSet.h"
#endif

struct FAssetData;
DECLARE_LOG_CATEGORY_EXTERN(LogKamoModule, Log, All);


class KAMO_API FKamoModule : public IKamoModule
{
public:

	FKamoModule();
	~FKamoModule();

	static inline FKamoModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FKamoModule>("Kamo");
	}

	virtual FString GetKamoTenantName() const override;

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	bool IsInitialized() { return is_initialized; }

	UKamoRuntime* GetKamoRuntime(UWorld* world);

private:
	bool is_initialized;
	TMap<UWorld*, UKamoRuntime*> runtime_map;

	FDelegateHandle WorldAddedEventBinding;
	FDelegateHandle WorldDestroyedEventBinding;
	FDelegateHandle WorldLoadEventBinding;
	FDelegateHandle WorldBeginTearDownBinding;

	FDelegateHandle KamoRuntimeEventBinding;

	void OnWorldCreated(UWorld* world);
	void OnPostLoadMapWithWorld(UWorld* world);
	void ProcessNewWorld(UWorld* world);
	void OnWorldDestroyed(UWorld* world);
	void OnWorldBeginTearDown(UWorld* world);
	UKamoRuntime* CreateRuntime(UWorld* world);

	void OnKamoRuntime(class UKamoRuntime* kamo_runtime);

	void ShutdownRuntimeForWorld(UWorld* world);

	void OnPreGarbageCollection();
	void OnPostGarbageCollection();
	void OnPostEngineInit();

#if WITH_EDITOR
	FTickerDelegate TickDelegate;
	FTSTicker::FDelegateHandle TickDelegateHandle;

	bool Tick(float);
	int numTicks = 0;

	FDelegateHandle PieBeginEventBinding;
	FDelegateHandle PieStartedEventBinding;
	FDelegateHandle PieEndedEventBinding;

	void OnPieBegin(bool bIsSimulating);
	void OnPieStarted(bool bIsSimulating);
	void OnPieEnded(bool bIsSimulating);

	void OnAssetRenamed(const FAssetData& asset_data, const FString& name);
	void OnAssetRemoved(const FAssetData& asset_data);

	TSharedPtr<FUICommandList> PluginCommands;
	void TestAction();
#endif

	bool bIsCollectingGarbage = false;
};
