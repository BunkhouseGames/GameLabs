// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/CollisionProfile.h"
#include "HarvestedProxy.h"

#include "HarvestableFieldAsset.generated.h"

/**
* Asset types for Layers.
*/
UCLASS(BlueprintType)
class LANDMASSENGINE_API UHarvestableFieldAsset : public UObject
{
	GENERATED_BODY()

public:

	class UHarvestableFieldAssetInstance* CreateInstance(UObject* Outer);
	void GenerateTransformForLocation(FTransform& Transform, const FVector& Location, FRandomStream& Rand);

	/** Cardinal name of this asset which must be unique within the project. External tools and data structures
	use this name to reference this asset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName AssetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool ConvertFromFoliage = true;
	
	// Data stuff
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UStaticMesh* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TObjectPtr<class UMaterialInterface>> OverrideMaterials;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AHarvestedProxy> HarvestedProxyClass = AHarvestedProxy::StaticClass();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StartCullDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 EndCullDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool OverrideCollisionProfile = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCollisionProfileName CollisionProfile = UCollisionProfile::BlockAll_ProfileName;

	// Parameters for procedural generation
	UPROPERTY(EditAnywhere)
	float MinScale = 1.0f;

	UPROPERTY(EditAnywhere)
	float MaxScale = 1.0f;

	UPROPERTY(EditAnywhere)
	float MinSinkAmount = 0.0f;

	UPROPERTY(EditAnywhere)
	float MaxSinkAmount = 0.0f;

	/* Initial rotation. Default is zero (straight up, no frills).*/
	UPROPERTY(EditAnywhere)
	FRotator Rotation;

	/** Random rotation angles. Each angle is adjusted by a random +- value. Use values 0-180. */
	UPROPERTY(EditAnywhere)
	FRotator RandomRotationValues;

};

