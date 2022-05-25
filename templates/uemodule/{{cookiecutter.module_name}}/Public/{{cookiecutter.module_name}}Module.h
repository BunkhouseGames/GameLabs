// {{cookiecutter.copyright}}

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"


class {{cookiecutter.module_name.upper()}}_API F{{cookiecutter.module_name}}Module: public IModuleInterface
{
public:

	F{{cookiecutter.module_name}}Module();
	~F{{cookiecutter.module_name}}Module();

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline F{{cookiecutter.module_name}}Module& Get()
	{
		return FModuleManager::LoadModuleChecked<F{{cookiecutter.module_name}}Module>("{{cookiecutter.module_name}}");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("{{cookiecutter.module_name}}");
	}

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

};

