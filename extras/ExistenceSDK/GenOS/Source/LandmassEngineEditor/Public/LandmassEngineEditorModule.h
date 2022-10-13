// Copyright 2021 Arctic Theory ehf.. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class FLandmassEngineEditorModule : public IModuleInterface
{
public:

	FLandmassEngineEditorModule();
	~FLandmassEngineEditorModule();

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FLandmassEngineEditorModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FLandmassEngineEditorModule>("LandmassEngineEditor");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("LandmassEngineEditor");
	}

	virtual UObject* CreateAsset(UClass* Class, const FString& PackagePath, const FName& BaseName);
	
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

};

