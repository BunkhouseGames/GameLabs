// Copyright Arctic Theory, Ehf. All Rights Reserved.


#include "GenosBaseHarvesterComponent.h"
#include "GenosGameInstanceSubsystem.h"
#include "HarvestableFieldSection.h"
#include "DrawDebugHelpers.h"


DECLARE_CYCLE_STAT(TEXT("PostInitialize"), STAT_HF_PostInitialize, STATGROUP_HarvestableFields);
DECLARE_CYCLE_STAT(TEXT("SuppressSlotsBelowActor"), STAT_HF_SuppressSlotsBelowActor, STATGROUP_HarvestableFields);
DECLARE_CYCLE_STAT(TEXT("ScanForHarvestables"), STAT_HF_ScanForHarvestables, STATGROUP_HarvestableFields);
DECLARE_CYCLE_STAT(TEXT("ProcessHitResults"), STAT_HF_ProcessHitResults, STATGROUP_HarvestableFields);
DECLARE_CYCLE_STAT(TEXT("HarvestFromScanResults"), STAT_HF_HarvestFromScanResults, STATGROUP_HarvestableFields);


// Sets default values for this component's properties
UGenosBaseHarvesterComponent::UGenosBaseHarvesterComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void UGenosBaseHarvesterComponent::BeginPlay()
{
	Super::BeginPlay();
	GenOS = GetOwner()->GetWorld()->GetSubsystem<UGenosGameInstanceSubsystem>();
	if (GenOS)
	{
		GenOS->HarvesterComponents.Add(this);
	}
}


void UGenosBaseHarvesterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (GenOS)
	{
		GenOS->HarvesterComponents.Remove(this);
	}
}


// Called every frame
void UGenosBaseHarvesterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		LastScan += DeltaTime;
		if (ScanInterval >= 0.0 && LastScan > ScanInterval)
		{
			LastScan = 0.0;
			ScanForHarvestables();

			if (bHarvestFromScanResults)
			{
				HarvestFromScanResults();
			}
		}
	}
}


void UGenosBaseHarvesterComponent::PostInitialize()
{
	SCOPE_CYCLE_COUNTER(STAT_HF_PostInitialize);

	if (SuppressSlots)
	{
		SuppressSlotsBelowActor();
	}
}



void UGenosBaseHarvesterComponent::SuppressSlotsBelowActor()
{
	SCOPE_CYCLE_COUNTER(STAT_HF_SuppressSlotsBelowActor);

	// For the time being, let's nuke everything within XY bounds	
	FVector Origin, Extent;
	GetOwner()->GetActorBounds(true, Origin, Extent, false);
	Extent.Z = 1000000.0;
	FBox Box = FBox::BuildAABB(Origin, Extent);

	for (AHarvestableFieldSection* Section : GenOS->GetHarvestablesInRegion(Box))
	{
		Section->HarvestAllPointsWithinBounds(Box);
	}
}


static void DrawDebugCapsuleTraceMulti(const UWorld* World, const FVector& Start, const FVector& End, float Radius, float HalfHeight, bool bHit, const TArray<FHitResult>& OutHits, FLinearColor TraceColor, FLinearColor TraceHitColor, float LifeTime);


void UGenosBaseHarvesterComponent::ScanForHarvestables()
{
	SCOPE_CYCLE_COUNTER(STAT_HF_ScanForHarvestables);

	if (GetOwner()->GetWorld() == nullptr)
	{
		return;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ScanHarvestables), bTraceComplex);
	Params.bReturnPhysicalMaterial = true;
	Params.bReturnFaceIndex = true;
	Params.AddIgnoredActor(GetOwner());
	TArray<FHitResult> OutHits;

	FVector Begin = GetComponentLocation();
	FVector End = Begin;
	if (Distance > 0.0)
	{
		FVector ForwardVector = GetForwardVector();
		ForwardVector *= Distance;
		End = Begin + ForwardVector;
	}
	else
	{
		End = Begin;
	}

	bool const bHit = GetOwner()->GetWorld()->SweepMultiByProfile(
		OutHits,
		Begin, End, FQuat::Identity,
		ScanProfile.Name,
		FCollisionShape::MakeCapsule(Radius, HalfHeight),
		Params
	);

	ScanResults.Reset();
	ProcessHitResults(OutHits, ScanResults, this);

	if (bDrawDebugLines)
	{
		DrawDebugCapsuleTraceMulti(Begin, End);
	}

}


