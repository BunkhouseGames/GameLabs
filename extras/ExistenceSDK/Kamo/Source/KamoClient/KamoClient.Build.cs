// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class KamoClient : ModuleRules
{
	public KamoClient(ReadOnlyTargetRules Target) : base(Target)
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
				"OnlineSubsystem",
				"Engine",				
				"OnlineSubsystemUtils", 
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"DeveloperSettings",
				"UMG",

			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);



		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd",
					"EditorStyle",
					"Blutility",
					"UMGEditor",
					"KamoEditor",
				}
				);
		}
	}
}
