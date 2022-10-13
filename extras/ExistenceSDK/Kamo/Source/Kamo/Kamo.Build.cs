// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Kamo : ModuleRules
{
	public Kamo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
     
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
                "Json",
                "JsonUtilities",
				"DeveloperSettings",
				"KamoRuntime",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"UMG",
				"Slate",
				"SlateCore",
				"JsonUtilities",
				"OnlineSubsystemUtils",
				// ... add private dependencies that you statically link with here ...	
                "HTTP",
                "Projects",
				"Sockets",
				
				"KamoClient",

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
