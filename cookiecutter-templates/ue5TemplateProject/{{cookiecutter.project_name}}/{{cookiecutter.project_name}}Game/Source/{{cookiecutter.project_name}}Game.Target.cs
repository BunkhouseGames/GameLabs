// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class {{cookiecutter.project_name}}GameTarget : TargetRules
{
	public {{cookiecutter.project_name}}GameTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange( new string[] { "{{cookiecutter.project_name}}Game" } );
	}
}
