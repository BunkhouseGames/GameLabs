// Copyright 2021 Arctic Theory ehf.. All Rights Reserved.

#include "LandmassEngineModule.h"
#include "GenosGameInstanceSubsystem.h"

#if	WITH_EDITOR
#include "Editor.h"
#endif


#define LOCTEXT_NAMESPACE "FLandmassEngineModule"

FLandmassEngineModule::FLandmassEngineModule()
{
}


FLandmassEngineModule::~FLandmassEngineModule()
{
}


void FLandmassEngineModule::StartupModule()
{
	HandlePostLoadMapBinding = FCoreUObjectDelegates::PostLoadMapWithWorld.AddRaw(this, &FLandmassEngineModule::HandlePostLoadMap);


#if WITH_EDITOR
	if (!IsRunningDedicatedServer())
	{
		// Bind events on PIE start/end to open/close camera
		PieBeginEventBinding = FEditorDelegates::BeginPIE.AddRaw(this, &FLandmassEngineModule::OnPieBegin);
		PieStartedEventBinding = FEditorDelegates::PostPIEStarted.AddRaw(this, &FLandmassEngineModule::OnPieStarted);
		PieEndedEventBinding = FEditorDelegates::PrePIEEnded.AddRaw(this, &FLandmassEngineModule::OnPieEnded);
	}
#endif

}

void FLandmassEngineModule::ShutdownModule()
{
	if (GEngine)
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(HandlePostLoadMapBinding);
	}


#if WITH_EDITOR
	FEditorDelegates::BeginPIE.Remove(PieBeginEventBinding);
	FEditorDelegates::PostPIEStarted.Remove(PieStartedEventBinding);
	FEditorDelegates::PrePIEEnded.Remove(PieEndedEventBinding);
#endif

}


void FLandmassEngineModule::HandlePostLoadMap(UWorld* InLoadedWorld)
{
	if (InLoadedWorld)
	{
		if (UGenosGameInstanceSubsystem* Genos = InLoadedWorld->GetSubsystem<UGenosGameInstanceSubsystem>())
		{
			Genos->HandlePostLoadMap(InLoadedWorld);
		}
	}
}


#if WITH_EDITOR

void FLandmassEngineModule::OnPieBegin(bool bIsSimulating)
{
	//UE_LOG(LogKamoModule, Display, TEXT("OnPieBegin"));

	if (!bIsSimulating)
	{

	}
}

void FLandmassEngineModule::OnPieStarted(bool bIsSimulating)
{
	// Handle the PIE world as a normal game world
	if (auto WorldContext = GEditor->GetPIEWorldContext())
	{
		UWorld* PieWorld = WorldContext->World();
		//UE_LOG(LogKamoModule, Display, TEXT("OnPieStarted: world: %s"), *PieWorld->GetFullName());
		if (PieWorld)
		{
			HandlePostLoadMap(PieWorld);
		}
	}
}

void FLandmassEngineModule::OnPieEnded(bool bIsSimulating)
{
	if (auto WorldContext = GEditor->GetPIEWorldContext())
	{
		UWorld* PieWorld = WorldContext->World();
		//UE_LOG(LogKamoModule, Display, TEXT("OnPieEnded: world: %s"), *PieWorld->GetFullName());
		if (!bIsSimulating && PieWorld)
		{
			//////OnWorldDestroyed(PieWorld);
		}
	}
}

#endif


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLandmassEngineModule, LandmassEngine)
