// Fill out your copyright notice in the Description page of Project Settings.


#include "MasterHarvestableFieldShape.h"

FBox AMasterHarvestableFieldShape::GetBoundingBox() const
{
	return FBox();
}

bool AMasterHarvestableFieldShape::GetRandomLocation(const FBox& BBox, FVector& Location)
{
	return false;
}

void AMasterHarvestableFieldShape::AddRandomInstance(FBox BBox, FCollisionObjectQueryParams ObjectQueryParams,
	FCollisionQueryParams CollisionQueryParams)
{
	FVector Point;
	if (!GetRandomLocation(BBox, Point))
	{
		// We exhausted our tries
		return;
	}

	// Try find the ground beneath and place the slot there. 
	// TODO: Account for things that block the terrain below.

	// We manipulate Z to span the top to bottom of our bounding box.
	FVector Bottom = Point;
	Point.Z = BBox.Max.Z;
	Bottom.Z = BBox.Min.Z;

	FHitResult Result;
	{
		SCOPE_CYCLE_COUNTER(STAT_HF_LineTrace);
		GetWorld()->LineTraceSingleByObjectType(Result, Point, Bottom, ObjectQueryParams, CollisionQueryParams);
	}
		
	if (!Result.IsValidBlockingHit())
	{
		// No surface below us
		return;
	}

	
	FVector Location = Result.ImpactPoint;

	int32 AssetIndex = Rand.RandRange(0, Assets.Num() - 1);
	UHarvestableFieldAsset* NewAsset = Assets[AssetIndex];
	if (NewAsset)
	{
		FStaticSlotInfo Slot;
		Slot.AssetIndex = AssetIndex;
		NewAsset->GenerateTransformForLocation(Slot.Transform, Location, Rand);
		Slots.Add(Slot);
	}	
}

#if WITH_EDITOR
void AMasterHarvestableFieldShape::PrepareSlots()
{
	SCOPE_CYCLE_COUNTER(STAT_HF_PrepareSlots);
	if (!Assets.Num())
	{
		return;
	}

	// Set up initial values
	FBox BBox = GetBoundingBox();
	FVector Center;
	FVector Extents;
	BBox.GetCenterAndExtents(Center, Extents);
	FCollisionObjectQueryParams ObjectQueryParams(FCollisionObjectQueryParams::AllStaticObjects);
	FCollisionQueryParams CollisionQueryParams(FCollisionQueryParams::DefaultQueryParam);
	CollisionQueryParams.AddIgnoredActor(this);

	// Empty our cloud and prepare for generation
	Slots.Empty();
	Rand.Initialize(RandSeed);

	for (int i = 0; i < RandNumInstances; i++)
	{
		AddRandomInstance(BBox, ObjectQueryParams, CollisionQueryParams);
	}
}
#endif