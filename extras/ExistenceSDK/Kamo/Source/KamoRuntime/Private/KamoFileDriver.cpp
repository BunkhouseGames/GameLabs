// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.


#include "KamoFileDriver.h"
#include "KamoRuntimeModule.h"

#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"


FString KamoFileDriver::GetHomePath() const
{
#if PLATFORM_DESKTOP
#if PLATFORM_WINDOWS
	auto variable_name = TEXT("USERPROFILE");
#else
	auto variable_name = TEXT("HOME");
#endif

	auto home = FPlatformMisc::GetEnvironmentVariable(variable_name);
	if (home.IsEmpty())
	{
		UE_LOG(LogKamoRuntime, Error, TEXT("KamoFileDriver::GetHomePath - Environment var not found: %s"), variable_name);
		return "";
	}
#else
	// non-desktop platforms
	auto home = FPaths::ProjectSavedDir();
#endif

	// ~/.kamo/<name>/<driver type>
	auto home_path = FPaths::Combine(home, TEXT(".kamo"), tenant_name, GetDriverType());
	UE_LOG(LogKamoRuntime, Display, TEXT("KamoFileDriver::GetHomePath - generating: %s"), *home_path);
	return home_path;
}


bool KamoFileDriver::OnSessionCreated()
{
	session_path = GetHomePath();
	UE_LOG(LogKamoRuntime, Display, TEXT("KamoFileDriver: Session path is: %s"), *session_path);
	

	IPlatformFile& pf = FPlatformFileManager::Get().GetPlatformFile();
	if (!pf.DirectoryExists(*session_path) && !pf.CreateDirectoryTree(*session_path))
	{
		UE_LOG(LogKamoRuntime, Error, TEXT("KamoFileDriver: Cannot create directory: %s"), *session_path);
		return false;
	}

	if (pf.DirectoryExists(*session_path))
	{
		UE_LOG(LogKamoRuntime, Display, TEXT("KamoFileDriver: URL: %s"), *GetSessionURL());
		return true;
	}
	
	UE_LOG(LogKamoRuntime, Error, TEXT("KamoFileDriver: Path not found: %s"), *session_path);
	return false;
}


