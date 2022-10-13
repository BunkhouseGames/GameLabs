// Copyright 2021 Arctic Theory ehf.. All Rights Reserved.. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class LandmassEngine : ModuleRules
{
    public LandmassEngine(ReadOnlyTargetRules Target) : base(Target)
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
				"Core", "Engine"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"NetCore",
				"Slate",
				"SlateCore",
				"Json",
				"JsonUtilities",
				"AssetRegistry",
				"Kamo",
				"Foliage",
				"NavigationSystem",
				"PhysicsCore"
				// ... add private dependencies that you statically link with here ...	
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
					"EditorScriptingUtilities",
					"AssetTools",
				}
				);
			PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
					Path.Combine(ModuleDirectory, "../LandmassEngineEditor/Public")
				}
			);
				
		}
	}
}