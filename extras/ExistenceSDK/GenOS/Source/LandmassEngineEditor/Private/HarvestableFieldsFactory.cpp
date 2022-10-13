#include "HarvestableFieldsFactory.h"
#include "HarvestableFieldAsset.h"

UHarvestableFieldAssetFactory::UHarvestableFieldAssetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UHarvestableFieldAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UHarvestableFieldAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UHarvestableFieldAsset* LayerAsset = nullptr;
	if (ensure(SupportedClass == Class))
	{
		ensure(0 != (RF_Public & Flags));
		LayerAsset = NewObject<UHarvestableFieldAsset>(InParent, Class, Name, Flags);
		if (LayerAsset)
		{
			//LayerAsset->xxx = xxx;
		}
	}
	return LayerAsset;
}
