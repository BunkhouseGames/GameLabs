// 

#pragma once

#include "CoreMinimal.h"
#include "FoliageType.h"
#include "MasterHarvestableField.h"
#include "MasterHarvestableFieldFoliage.generated.h"

/**
 * 
 */
UCLASS()
class LANDMASSENGINE_API AMasterHarvestableFieldFoliage : public AMasterHarvestableField
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	int32 FindOrAddAsset(UFoliageType* FoliageType, const TArray<FAssetData>& AssetDataList);
	bool IsDuplicate(FStaticSlotInfo SlotInfo, int32 AssetIndex, int32 Index);

	virtual void PrepareSlots() override;
	void PrepareSlotsForFoliageActor(class AInstancedFoliageActor* FoliageActor);

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Authoring")
	void ConvertToFoliage();
#endif

	// Where to place generated asset files
	UPROPERTY(EditAnywhere, Category = "Authoring", meta = (LongPackageName))
	FDirectoryPath PackagePath = FDirectoryPath{ TEXT("/Game/Assets/Harvestable") };

	// Slots with identical transform and mesh are considered duplicates and will be ignored
	UPROPERTY(EditAnywhere, Category = "Authoring")
	bool IgnoreDuplicateSlots = false;

};
