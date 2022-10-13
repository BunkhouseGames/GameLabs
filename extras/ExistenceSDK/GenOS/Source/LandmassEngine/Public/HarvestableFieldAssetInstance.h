// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StaticSlotInfo.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"

#include "HarvestableFieldAssetInstance.generated.h"

/**
 * 
 */
UCLASS()
class LANDMASSENGINE_API UHarvestableFieldAssetInstance : public UObject
{
	GENERATED_BODY()

public:
	void RefreshAsset(AActor* HarvestableField, const TArray<FStaticSlotInfo>& Slots, int32 AssetIndex);
	void UpdateInstanceTransform(int32 SlotIndex, const FTransform& Transform);
	void FinalizeUpdate();
	void CleanupAsset();
	
	/* Asset specific mapping function to map a HitResult to a slot. Returns -1 if there is no mapping. */
	int32 MapHitResultToSlotIndex(const FHitResult& HitResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, DuplicateTransient, TextExportTransient)
	TMap<int32, int32> InstanceIndices; // Key is SlotIndex, value is InstanceIndex

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient)
	bool IsModified;

	UPROPERTY(BlueprintReadOnly, Transient)
	class UHarvestableFieldAsset* Asset;

	UPROPERTY(Transient, DuplicateTransient)
	class UHierarchicalInstancedStaticMeshComponent* MeshComponent;

	UPROPERTY(Transient, DuplicateTransient)
	class UInstanceCollisionComponent* CollisionComponent;

private:

	static bool RemoveAndDestroyComponent(UObject* InComponent);
};