void UGenosBaseHarvesterComponent::ProcessHitResults(const TArray<FHitResult> HitResults, TArray<FTargetSlotInfo>& TargetSlots, UGenosBaseHarvesterComponent* Harvester)
{
	SCOPE_CYCLE_COUNTER(STAT_HF_ProcessHitResults);

	// Prepare list of valid hit results
	for (const FHitResult& HitResult : HitResults)
	{
		if (HitResult.Item < 0)
		{
			// Cannot be mapped to a slot
			continue;
		}

		AHarvestableFieldSection* HF = Cast<AHarvestableFieldSection>(HitResult.GetActor());
		if (HF == nullptr)
		{
			// Not a harvestable field.
			continue;
		}

		// Iterate through the assets and look for a hit.
		int32 SlotIndex = -1;
		int32 AssetIndex;
		for (AssetIndex = 0; AssetIndex < HF->AssetInstances.Num(); AssetIndex++)
		{
			SlotIndex = HF->AssetInstances[AssetIndex]->MapHitResultToSlotIndex(HitResult);
			if (SlotIndex >= 0)
			{
				break;
			}
		}

		if (SlotIndex == -1)
		{
			// No hit
			UE_LOG(LogHF, Warning, TEXT("Scan hit %i in component %s was not mapped to any slot. Consider improving the trace query."), HitResult.Item, *GetNameSafe(Harvester));
			continue;
		}

		FTargetSlotInfo Target;
		Target.HitResult = HitResult;
		Target.Instigator = Harvester ? Cast<APawn>(Harvester->GetOwner()) : nullptr;
		Target.HasSlotInfo = true;
		Target.HarvestableFieldSection = HF;
		Target.AssetInstance = HF->AssetInstances[AssetIndex];
		Target.Asset = HF->AssetInstances[AssetIndex]->Asset;
		Target.StaticSlotInfo = HF->Slots[SlotIndex];

		// Search for the dynamic slot
		for (FDynamicSlotInfoItem& DynamicSlotInfoItem : HF->DynamicSlots.Items)
		{
			if (DynamicSlotInfoItem.Info.SlotIndex == SlotIndex)
			{
				Target.DynamicSlotInfo = DynamicSlotInfoItem.Info;
				break;
			}
		}

		Target.SlotIndex = SlotIndex;
		Target.HarvestedProxyClass = Target.AssetInstance->Asset->HarvestedProxyClass;
		Target.Harvester = Harvester;

		TargetSlots.Add(Target);
	}
}


void UGenosBaseHarvesterComponent::HarvestFromScanResults(int32 ItemLimit)
{
	SCOPE_CYCLE_COUNTER(STAT_HF_HarvestFromScanResults);

	if (ScanResults.Num() == 0)
	{
		return;
	}

	if (ItemLimit > 0 && ItemLimit < ScanResults.Num())
	{
		ScanResults.SetNum(ItemLimit);
	}

	AHarvestableFieldSection::HarvestMultipleTargets(ScanResults);
}


void UGenosBaseHarvesterComponent::DrawDebugCapsuleTraceMulti(const FVector& Start, const FVector& End)
{
#if ENABLE_DRAW_DEBUG
	bool bPersistent = false;
	const UWorld* World = GetOwner()->GetWorld();
	
	float LifeTime = DrawTime;
	
	if (ScanInterval == 0.0)
	{
		DrawTime = 0.0;
	}
	else if (ScanInterval > DrawTime)
	{
		DrawTime = ScanInterval;
	}

	if (ScanResults.Num() && ScanResults.Last().HitResult.bBlockingHit)
	{
		// Red up to the blocking hit, green thereafter
		FVector const BlockingHitPoint = ScanResults.Last().HitResult.Location;
		::DrawDebugCapsule(World, Start, HalfHeight, Radius, FQuat::Identity, TraceColor.ToFColor(true), bPersistent, LifeTime);
		::DrawDebugCapsule(World, BlockingHitPoint, HalfHeight, Radius, FQuat::Identity, TraceColor.ToFColor(true), bPersistent, LifeTime);
		::DrawDebugLine(World, Start, BlockingHitPoint, TraceColor.ToFColor(true), bPersistent, LifeTime);

		::DrawDebugCapsule(World, End, HalfHeight, Radius, FQuat::Identity, TraceHitColor.ToFColor(true), bPersistent, LifeTime);
		::DrawDebugLine(World, BlockingHitPoint, End, TraceHitColor.ToFColor(true), bPersistent, LifeTime);
	}
	else
	{
		// no hit means all red
		::DrawDebugCapsule(World, Start, HalfHeight, Radius, FQuat::Identity, TraceColor.ToFColor(true), bPersistent, LifeTime);
		::DrawDebugCapsule(World, End, HalfHeight, Radius, FQuat::Identity, TraceColor.ToFColor(true), bPersistent, LifeTime);
		::DrawDebugLine(World, Start, End, TraceColor.ToFColor(true), bPersistent, LifeTime);
	}

	// draw hits
	for (const FTargetSlotInfo& ScanResult : ScanResults)
	{
		static const float KISMET_TRACE_DEBUG_IMPACTPOINT_SIZE = 16.f;
		::DrawDebugPoint(World, ScanResult.HitResult.ImpactPoint, KISMET_TRACE_DEBUG_IMPACTPOINT_SIZE, (ScanResult.HitResult.bBlockingHit ? TraceColor.ToFColor(true) : TraceHitColor.ToFColor(true)), bPersistent, LifeTime);
	}
#endif // ENABLE_DRAW_DEBUG
}


