// Fill out your copyright notice in the Description page of Project Settings.


#include "HarvestableFieldAsset.h"

#include "HarvestableFieldsStats.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"


UHarvestableFieldAssetInstance* UHarvestableFieldAsset::CreateInstance(UObject* Outer)
{
	auto Result = NewObject<UHarvestableFieldAssetInstance>(Outer);
	Result->Asset = this;
	return Result;
}


void UHarvestableFieldAsset::GenerateTransformForLocation(FTransform& Transform, const FVector& Location, FRandomStream& Rand)
{
	// Note: Always run the random generator so we have consistent results for each iteration.

	// Apply randomly adjusted sink or raise if applicable.
	float SinkAmount = Rand.FRandRange(MinSinkAmount, MaxSinkAmount);
	FVector AdjustedLocation = Location;
	AdjustedLocation.Z += SinkAmount;

	Transform.SetLocation(AdjustedLocation);

	// Apply random rotation if applicable
	FRotator Rotator(
		Rotation.Pitch + Rand.FRandRange(-RandomRotationValues.Pitch, RandomRotationValues.Pitch),
		Rotation.Yaw + Rand.FRandRange(-RandomRotationValues.Yaw, RandomRotationValues.Yaw),
		Rotation.Roll + Rand.FRandRange(-RandomRotationValues.Roll, RandomRotationValues.Roll)
	);

	Transform.SetRotation(Rotator.Quaternion());
	Transform.NormalizeRotation();

	// Apply random scaling if applicable
	if (MaxScale == MinScale)
	{
		if (MaxScale != 1.0)
		{
			Transform.SetScale3D(FVector(MaxScale));
		}
	}
	else
	{
		float Scale = Rand.FRandRange(MinScale, MaxScale);
		Transform.SetScale3D(FVector(Scale));
	}

}
