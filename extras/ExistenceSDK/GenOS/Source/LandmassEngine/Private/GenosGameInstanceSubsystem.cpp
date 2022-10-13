// Fill out your copyright notice in the Description page of Project Settings.


#include "GenosGameInstanceSubsystem.h"

#include "GenosHarvesterComponent.h"
#include "HarvestableFieldSection.h"
#include "HarvestableFieldsStats.h"
#include "Kismet/GameplayStatics.h"


DECLARE_CYCLE_STAT(TEXT("GetHarvestablesInRegion"), STAT_HF_GetHarvestablesInRegion, STATGROUP_HarvestableFields);



void UGenosGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}


void UGenosGameInstanceSubsystem::Deinitialize()
{
}


void UGenosGameInstanceSubsystem::HandlePostLoadMap(UWorld* InLoadedWorld)
{
	for (UGenosBaseHarvesterComponent* Harvester : HarvesterComponents)
	{
		// Signal readiness of harvestable fields subsystem
		Harvester->PostInitialize();
	}
}


TArray<AHarvestableFieldSection*> UGenosGameInstanceSubsystem::GetHarvestablesInRegion(const FBox& Box)
{
	SCOPE_CYCLE_COUNTER(STAT_HF_GetHarvestablesInRegion);

	TArray<AHarvestableFieldSection*> Harvestables;

	for (AHarvestableFieldSection* Section : HarvestableFields)
	{
		FVector HFOrigin, HFExtent;
		Section->GetActorBounds(true, HFOrigin, HFExtent, false);
		FBox HFBox = FBox::BuildAABB(HFOrigin, HFExtent);
		if (HFBox.Intersect(Box))
		{
			Harvestables.Add(Section);
		}
	}

	return Harvestables;
}

FHarvestable UGenosGameInstanceSubsystem::GetClosestHarvestableInRange(FVector Location, float Radius)
{
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Shape;
	Shape.SetSphere(Radius);

	FHarvestable Closest;
	
	GetWorld()->OverlapMultiByChannel(Overlaps, Location, FQuat::Identity, ECollisionChannel::ECC_WorldDynamic, Shape);
	float ClosestDistance = FLT_MAX;
	for (auto Overlap : Overlaps)
	{
		AHarvestableFieldSection* Section = Cast<AHarvestableFieldSection>(Overlap.GetActor());
		if (Section)
		{
			int32 Index = Section->GetClosestHarvestableInRange(Location, Radius);
			if (Index >= 0)
			{
				auto SlotLocation = Section->GetSlotLocation(Index);
				auto Distance = (SlotLocation - Location).Length();
				if (Distance < ClosestDistance)
				{
					Closest.Section = Section;
					Closest.Index = Index;
					ClosestDistance = Distance;
				}
			}
		}
	}
	return Closest;
}

FHarvestable UGenosGameInstanceSubsystem::GetClosestHarvestableInFront(FVector Location, FVector Direction, float Range, FName AssetName)
{
	TArray<FHitResult> Hits;
	const FVector End = Location + Direction * Range;
	FCollisionObjectQueryParams ObjectQueryParams(FCollisionObjectQueryParams::AllObjects);
	FCollisionQueryParams QueryParams;

	constexpr float SphereWidth = 50.0f;
	const FVector Start = Location + Direction * SphereWidth;
	FCollisionShape Shape = FCollisionShape::MakeSphere(SphereWidth);
	if (GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECollisionChannel::ECC_WorldDynamic, Shape))
	{
		for (auto Hit : Hits)
		{
			FHarvestable Closest;
			Closest.Section = Cast<AHarvestableFieldSection>(Hit.GetActor());
			if (Closest.Section)
			{
				Closest.Index = Closest.Section->MapHitResultToSlotIndex(Hit);
				if (Closest.Index == -1)
				{
					Closest.Section = nullptr;
				}
			}
			if (Closest.Section)
			{
				if (AssetName.IsNone() || AssetName.IsEqual(Closest.Section->GetAssetNameForSlot(Closest.Index)))
				{
					return Closest;
				}
			}

			if (AHarvestedProxy* Proxy = Cast<AHarvestedProxy>(Hit.GetActor()))
			{
				Closest.Section = Proxy->Target.HarvestableFieldSection.Get();
				Closest.Index = Proxy->Target.SlotIndex;
				Closest.Proxy = Proxy;
				if (AssetName.IsNone() || AssetName.IsEqual(Closest.Section->GetAssetNameForSlot(Closest.Index)))
				{
					return Closest;
				}
			}
		}
	}

	return FHarvestable();
}

