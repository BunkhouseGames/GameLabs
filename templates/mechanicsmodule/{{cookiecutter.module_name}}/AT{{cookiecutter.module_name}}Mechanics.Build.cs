// {{ cookiecutter._copyright }}

using UnrealBuildTool;

public class AT{{cookiecutter.module_name}}Mechanics : ModuleRules
{
    public AT{{cookiecutter.module_name}}Mechanics(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                // Engine modules
                "Core",
                "CoreUObject",
                "Engine",
                "GameplayTags",

                // Engine plugin modules
                "ModelViewViewModel",

                // 3rd party plugin modules
                //...

                // Local plugin modules
                "Kamo",

                // Local modules
                "ATMechanics",
            }
            );
	}
}