// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"

DECLARE_LOG_CATEGORY_EXTERN(LogKamoRuntime, Log, All);


class FKamoRuntimeModule : public IModuleInterface
{
public:

	FKamoRuntimeModule();
	~FKamoRuntimeModule();

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FKamoRuntimeModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FKamoRuntimeModule>("KamoRuntime");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("KamoRuntime");
	}

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

};

