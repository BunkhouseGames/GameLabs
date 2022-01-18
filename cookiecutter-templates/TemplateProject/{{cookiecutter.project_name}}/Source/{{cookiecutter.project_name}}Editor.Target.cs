// {{cookiecutter.copyright}}

using UnrealBuildTool;
using System.Collections.Generic;

public class {{cookiecutter.project_name}}EditorTarget : TargetRules
{
	public {{cookiecutter.project_name}}EditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange( new string[] { } );
	}
}
