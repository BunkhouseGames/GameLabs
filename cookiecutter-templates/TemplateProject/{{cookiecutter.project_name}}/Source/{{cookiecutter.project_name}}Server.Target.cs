// {{cookiecutter.copyright}}

using UnrealBuildTool;
using System.Collections.Generic;

public class {{cookiecutter.project_name}}ServerTarget : TargetRules
{
	public {{cookiecutter.project_name}}ServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		bUseLoggingInShipping = true;
	}
}
