// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class RedisClient : ModuleRules
{
	public RedisClient(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

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
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Json",
				"Sockets",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

		
		// Add include path for hiredis and redis++ libraries
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../ThirdParty"));

		// Add hiredis library dependencies
		string Lib = Path.Combine(ModuleDirectory, "../ThirdParty/Lib");
		bool is_windows = Target.Platform == UnrealTargetPlatform.Win64;
#if !UE_5_0_OR_LATER
		is_windows = is_windows || Target.Platform == UnrealTargetPlatform.Win32;
#endif
		string libname;
		bool use_debug = Target.Configuration != UnrealTargetConfiguration.Shipping;
		if (is_windows)
		{
			libname = use_debug ? "hiredis_staticd.lib" : "hiredis_static.lib";
		}
		else
		{
			libname = use_debug ? "libhiredisd.a" : "libhiredis.a";
		}
		PublicAdditionalLibraries.Add(Path.Combine(Lib, Target.Platform.ToString(), libname));


		// Redis++ settings		
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../ThirdParty/redis++/no_tls")); // Pick no TLS option for the library
		bEnableExceptions = true;

	}
}
