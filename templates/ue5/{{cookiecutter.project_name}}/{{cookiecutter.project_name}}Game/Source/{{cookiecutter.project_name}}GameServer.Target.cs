// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class {{cookiecutter.project_name}}GameServerTarget : TargetRules
{
	public {{cookiecutter.project_name}}GameServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.Add("{{cookiecutter.project_name}}Game");
		bUseLoggingInShipping = true;
	}
}
