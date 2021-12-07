// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class {{cookiecutter.project_name}}Target : TargetRules
{
	public {{cookiecutter.project_name}}Target( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		bUseLoggingInShipping = true;
		ExtraModuleNames.AddRange(new string[]
			{
				""
			});
	}
}
