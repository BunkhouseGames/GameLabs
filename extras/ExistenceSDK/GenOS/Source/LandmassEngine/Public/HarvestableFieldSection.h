// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DynamicSlotInfo.h"
#include "TargetSlotInfo.h"
#include "HarvestableFieldAsset.h"
#include "HarvestableFieldAssetInstance.h"
#include "StaticSlotInfo.h"
#include "KamoPersistable.h"
#include "StaticSlotInfoArray.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "Containers/BitArray.h"

#include "HarvestableFieldSection.generated.h"

class AMasterHarvestableField;

UCLASS()
class LANDMASSENGINE_API AHarvestableFieldSection : public AActor, public IKamoPersistable
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	AHarvestableFieldSection();

	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginDestroy() override;

	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;

	virtual void KamoAfterLoad_Implementation() override;

	void RefreshAssets();
	void CleanupAssets();

	void SuppressOccludedPoints();
	
	/* Asset specific mapping function to map a HitResult to a slot. Returns -1 if there is no mapping. */
	int32 MapHitResultToSlotIndex(const FHitResult& HitResult);

#if WITH_EDITOR
	void AddToFoliage();
#endif

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	AMasterHarvestableField* Master;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FString Id;
	
	// Static Slots Info
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FStaticSlotInfoArray Slots;

	// Dynamic and replicated Slots Info
	UPROPERTY(Replicated, Transient)
	FDynamicSlotInfoArray DynamicSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool KamoDirty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RegrowRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RegrowTickRate;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient)
	TArray<UHarvestableFieldAssetInstance*> AssetInstances;

	UFUNCTION(BlueprintCallable)
	FName GetAssetNameForSlot(int32 Index);
	
	UFUNCTION(BlueprintCallable)
	bool IsSlotActive(int32 Index);

	UFUNCTION(BlueprintPure)
	FVector GetSlotLocation(int32 Index);

	UFUNCTION(BlueprintPure)
	FTransform GetSlotTransform(int32 Index);

	UFUNCTION(BlueprintPure)
	UStaticMesh* GetSlotMesh(int32 Index);

	UFUNCTION(BlueprintPure)
	FBox GetCollisionBounds(int32 Index);

	UFUNCTION(BlueprintCallable)
	TArray<int32> GetHarvestablesInRange(const FVector& Location, float Radius);

	UFUNCTION(BlueprintCallable)
	int32 GetClosestHarvestableInRange(const FVector& Location, float Radius);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void HarvestRaw(int32 TargetIndex);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	static void HarvestMultipleTargets(const TArray<FTargetSlotInfo>& Targets);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Harvest(int32 TargetIndex, TSubclassOf<class AHarvestedProxy> HarvestedProxyClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void HarvestTarget(const FTargetSlotInfo& Target);

	void OnSlotChange(const FDynamicSlotInfo& DynamicSlotInfo);
	void HarvestAllPointsWithinBounds(const FBox& Box);

	UFUNCTION(BlueprintCallable)
	void SetDynamicSlotInfo(const FDynamicSlotInfo& DynamicSlotInfo);

	UFUNCTION(BlueprintCallable)
	void SpawnHarvestedProxy(const FTargetSlotInfo& Target);

	UPROPERTY()
	UBoxComponent* BoxComponent;
};

