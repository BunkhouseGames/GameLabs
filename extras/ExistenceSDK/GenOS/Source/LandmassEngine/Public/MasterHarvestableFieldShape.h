// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MasterHarvestableField.h"
#include "MasterHarvestableFieldShape.generated.h"

UCLASS(Abstract)
class LANDMASSENGINE_API AMasterHarvestableFieldShape : public AMasterHarvestableField
{
	GENERATED_BODY()

public:
	// Overridden in child classes
	virtual FBox GetBoundingBox() const;
	virtual bool GetRandomLocation(const FBox& BBox, FVector& Location);

#if WITH_EDITOR
	virtual void PrepareSlots() override;
#endif
	
	UPROPERTY(EditAnywhere)
	int32 RandNumInstances = 20;

	UPROPERTY(EditAnywhere)
	int32 RandSeed;

	UPROPERTY(EditAnywhere)
	int32 MaxTries = 20;

	UPROPERTY(SimpleDisplay)
	int32 ActualInstances = 0;


	FRandomStream Rand;

protected:
	void AddRandomInstance(FBox BBox, FCollisionObjectQueryParams ObjectQueryParams,
					FCollisionQueryParams CollisionQueryParams);

	const float InitialSize = 1000.0;

};
