// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

#include "KamoRuntimeInterface.h"


class IKamoModule : public IModuleInterface
{
public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline IKamoModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IKamoModule>("Kamo");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("Kamo");
	}


	// Make Kamo tenant name available to all
	virtual FString GetKamoTenantName() const = 0;
};
