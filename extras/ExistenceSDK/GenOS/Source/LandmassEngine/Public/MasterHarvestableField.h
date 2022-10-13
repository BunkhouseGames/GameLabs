// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HarvestableFieldSection.h"
#include "HarvestableFieldAsset.h"
#include "StaticSlotInfo.h"
#include "GameFramework/Actor.h"
#include "MasterHarvestableField.generated.h"

UCLASS(Abstract)
class LANDMASSENGINE_API AMasterHarvestableField : public AActor
{
	GENERATED_BODY()

public:
	AMasterHarvestableField();

	virtual void PostLoad() override;

	/** Cardinal name of this harvestable field which must be unique within the project. External tools and data structures
	use this name to reference this asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName FieldName;

	// Static Slots Info
	UPROPERTY(BlueprintReadOnly, Transient, DuplicateTransient, NonTransactional)
	TArray<FStaticSlotInfo> Slots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxNumSlotsPerSection = 2000;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<UHarvestableFieldAsset*> Assets;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RegrowRate = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RegrowTickRate = -1.0f;

	static void AddKamoIdTag(AHarvestableFieldSection* Section);
	static void GetSlotBounds(const TArray<FStaticSlotInfo>& SlotArray, FVector& MinBounds, FVector& MaxBounds);
	

#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Debugging")
	virtual void PrepareSlots() {}

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Authoring")
	void CreateSections();

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Authoring")
	void ClearSections();
	
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Debugging")
	void StartSplit();

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Debugging")
	void ContinueSplit();

	TArray<AHarvestableFieldSection*> GetSections() const;

#endif
	
protected:
	

#if WITH_EDITOR
	void SplitSlots(const TArray<FStaticSlotInfo>& Input, TArray<FStaticSlotInfo>& Sorted, TArray<FStaticSlotInfo>& Upper);
	void SplitSlotsRecursive(const TArray<FStaticSlotInfo>& Input, TArray<TArray<FStaticSlotInfo>>& OutputSlots);
	AHarvestableFieldSection* CreateFieldSection(const TArray<FStaticSlotInfo>& SlotsForSection);

	TArray<TArray<FStaticSlotInfo>> SectionSlotsForVisualDebugging;
	void DrawDebugBoundingBox(const TArray<FStaticSlotInfo>& SlotsArray, const FColor& Color);
#endif
};
