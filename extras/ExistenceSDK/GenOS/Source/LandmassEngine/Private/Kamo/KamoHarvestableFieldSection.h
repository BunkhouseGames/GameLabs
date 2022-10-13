// 

#pragma once

#include "CoreMinimal.h"
#include "KamoObject.h"
#include "UObject/Object.h"
#include "KamoHarvestableFieldSection.generated.h"

/**
 * 
 */
UCLASS()
class LANDMASSENGINE_API UKamoHarvestableFieldSection : public UKamoActor
{
	GENERATED_BODY()
public:
	virtual void ApplyKamoStateToActor_Implementation(EKamoStateStage stage_stage) override;
	virtual void UpdateKamoStateFromActor_Implementation() override;
};
