// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "KamoRt.h"

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"

#include "KamoBP.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogKamoBP, Log, All);


/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class KAMO_API UKamoBP : public UObject
{
	GENERATED_BODY()
    
    UPROPERTY()
	UKamoRuntime* runtime;
    
	static UKamoBP* GetKamoWorld(UWorld* world);

public:
	UFUNCTION(BlueprintPure, Category = "Kamo")
	static UDataTable* GetKamoTable();

	UFUNCTION(BlueprintPure, Category = "Kamo")
	static UKamoBP* GetKamoWorldFromActor(AActor* actor);

	UFUNCTION(BlueprintPure, Category = "Kamo")
	static UKamoBP* GetKamoWorldFromKamoObject(UKamoActor* kamo_object);

	UFUNCTION(BlueprintCallable, Category = "Kamo")
	AGameModeBase* GetGameMode();
	
    UFUNCTION(BlueprintCallable, Category = "Kamo")
    bool MoveObject(UKamoID* id, UKamoID* root_id, const FString& spawn_target);
    
	UFUNCTION(BlueprintPure, Category = "Kamo")
	UKamoChildObject* GetObjectFromActor(AActor* actor) const;

	UFUNCTION(BlueprintPure, Category = "Kamo")
	static UKamoChildObject* GetObjectFromActorNow(AActor* actor);

	/* Access to Kamo Object map in the runtime. Debugging purpose only! */
	UFUNCTION(BlueprintPure, Category = "Kamo")
    TMap<FString, UKamoObject*> GetInternalState();
    
    UFUNCTION(BlueprintCallable, Category = "Kamo")
    bool SendMessage(UKamoID* handler_id, const FString& message_type, UKamoState* payload);

	UFUNCTION(BlueprintPure, Category = "Kamo")
	UUE4ServerHandler* GetUE4Server(UKamoID* handler_id);

	UFUNCTION(BlueprintPure, Category = "Kamo")
	UUE4ServerHandler* GetUE4ServerForRoot(UKamoID* id);

	UFUNCTION(BlueprintPure, Category = "Kamo")
	UUE4ServerHandler* GetUE4ServerForChild(UKamoID* id);
    
	UFUNCTION(BlueprintPure, Category = "Kamo")
	AActor* SpawnActorForKamoObject(UKamoActor* kamo_object);

	/*	UFUNCTION(BlueprintPure, Category = "Kamo")
	FKamoClassMap GetKamoClass(const FString& class_name);
	*/

	UFUNCTION(BlueprintPure, Category = "Kamo\|Utilities")
	static UKamoID* GetKamoIDFromString(const FString& kamo_id);

	// TODO: This is a utility function that should be elsewhere
	UFUNCTION(BlueprintPure, Category = "Kamo\|Utilities")
	static FString GetCommandLineArgument(const FString& argument);

	// The following two functions should be removed and a single struct exposed with all this runtime info instead
	UFUNCTION(BlueprintPure, Category = "Kamo\|Utilities")
	UKamoID* GetUE4ServerID();

	UFUNCTION(BlueprintPure, Category = "Kamo\|Utilities")
	TArray<UKamoID*> GetUE4ServerRegions();

	// TODO: Figure out the purpose of the following four functions.
	UFUNCTION(BlueprintPure, Category = "Kamo\|Utilities")
	static UKamoState* CreateMessageCommand(UKamoID* kamo_id, UKamoID* root_id, const FString& command, const FString& parameters);

	UFUNCTION(BlueprintPure, Category = "Kamo\|Utilities")
	static UKamoState* CreateCommandMessagePayload(TArray<UKamoState*> commands);

	UFUNCTION(BlueprintCallable, Category = "Kamo\|Utilities")
	bool SendCommandToObject(UKamoID* id, const FString& command, const FString& parameters);

	UFUNCTION(BlueprintCallable, Category = "Kamo\|Utilities")
	bool SendCommandsToObject(UKamoID* id, TArray<UKamoState*> commands);

	UFUNCTION(BlueprintCallable, Category = "Kamo\|Utilities")
	bool EmbedObject(UKamoID* id, UKamoID* container_id, const FString& category);

	UFUNCTION(BlueprintCallable, Category = "Kamo\|Utilities")
	bool ExtractObject(UKamoID* id, UKamoID* container_id);

	UFUNCTION(BlueprintCallable, Category = "Kamo\|Utilities")
	UKamoID* FormatRegionName(const FString& map_name, const FString& volume_name, const FString& instance_id);

	// TODO: These are utility functions that should be elsewhere

	UFUNCTION(BlueprintCallable, Category = "Kamo\|Utilities")
	static void SetEditorUserSettingsBool(FString Key, bool Value);

	UFUNCTION(BlueprintCallable, Category = "Kamo\|Utilities")
	static void SetEditorUserSettingsString(FString Key, FString Value);

	UFUNCTION(BlueprintPure, Category = "Kamo\|Utilities")
	static void GetEditorUserSettingsBool(FString Key, bool& Found, bool& Value);

	UFUNCTION(BlueprintPure, Category = "Kamo\|Utilities")
	static void GetEditorUserSettingsString(FString Key, bool& Found, FString& Value);

	UFUNCTION(BlueprintPure, Category = "Kamo\|Utilities")
	static class UKamoProjectSettings* GetKamoSettings();

	UFUNCTION(BlueprintCallable, Category = "Kamo\|Utilities")
	static class UKamoState* KamoStateFromJsonString(const FString& jsonString);

	UFUNCTION(BlueprintCallable, Category = "Kamo\|Utilities")
	static void WinSetConsoleTitle(const FString& title);

};
