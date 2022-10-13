// Copyright 2021 Arctic Theory ehf.. All Rights Reserved.

#include "ATMissionSystemEditorModule.h"

#include "ATMissions.h"
#include "Factories/Factory.h"
#include "AssetTypeActions_Base.h"


#define LOCTEXT_NAMESPACE "FATMissionSystemEditorModule"



class FAssetTypeActions_MissionAsset : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MissionAsset", "Mission Asset"); }
	virtual FColor GetTypeColor() const override { return FColor(175, 0, 128); }
	virtual UClass* GetSupportedClass() const override { return UMission::StaticClass(); }
	virtual uint32 GetCategories() override { return EAssetTypeCategories::Gameplay; }
};




UMissionAssetFactory::UMissionAssetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UMission::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UMissionAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UMission* Mission = nullptr;
	if (ensure(SupportedClass == Class))
	{
		ensure(0 != (RF_Public & Flags));
		Mission = NewObject<UMission>(InParent, Name, Flags);
		if (Mission)
		{
			//LayerAsset->xxx = xxx;
		}
	}
	return Mission;
}







FATMissionSystemEditorModule::FATMissionSystemEditorModule()
{
}


FATMissionSystemEditorModule::~FATMissionSystemEditorModule()
{
}


void FATMissionSystemEditorModule::StartupModule()
{
	// Register asset types
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	{
		AssetTools.RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_MissionAsset));
	}
}

void FATMissionSystemEditorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FATMissionSystemEditorModule, ATMissionSystemEditor)
