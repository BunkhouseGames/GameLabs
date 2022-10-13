// Copyright Arctic Theory, Ehf. All Rights Reserved.

#pragma once
#include "StaticSlotInfo.h"
#include "DynamicSlotInfo.h"
#include "TargetSlotInfo.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/CollisionProfile.h"
#include "GenosBaseHarvesterComponent.generated.h"


UCLASS( Abstract, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LANDMASSENGINE_API UGenosBaseHarvesterComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGenosBaseHarvesterComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Called by GenosGameInstanceSubsystem once all harvestables and harvesteres are loaded.
	virtual void PostInitialize();



	/**
	Suppresses slots in harvestable field.

	All harvestable slots located below the bounding box of this component (or owner) will be suppressed
	and marked as disabled. Regrowth from these slots is disabled as well.

	Use this to enable actors to sit on top of harvestable field without trees and such sticking through them.
	*/
	UFUNCTION(BlueprintCallable)
	void SuppressSlotsBelowActor();

	/*
	Set to true to suppress slots on startup/BeginPlay.
	
	The suppression is executed on BeginPlay/PostInitialize if 'SuppressSlots' is true.

	Note as this runs on BeginPlay it does not use any world query. The asset instancing is done after the slots
	have been suppressed.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Harvester)
	bool SuppressSlots = false;

	// Optional name to associate with harvesting event. Use it as a hint for harvesting proxies.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = Harvester)
	FName HarvestingEventName;

	/* Scan harvestables within range */
	UFUNCTION(BlueprintCallable)
	void ScanForHarvestables();

	UFUNCTION(BlueprintCallable)
	static void ProcessHitResults(const TArray<FHitResult> HitResults, TArray<FTargetSlotInfo>& TargetSlots, UGenosBaseHarvesterComponent* Harvester = nullptr);

	/* Harvest all items from scan results. Limit the number of items with 'ItemLimit' or 0 for no limit. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void HarvestFromScanResults(int32 ItemLimit=0);
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = Scanner)
	FCollisionProfileName ScanProfile = UCollisionProfile::BlockAll_ProfileName;

	/* Scan interval in seconds. 0 means every frame, < 0 means no automatic scanning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = Scanner)
	float ScanInterval = 0.0;
	
	float LastScan = 0.0;  // Keep track of time between scans

	/*Scanning radius for slot detection within the range of this componnent. Default is 5 meters.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = Scanner)
	float Radius = 200.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = Scanner)
	float HalfHeight = 50.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = Scanner)
	float Distance = 500.0;

	/* True to test against complex collision, false to test against simplified collision.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = Scanner)
	bool bTraceComplex = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Scanner, AdvancedDisplay)
	TArray<AActor*> ActorsToIgnore;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = Scanner)
	bool bDrawDebugLines = false;

	/** Vertex color to apply to the frames */
	UPROPERTY(BlueprintReadOnly, Interp, Category = Scanner)
	FLinearColor TraceColor = FLinearColor::Red;

	UPROPERTY(BlueprintReadOnly, Interp, Category = Scanner)
	FLinearColor TraceHitColor = FLinearColor::Green;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = Scanner)
	float DrawTime=5.0;

	/* Set to true to automatically harvest all from scan results after each scan.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = AutoHarvest)
	bool bHarvestFromScanResults = false;
	
	/* Slots within 'ScanRadius' range of this component.*/
	UPROPERTY(BlueprintReadOnly, Category = Harvester)
	TArray<FTargetSlotInfo> ScanResults;


	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY()
	class UGenosGameInstanceSubsystem* GenOS;

	// Borrowed from KismetTraceUtils
	void DrawDebugCapsuleTraceMulti(const FVector& Start, const FVector& End);

public:	


};
