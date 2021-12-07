// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class {{cookiecutter.project_name}}ServerTarget : TargetRules
{
	public {{cookiecutter.project_name}}ServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		
		ExtraModuleNames.AddRange( new string[] {  } );
		bUseLoggingInShipping = true;
	}
}
