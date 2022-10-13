// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TargetSlotInfo.h"
#include "Subsystems/WorldSubsystem.h"
#include "GenosGameInstanceSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FHarvestable
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	AHarvestableFieldSection* Section = nullptr;

	UPROPERTY(BlueprintReadWrite)
	int32 Index = -1;

	UPROPERTY(BlueprintReadWrite)
	AHarvestedProxy* Proxy = nullptr;
};

/**
 * 
 */
UCLASS()
class LANDMASSENGINE_API UGenosGameInstanceSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:


	DECLARE_MULTICAST_DELEGATE_OneParam(FOnHarvestableActorsReadyDelegate, UGenosGameInstanceSubsystem*);
	FOnHarvestableActorsReadyDelegate OnHarvestableActorsReady;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotHarvested, const FTargetSlotInfo&, HarvestedInfo);

	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnSlotHarvested OnSlotHarvested;


	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void HandlePostLoadMap(UWorld* InLoadedWorld);


	UPROPERTY(BlueprintReadOnly)
	TArray<class AHarvestableFieldSection*> HarvestableFields;

	UPROPERTY(BlueprintReadOnly)
	TArray<class UGenosBaseHarvesterComponent*> HarvesterComponents;

	/** Get all harvestables that are inside or intersect with 'Box'.*/
	//UFUNCTION(BlueprintCallable)
	TArray<AHarvestableFieldSection*> GetHarvestablesInRegion(const FBox& Box);

	UFUNCTION(BlueprintCallable)
	FHarvestable GetClosestHarvestableInRange(FVector Location, float Radius);

	UFUNCTION(BlueprintCallable)
	FHarvestable GetClosestHarvestableInFront(FVector Location, FVector Direction, float Range, FName AssetName);

private:


	/** Notify Genos agent components that intersect with any harvestable field*/
	void NotifyIntersectedAgents();
	
};
