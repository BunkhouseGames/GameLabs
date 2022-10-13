// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class KamoRuntime : ModuleRules
{
	public KamoRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
		);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...                
			}
		);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Json",
				"Sockets",
				// ... add private dependencies that you statically link with here ...	
				"KamoClient",
			}
		);

		// If redis++ library is used we must enable exceptions.
		bEnableExceptions = true;

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);

		if (Target.Platform == UnrealTargetPlatform.Win64 || 
			Target.Platform == UnrealTargetPlatform.Linux)
        {
			PublicDefinitions.Add("WITH_REDIS_CLIENT=1");
			PrivateDependencyModuleNames.Add("RedisClient");
		}
        else
        {
			PublicDefinitions.Add("WITH_REDIS_CLIENT=0");
		}
	}
}
