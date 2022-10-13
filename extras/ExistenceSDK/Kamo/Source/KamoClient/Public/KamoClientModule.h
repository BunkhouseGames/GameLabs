// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"
#include "Engine/LocalPlayer.h"

#include "KamoClientModule.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogKamoClientModule, Log, All);


class FKamoClientModule : public IModuleInterface
{
public:

	FKamoClientModule();
	~FKamoClientModule();

	
	bool IsGameModule() const override
	{
		return true;
	}

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FKamoClientModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FKamoClientModule>("KamoClient");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("KamoClient");
	}

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};


UCLASS(transient)
class KAMOCLIENT_API ULocalKamoPlayer : public ULocalPlayer
{
	GENERATED_BODY()

public:

	virtual FString GetNickname() const override;

};
