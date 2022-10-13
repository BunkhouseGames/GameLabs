// Copyright 2021 Arctic Theory ehf.. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"


class FLandmassEngineModule : public IModuleInterface
{
public:

	FLandmassEngineModule();
	~FLandmassEngineModule();

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FLandmassEngineModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FLandmassEngineModule>("LandmassEngine");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("LandmassEngine");
	}

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void HandlePostLoadMap(UWorld* InLoadedWorld);

private:

	FDelegateHandle HandlePostLoadMapBinding;



#if WITH_EDITOR
	FDelegateHandle PieBeginEventBinding;
	FDelegateHandle PieStartedEventBinding;
	FDelegateHandle PieEndedEventBinding;

	void OnPieBegin(bool bIsSimulating);
	void OnPieStarted(bool bIsSimulating);
	void OnPieEnded(bool bIsSimulating);
#endif

};

