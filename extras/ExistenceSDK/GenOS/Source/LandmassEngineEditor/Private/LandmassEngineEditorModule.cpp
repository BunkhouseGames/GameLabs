// Copyright 2021 Arctic Theory ehf.. All Rights Reserved.

#include "LandmassEngineEditorModule.h"
#include "AssetToolsModule.h"
#include "HarvestableFieldsFactory.h"

#define LOCTEXT_NAMESPACE "FLandmassEngineEditorModule"

FLandmassEngineEditorModule::FLandmassEngineEditorModule()
{
}


FLandmassEngineEditorModule::~FLandmassEngineEditorModule()
{
}


UObject* FLandmassEngineEditorModule::CreateAsset(UClass* Class, const FString& PackagePath, const FName& BaseName)
{
	UPackage* Package = CreatePackage(*PackagePath);
	UPackage* OutermostPkg = Package->GetOutermost();
	auto Factory = NewObject<UHarvestableFieldAssetFactory>();
	FName Name = MakeUniqueObjectName(OutermostPkg, Class, BaseName);
	return Factory->FactoryCreateNew(Class, OutermostPkg, Name, RF_Standalone | RF_Public, nullptr, GWarn);
}

void FLandmassEngineEditorModule::StartupModule()
{
	// Register asset types
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	{
		AssetTools.RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_LayerAsset));
	}

	// Register detail customizations
	auto& PropertyModule = FModuleManager::LoadModuleChecked< FPropertyEditorModule >("PropertyEditor");
}

void FLandmassEngineEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		auto& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLandmassEngineEditorModule, LandmassEngineEditor)
