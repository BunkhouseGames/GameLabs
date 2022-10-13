// Fill out your copyright notice in the Description page of Project Settings.


#include "KamoEditorSubsystem.h"
#include "Misc/App.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/Paths.h"

void UKamoEditorSubsystem::RunStandaloneServer(TArray<FString> maps, TArray<FString> arguments, bool WaitForDebugger, bool KamoVerbose)
{
	// TODO add the ability to use the debug editor if its compiled
	FString EnginePath = FPaths::Combine(FPaths::EngineDir(), TEXT("Binaries"), TEXT("Win64"), TEXT("UE4Editor.exe"));
	FString ProjectPath = FPaths::Combine(FPaths::ProjectDir(), FApp::GetProjectName()) + TEXT(".uproject");

	// Runs the external process
	for (FString map : maps)
	{
		FString port = FString::FromInt(FMath::RandRange(8000, 9000));
		FString args = ProjectPath + " " + map + " -server -log -port=" + port;

		FPlatformProcess::CreateProc(*EnginePath, *args, true, false, false, nullptr, 0, nullptr, nullptr);
	}
}
void UKamoEditorSubsystem::WriteToConfig(FString Key, FString Value)
{

	FString value;
	GConfig->SetString(TEXT("/Script/UnrealEd.KamoED"), TEXT("SelectedMaps"), TEXT("Fudge"), GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
	// GConfig->GetString(TEXT("/Script/UnrealEd.LevelEditorPlaySettings"), TEXT("MouseControlLabelPosition"), value, GEditorPerProjectIni);
	UE_LOG(LogTemp, Log, TEXT("%s"), *value);

}

