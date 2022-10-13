// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#include "KamoSettings.h"
#include "Kamo.h"

FString KamoUtil::get_tenant_name()
{
	// Read tenant name from commandline
	FString tenant = GetCommandLineSwitch("kamotenant");

	if (tenant.IsEmpty())
	{
		tenant = GetCommandLineSwitch("tenant");
		if (!tenant.IsEmpty())
		{
			UE_LOG(LogKamo, Warning, TEXT("Using 'tenant' command line argument is deprecated. Please use '-kamotenant=%s'"), *tenant);
		}
	}
		
	if (tenant != "")
	{
		UE_LOG(LogKamo, Log, TEXT("tenant name from commandline: %s"), *tenant);
		return tenant;
	}

	// Read tenant name from project settings
	const auto Settings = UKamoProjectSettings::Get();
	tenant = Settings->kamo_tenant;

	if (Settings->kamo_tenant != "")
	{
		UE_LOG(LogKamo, Log, TEXT("reading tenant from project settings: %s"), *tenant);
		return Settings->kamo_tenant;
	}

	// If we the tenant name has not been passed in we use the project name
	tenant = FString(FApp::GetProjectName()).ToLower();
	//UE_LOG(LogKamo, Log, TEXT("default tenant name from project: %s"), *tenant);

	return tenant;
}


FString KamoUtil::get_driver_name()
{
	// Read driver name from commandline
	FString driver = GetCommandLineSwitch("kamodriver");

	if (driver != "")
	{
		UE_LOG(LogKamo, Log, TEXT("driver name from commandline: %s"), *driver);
		return driver;
	}

	// Read driver name from project settings
	const auto Settings = UKamoProjectSettings::Get();
	driver = Settings->kamo_driver;

	if (Settings->kamo_driver != "")
	{
		//UE_LOG(LogKamo, Log, TEXT("reading driver from project settings: %s"), *driver);
		return Settings->kamo_driver;
	}

	// If we the driver name has not been passed in we use "file" driver.
	UE_LOG(LogKamo, Log, TEXT("default driver \"file\" used"));
	return FString("file");
}


FString KamoUtil::GetCommandLineSwitch(const FString& switch_name)
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

UKamoProjectSettings::UKamoProjectSettings(const FObjectInitializer& ObjectInitlaizer)
	: Super(ObjectInitlaizer)
{
	// Add default class map entries here...
}

FName UKamoProjectSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

#if WITH_EDITOR
FText UKamoProjectSettings::GetSectionText() const
{
	return NSLOCTEXT("KamoPlugin", "KamoSettingsSection", "Kamo");
}
#endif

#if WITH_EDITOR
void UKamoProjectSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// If anything needs to be refreshed in the runtime...

	if (PropertyChangedEvent.Property != NULL)
	{
		const FName PropName = PropertyChangedEvent.MemberProperty->GetFName();
		if (
			PropName == GET_MEMBER_NAME_CHECKED(UKamoProjectSettings, kamo_tenant)
			)
		{
		}
	}
	
	// TODO: Make sure the data table is using 'FKamoClassMap' as row structure (see elsewhere, Runtime Init)
}
#endif


void UKamoProjectSettings::SaveConfig()
{
	Super::SaveConfig();
}
