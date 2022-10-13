// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenosBaseHarvesterComponent.h"
#include "GenosHarvesterComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LANDMASSENGINE_API UGenosHarvesterComponent : public UGenosBaseHarvesterComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGenosHarvesterComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void OnHarvestableActorsReady(class UGenosGameInstanceSubsystem*);


	void HarvestWithinBounds() const;
	FTransform LastTransform;
	bool ShouldCheck=false;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	
		
};
