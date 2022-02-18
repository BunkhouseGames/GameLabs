// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class {{cookiecutter.project_name}}Game : ModuleRules
{
	public {{cookiecutter.project_name}}Game(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

	}
}
