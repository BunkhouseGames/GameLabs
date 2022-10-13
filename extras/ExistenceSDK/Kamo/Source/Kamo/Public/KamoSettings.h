// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"

#include "KamoSettings.generated.h"


class KAMO_API KamoUtil
{
public:
	static FString get_tenant_name();
	static FString get_driver_name();
	static FString GetCommandLineSwitch(const FString& switch_name);
};



UCLASS(BlueprintType, Config = Game, DefaultConfig, meta = (DisplayName = "Kamo"))
class KAMO_API UKamoProjectSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:


	UPROPERTY(BlueprintReadWrite, Config, noclear, EditAnywhere, meta = (MetaClass = "DataTable", DisplayName = "Class map"), Category = "KamoSettings")
		FSoftObjectPath kamo_table;

	/* If left empty it will default to the -kamotenant value passed in through the command line or if that is not available the project name .*/
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Kamo Tenant"), Category = "KamoSettings")
		FString kamo_tenant = "";

	/* If left empty it will default to the -kamodriver value passed in through the command line or if that is not available it defaults to "file" driver.*/
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Kamo Driver"), Category = "KamoSettings")
	FString kamo_driver = "";

	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Default Player Region For New Players"), Category = "KamoSettings")
	FString DefaultPlayerRegion = "";

	

	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Editor widgets that should be refreshed during startup", MultiLine = true), Category = "KamoSettings")
		TArray<FString> editorWidgets;

	// KamoEd settings

	/* Include directories for KamoEd plugins relative from project root.*/
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, Category = "KamoSettings")
		TArray<FString> kamoed_plugins;


	// Kamo Server settings
	/* Exclude the following maps from Kamo */
	UPROPERTY(config, EditAnywhere, Category = "KamoSettings", meta = (AllowedClasses = "World", DisplayName = "Exclude the following maps from Kamo"))
	TArray<FSoftObjectPath> ExcludedKamoMaps;

	// Kamo Runtime tweaks - Will be repurposed once tick groups are in.*/
	/** Mark and sync rate */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, Category = "KamoSettings")
		float mark_and_sync_rate = 1.0;

	/** Message queue flush rate - Will be repurposed once tick groups are in.*/
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, Category = "KamoSettings")
		float message_queue_flush_rate = 0.250;

	/** Create Kamo Runtime Just-In-Time*/
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, Category = "KamoSettings")
		bool jit_create_runtime = false;

	/** Shutdown server if no players have been online for given secs */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, Category = "KamoSettings")
	float power_saving_after_sec = 0.0;

	/** Power save shutdown only on Linux servers */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, Category = "KamoSettings")
	bool power_save_only_on_linux = true;

	/** Server heartbeat interval in secs */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, Category = "KamoSettings")
	float server_heartbeat_interval = 20.0;

#if WITH_EDITORONLY_DATA
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, Category = "KamoSettings")
	bool DoCourtesyMoveInPIE = true;
#endif

	UKamoProjectSettings(const FObjectInitializer& ObjectInitializer);

	// Begin UDeveloperSettings Interface
	virtual FName GetCategoryName() const override;
#if WITH_EDITOR
	virtual FText GetSectionText() const override;
#endif
	// END UDeveloperSettings Interface

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(BlueprintCallable, Category = "Kamo\|Utilities")
	void SaveConfig();


public:
	static UKamoProjectSettings* Get() { return CastChecked<UKamoProjectSettings>(UKamoProjectSettings::StaticClass()->GetDefaultObject()); }

};
