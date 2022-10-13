// Copyright 2021 Arctic Theory ehf.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "AssetTypeActions_Base.h"
#include "HarvestableFieldAsset.h"

#include "HarvestableFieldsFactory.generated.h"

UCLASS(hidecategories = Object)
class LANDMASSENGINEEDITOR_API UHarvestableFieldAssetFactory : public UFactory
{
	GENERATED_BODY()

		UHarvestableFieldAssetFactory(const FObjectInitializer& ObjectInitializer);

public:
	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//~ Begin UFactory Interface

};





class FAssetTypeActions_LayerAsset : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_LayerAsset", "Layer Asset"); }
	virtual FColor GetTypeColor() const override { return FColor(175, 0, 128); }
	virtual UClass* GetSupportedClass() const override { return UHarvestableFieldAsset::StaticClass(); }
	virtual uint32 GetCategories() override { return EAssetTypeCategories::Gameplay; }
};
