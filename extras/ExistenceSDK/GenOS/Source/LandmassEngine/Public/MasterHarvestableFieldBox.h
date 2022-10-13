// 

#pragma once

#include "CoreMinimal.h"
#include "MasterHarvestableFieldSpline.h"
#include "MasterHarvestableFieldBox.generated.h"

/**
 * 
 */
UCLASS()
class LANDMASSENGINE_API AMasterHarvestableFieldBox : public AMasterHarvestableFieldShape
{
	GENERATED_BODY()
public:
	AMasterHarvestableFieldBox();
	
	UPROPERTY(NoClear, EditAnywhere, BlueprintReadWrite)
	class UBoxComponent* Box;
	
	virtual FBox GetBoundingBox() const override;
	virtual bool GetRandomLocation(const FBox& BBox, FVector& Location) override;
};
