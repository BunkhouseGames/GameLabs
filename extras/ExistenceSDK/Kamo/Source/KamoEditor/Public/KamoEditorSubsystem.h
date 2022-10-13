// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "EditorSubsystem.h"
#include "KamoEditorSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class KAMOEDITOR_API UKamoEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable)
	void RunStandaloneServer(TArray<FString> maps, TArray<FString> arguments, bool WaitForDebugger, bool KamoVerbose);
	
	UFUNCTION(BlueprintCallable)
	void WriteToConfig(FString Key, FString Value);

};
