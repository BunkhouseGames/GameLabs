// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MasterHarvestableFieldShape.h"
#include "MasterHarvestableFieldSpline.generated.h"

UCLASS(Blueprintable)
class LANDMASSENGINE_API AMasterHarvestableFieldSpline : public AMasterHarvestableFieldShape
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMasterHarvestableFieldSpline();

	virtual FBox GetBoundingBox() const override;
	virtual bool GetRandomLocation(const FBox& BBox, FVector& Location) override;

	UPROPERTY(NoClear, EditAnywhere, BlueprintReadWrite)
	class USplineComponent* Spline;

protected:
	FBox GetSplineBounds() const;

	bool IsPointInSpline(const FVector& Point) const;
};
