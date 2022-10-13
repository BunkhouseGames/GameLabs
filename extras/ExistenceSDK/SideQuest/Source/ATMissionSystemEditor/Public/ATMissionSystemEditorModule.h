// Copyright 2021 Arctic Theory ehf.. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

#include "ATMissionSystemEditorModule.generated.h"


class ATMISSIONSYSTEMEDITOR_API FATMissionSystemEditorModule: public IModuleInterface
{
public:

	FATMissionSystemEditorModule();
	~FATMissionSystemEditorModule();

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FATMissionSystemEditorModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FATMissionSystemEditorModule>("ATMissionSystemEditor");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("ATMissionSystemEditor");
	}

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

};


UCLASS(hidecategories = Object)
class ATMISSIONSYSTEMEDITOR_API UMissionAssetFactory : public UFactory
{
	GENERATED_BODY()

		UMissionAssetFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//~ Begin UFactory Interface

};