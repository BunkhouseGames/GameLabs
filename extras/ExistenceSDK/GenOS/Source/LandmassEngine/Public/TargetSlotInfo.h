// Copyright Arctic Theory, Ehf. All Rights Reserved.

#pragma once
#include "StaticSlotInfo.h"
#include "DynamicSlotInfo.h"

#include "CoreMinimal.h"

#include "TargetSlotInfo.generated.h"


USTRUCT(BlueprintType)
struct FTargetSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FHitResult HitResult;

	UPROPERTY(BlueprintReadOnly)
	class APawn* Instigator = nullptr;

	UPROPERTY(BlueprintReadOnly)
	bool HasSlotInfo = false;
	
	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<class AHarvestableFieldSection> HarvestableFieldSection;

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<class UHarvestableFieldAssetInstance> AssetInstance;
	
	UPROPERTY(BlueprintReadOnly)
	class UHarvestableFieldAsset* Asset = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FStaticSlotInfo StaticSlotInfo;
	
	UPROPERTY(BlueprintReadOnly)
	FDynamicSlotInfo DynamicSlotInfo;
	
	UPROPERTY(BlueprintReadOnly)
	int32 SlotIndex = -1;

	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<class AHarvestedProxy> HarvestedProxyClass;

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<class UGenosBaseHarvesterComponent> Harvester;

	UPROPERTY(BlueprintReadOnly)
	FName EventName;

	UPROPERTY(BlueprintReadOnly)
	float EventLifetime = 0.0;
};


