// Fill out your copyright notice in the Description page of Project Settings.


#include "GenosHarvesterComponent.h"
#include "NavMesh/RecastNavMesh.h"


// Sets default values for this component's properties
UGenosHarvesterComponent::UGenosHarvesterComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGenosHarvesterComponent::BeginPlay()
{
	Super::BeginPlay();
}


void UGenosHarvesterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}


void UGenosHarvesterComponent::OnHarvestableActorsReady(UGenosGameInstanceSubsystem* GenosSubSystem)
{

}


void UGenosHarvesterComponent::HarvestWithinBounds() const
{
	if (GetOwner()->HasAuthority())
	{
		UE_LOG(LogHF, Error, TEXT("%s:HarvestWithinBounds() NOT IMPLEMENTED YET"), *GetNameSafe(this));
	}
}


// Called every frame
void UGenosHarvesterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
	if (ShouldCheck)
	{
		FTransform CurrentTransform = GetOwner()->GetTransform();
		if (CurrentTransform.GetLocation() != LastTransform.GetLocation() || CurrentTransform.GetRotation() != LastTransform.GetRotation())
		{
			LastTransform = CurrentTransform;
			HarvestWithinBounds();
		}

		if (GetOwner()->GetRootComponent()->Mobility != EComponentMobility::Movable)
		{
			ShouldCheck = false;
		}
	}
}

