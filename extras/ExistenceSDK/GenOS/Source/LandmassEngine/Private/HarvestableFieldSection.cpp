// Fill out your copyright notice in the Description page of Project Settings.

#include "HarvestableFieldSection.h"
#include "HarvestableFieldsStats.h"
#include "HarvestableFieldAssetInstance.h"
#include "MasterHarvestableField.h"
#include "GenosGameInstanceSubsystem.h"
#include "InstancedFoliageActor.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

DECLARE_CYCLE_STAT(TEXT("HarvestRaw"), STAT_HF_HarvestRaw, STATGROUP_HarvestableFields);
DECLARE_CYCLE_STAT(TEXT("HarvestMultipleTargets"), STAT_HF_HarvestMultipleTargets, STATGROUP_HarvestableFields);
DECLARE_CYCLE_STAT(TEXT("HarvestTarget"), STAT_HF_HarvestTarget, STATGROUP_HarvestableFields);
DECLARE_CYCLE_STAT(TEXT("SetDynamicSlotInfo"), STAT_HF_SetDynamicSlotInfo, STATGROUP_HarvestableFields);
DECLARE_CYCLE_STAT(TEXT("HarvestAllPointsWithinBounds"), STAT_HF_HarvestAllPointsWithinBounds, STATGROUP_HarvestableFields);
DECLARE_CYCLE_STAT(TEXT("SpawnHarvestedProxy"), STAT_HF_SpawnHarvestedProxy, STATGROUP_HarvestableFields);
DECLARE_CYCLE_STAT(TEXT("GetClosestHarvestableInRange"), STAT_HF_GetClosestHarvestableInRange, STATGROUP_HarvestableFields);


AHarvestableFieldSection::AHarvestableFieldSection()
{
	PrimaryActorTick.bCanEverTick = true;
	bAllowTickBeforeBeginPlay = true;
	bReplicates = true;

	USceneComponent* SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = SceneComponent;
	RootComponent->CreationMethod = EComponentCreationMethod::Instance;
	RootComponent->Mobility = EComponentMobility::Static;

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	BoxComponent->CreationMethod = EComponentCreationMethod::Instance;
	BoxComponent->SetupAttachment(RootComponent);
	BoxComponent->SetCollisionObjectType(ECC_GameTraceChannel1);
}


void AHarvestableFieldSection::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetSubsystem<UGenosGameInstanceSubsystem>()->HarvestableFields.Add(this);

	if (Master == nullptr)
	{
		UE_LOG(LogHF, Error, TEXT("HF Section %s without a master."), *GetNameSafe(this));
		return;
	}

	DynamicSlots.Owner = this;
	RefreshAssets();
}


void AHarvestableFieldSection::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	GetWorld()->GetSubsystem<UGenosGameInstanceSubsystem>()->HarvestableFields.Remove(this);
}


void AHarvestableFieldSection::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHarvestableFieldSection, DynamicSlots);
}

void AHarvestableFieldSection::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Finalize asset updates if needed
	for (UHarvestableFieldAssetInstance* Asset : AssetInstances)
	{
		if (Asset->IsModified)
		{
			Asset->IsModified = false;
			Asset->FinalizeUpdate();
		}
	}
}


void AHarvestableFieldSection::OnConstruction(const FTransform& Transform)
{	
	Super::OnConstruction(Transform);
	RefreshAssets();	
}


void AHarvestableFieldSection::BeginDestroy()
{
	CleanupAssets();
	Super::BeginDestroy();
}

bool AHarvestableFieldSection::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	return true;
}

void AHarvestableFieldSection::KamoAfterLoad_Implementation()
{
	UE_CLOG(DynamicSlots.Items.Num(), LogHF, Display, TEXT("%s::KamoAfterLoad Updating %i dynamic slots."), *GetNameSafe(this), DynamicSlots.Items.Num());
	for (FDynamicSlotInfoItem Item : DynamicSlots.Items)
	{
		OnSlotChange(Item.Info);
	}
}


void AHarvestableFieldSection::RefreshAssets()
{
	CleanupAssets();

	for (int32 AssetIndex = 0; Master && AssetIndex < Master->Assets.Num(); ++AssetIndex)
	{
		UHarvestableFieldAssetInstance* AssetInstance = Master->Assets[AssetIndex]->CreateInstance(this);		
		AssetInstance->RefreshAsset(this, Slots.Items, AssetIndex);
		AssetInstances.Add(AssetInstance);
	}
}


void AHarvestableFieldSection::CleanupAssets()
{
	for (UHarvestableFieldAssetInstance* AssetInstance : AssetInstances)
	{
		AssetInstance->CleanupAsset();
	}

	AssetInstances.Reset();
}



void AHarvestableFieldSection::SuppressOccludedPoints()
{
	int SlotIndex = 0;
	for (auto Slot : Slots)
	{
		if (!IsSlotActive(SlotIndex))
		{
			continue;
		}

		TArray<FHitResult> OutHits;
		FVector Begin = Slot.Transform.GetLocation();
		Begin.Z += 250.0;
		FVector End = Slot.Transform.GetLocation();
		End.Z += 100000.0; // 10 km up
		FCollisionQueryParams CollisionParams(SCENE_QUERY_STAT(LineOfSight), true, this);
		CollisionParams.AddIgnoredActor(this);


		//GetWorld()->LineTraceMultiByChannel(OutHits, End, Begin, ECollisionChannel::ECC_WorldStatic, FCollisionQueryParams::DefaultQueryParam, FCollisionResponseParams::DefaultResponseParam);
		/*
		if (GetWorld()->LineTraceTestByChannel(Begin, End, ECC_WorldStatic))
		{
			Harvest(SlotIndex, TEXT("Suppressed"), 0.0, this);
		}
		*/

		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldStatic);
		GetWorld()->LineTraceMultiByObjectType(OutHits, Begin, End, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects), FCollisionQueryParams::DefaultQueryParam);

		for (FHitResult HitResult : OutHits)
		{

			//UE_LOG(LogHF, Display, TEXT("  Hit: %s"), *GetNameSafe(HitResult.GetActor()));
			if (GetNameSafe(HitResult.GetActor()) == FString(TEXT("SuppressorOfWorlds")))
			{
				UE_LOG(LogHF, Display, TEXT("  Hit: %s"), *GetNameSafe(HitResult.GetActor()));
				HarvestRaw(SlotIndex);
			}
		}


		SlotIndex++;
	}
}


int32 AHarvestableFieldSection::MapHitResultToSlotIndex(const FHitResult& HitResult)
{
	// Simply iterate through our assets and look for a hit.
	for (UHarvestableFieldAssetInstance* Asset : AssetInstances)
	{
		int32 SlotIndex = Asset->MapHitResultToSlotIndex(HitResult);
		if (SlotIndex >= 0)
		{
			return SlotIndex;
		}

	}

	return -1;
}


#if WITH_EDITOR
void AHarvestableFieldSection::AddToFoliage()
{
	AInstancedFoliageActor* FoliageActor = GetLevel()->InstancedFoliageActor.Get();
	TMap<UFoliageType*, FFoliageInfo*> FoliageTypeToFoliageInfo = FoliageActor->GetAllInstancesFoliageType();
	TMap<UStaticMesh*, UFoliageType*> MeshToFoliageType;

	for (auto Tuple : FoliageTypeToFoliageInfo)
	{
		UObject* Source = Tuple.Key->GetSource();
		UStaticMesh* Mesh = Cast<UStaticMesh>(Source);
		if (Mesh)
		{
			MeshToFoliageType.Add(Mesh, Tuple.Key);
		}
	}

	for (auto AssetInstance : AssetInstances)
	{
		if (AssetInstance)
		{
			UFoliageType** FoliageType = MeshToFoliageType.Find(AssetInstance->Asset->Mesh);
			if (FoliageType && *FoliageType)
			{
				FFoliageInfo** FoliageInfo = FoliageTypeToFoliageInfo.Find(*FoliageType);
				FFoliageInfo* FI = *FoliageInfo;

				TArray<FFoliageInstance> FoliageInstances;
				TArray<const FFoliageInstance*> Pointers;
				FoliageInstances.Reserve(AssetInstance->InstanceIndices.Num());
				Pointers.Reserve(AssetInstance->InstanceIndices.Num());
				for (auto Pair : AssetInstance->InstanceIndices)
				{
					FFoliageInstance& FoliageInstance = FoliageInstances.AddDefaulted_GetRef();
					Pointers.Emplace(&FoliageInstance);
					
					FoliageInstance.ProceduralGuid = FGuid::NewGuid();

					int32 SlotIndex = Pair.Key;
					const FTransform& Transform = Slots[SlotIndex].Transform;
					FoliageInstance.Location = Transform.GetLocation();
					FoliageInstance.Rotation = Transform.GetRotation().Rotator();
					
					// TODO: Figure out float vs double
					//FoliageInstance.DrawScale3D = Transform.GetScale3D();
					FoliageInstance.DrawScale3D.X = Transform.GetScale3D().X;
					FoliageInstance.DrawScale3D.Y = Transform.GetScale3D().Y;
					FoliageInstance.DrawScale3D.Z = Transform.GetScale3D().Z;
				}
				FI->AddInstances(*FoliageType, Pointers);
				FI->Refresh(true, false);
			}
		}
	}
}
#endif

FName AHarvestableFieldSection::GetAssetNameForSlot(int32 Index)
{
	return Master->Assets[Slots[Index].AssetIndex]->AssetName;
}

bool AHarvestableFieldSection::IsSlotActive(int32 Index)
{
	bool IsActive = true;
	for (auto Item : DynamicSlots.Items)
	{
		if (Item.Info.SlotIndex == Index && (Item.Info.bIsEmpty || Item.Info.Growth < 1.0f))
		{
			IsActive = false;
			break;
		}
	}
	return IsActive;
}

FVector AHarvestableFieldSection::GetSlotLocation(int32 Index)
{
	if (Index >= Slots.Num())
	{
		return FVector();
	}
	return Slots[Index].Transform.GetLocation();
}

FTransform AHarvestableFieldSection::GetSlotTransform(int32 Index)
{
	if (Index >= Slots.Num())
	{
		return FTransform();
	}
	return Slots[Index].Transform;
}

UStaticMesh* AHarvestableFieldSection::GetSlotMesh(int32 Index)
{
	if (Index >= Slots.Num())
	{
		return nullptr;
	}

	return Master->Assets[Slots[Index].AssetIndex]->Mesh;
}

FBox AHarvestableFieldSection::GetCollisionBounds(int32 Index)
{
	if (Index >= Slots.Num())
	{
		return FBox();
	}

	UStaticMesh* Mesh = Master->Assets[Slots[Index].AssetIndex]->Mesh;
	UBodySetup* BodySetup = Mesh->GetBodySetup();
	if (BodySetup->AggGeom.GetElementCount() > 0)
	{
		return BodySetup->AggGeom.CalcAABB(FTransform::Identity);
	} else
	{
		return Mesh->GetBoundingBox();
	}
}

TArray<int32> AHarvestableFieldSection::GetHarvestablesInRange(const FVector& Location, float Radius)
{
	TArray<int32> Result;

	const float RadiusSquared = Radius * Radius;
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (IsSlotActive(Index))
		{
			const FVector SlotLocation = Slots[Index].Transform.GetLocation();
			const float Distance = FVector::DistSquared(SlotLocation, Location);
			if (Distance <= RadiusSquared)
			{
				Result.Add(Index);
			}
		}
	}

	return Result;
}

int32 AHarvestableFieldSection::GetClosestHarvestableInRange(const FVector& Location, float Radius)
{
	SCOPE_CYCLE_COUNTER(STAT_HF_GetClosestHarvestableInRange);

	const float RadiusSquared = Radius * Radius;
	float MaxDistanceSquared = FLT_MAX;
	int32 ClosestIndex = -1;
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (IsSlotActive(Index))
		{
			const FVector SlotLocation = GetSlotLocation(Index);
			const float DistanceSquared = FVector::DistSquared(SlotLocation, Location);
			if (DistanceSquared <= MaxDistanceSquared)
			{
				MaxDistanceSquared = DistanceSquared;
				ClosestIndex = Index;
			}
		}
	}

	if (MaxDistanceSquared > RadiusSquared)
	{
		ClosestIndex = -1;
	}

	return ClosestIndex;
}

void AHarvestableFieldSection::HarvestRaw(int32 TargetIndex)
{
	SCOPE_CYCLE_COUNTER(STAT_HF_HarvestRaw);

	if (!HasAuthority())
	{
		UE_LOG(LogHF, Error, TEXT("Can't harvest on client"));
		return;
	}
	
	if (!IsSlotActive(TargetIndex))
	{
		UE_LOG(LogHF, Warning, TEXT("Slot %i has already been harvested in %s"), TargetIndex, *GetNameSafe(this));
		return;
	}

	FDynamicSlotInfo Info;
	Info.bIsEmpty = true;
	Info.SlotIndex = TargetIndex;

	SetDynamicSlotInfo(Info);
}


void AHarvestableFieldSection::HarvestMultipleTargets(const TArray<FTargetSlotInfo>& Targets)
{
	SCOPE_CYCLE_COUNTER(STAT_HF_HarvestMultipleTargets);

	for (const FTargetSlotInfo& Target : Targets)
	{
		if (Target.HarvestableFieldSection.IsValid())
		{
			Target.HarvestableFieldSection.Get()->HarvestTarget(Target);
		}
	}
}

void AHarvestableFieldSection::Harvest(int32 TargetIndex, TSubclassOf<AHarvestedProxy> HarvestedProxyClass)
{
	FTargetSlotInfo Target;
	Target.SlotIndex = TargetIndex;
	Target.StaticSlotInfo = Slots[TargetIndex];
	Target.HarvestedProxyClass = HarvestedProxyClass;
	Target.Asset = Master->Assets[Target.StaticSlotInfo.AssetIndex];
	Target.HarvestableFieldSection = this;
	HarvestTarget(Target);
}


void AHarvestableFieldSection::HarvestTarget(const FTargetSlotInfo& Target)
{
	SCOPE_CYCLE_COUNTER(STAT_HF_HarvestTarget);

	if (!HasAuthority())
	{
		UE_LOG(LogHF, Error, TEXT("Can't harvest on client"));
		return;
	}

	if (!IsSlotActive(Target.SlotIndex))
	{
		UE_LOG(LogHF, Warning, TEXT("Slot %i has already been harvested in %s"), Target.SlotIndex, *GetNameSafe(this));
		return;
	}

	FDynamicSlotInfo Info;
	Info.bIsEmpty = true;
	Info.SlotIndex = Target.SlotIndex;
	SetDynamicSlotInfo(Info);

	GetWorld()->GetSubsystem<UGenosGameInstanceSubsystem>()->OnSlotHarvested.Broadcast(Target);

	if (Target.HarvestedProxyClass)
	{
		SpawnHarvestedProxy(Target);
	}
}


void AHarvestableFieldSection::SetDynamicSlotInfo(const FDynamicSlotInfo& DynamicSlotInfo)
{
	SCOPE_CYCLE_COUNTER(STAT_HF_SetDynamicSlotInfo);

	if (DynamicSlotInfo.SlotIndex < 0 || DynamicSlotInfo.SlotIndex >= Slots.Num())
	{
		UE_LOG(LogHF, Error, TEXT("SetDynamicSlotInfo: SlotIndex %i is out of range. Slot count is %i."), DynamicSlotInfo.SlotIndex, Slots.Num());
		return;
	}

	FDynamicSlotInfo SlotInfoDefault; // To compare default values
	SlotInfoDefault.SlotIndex = DynamicSlotInfo.SlotIndex;
	int32 Index = -1; // Minus one means not found

	// Search for the index
	for (int32 i = 0; i < DynamicSlots.Items.Num(); i++)
	{
		if (DynamicSlots.Items[i].Info.SlotIndex == DynamicSlotInfo.SlotIndex)
		{
			Index = i;
			break;
		}
	}


	if (DynamicSlotInfo == SlotInfoDefault)
	{
		// Reset dynamic info to default values. Simply remove the entry if it exists
		if (Index != -1)
		{
			DynamicSlots.Items.RemoveAt(Index);
			DynamicSlots.MarkArrayDirty();

			if (HasAuthority())
			{
				OnSlotChange(DynamicSlotInfo);
			}
		}
	}
	else
	{
		if (Index != -1 && DynamicSlotInfo == DynamicSlots.Items[Index].Info)
		{
			// No change detected. Ignore.
		}
		else
		{
			// Update the item or add it in
			if (Index != -1)
			{
				DynamicSlots.Items[Index].Info = DynamicSlotInfo;
				DynamicSlots.MarkItemDirty(DynamicSlots.Items[Index]); // Mark the item as updated and dirty
			}
			else
			{
				FDynamicSlotInfoItem SlotInfo;
				SlotInfo.Info = DynamicSlotInfo;
				DynamicSlots.Items.Add(SlotInfo);
				DynamicSlots.MarkItemDirty(SlotInfo);
			}

			DynamicSlots.MarkArrayDirty();

			if (HasAuthority())
			{
				OnSlotChange(DynamicSlotInfo);
			}
		}
	}
}

void AHarvestableFieldSection::OnSlotChange(const FDynamicSlotInfo& DynamicSlotInfo)
{
	SCOPE_CYCLE_COUNTER(STAT_HF_OnSlotChange);

	if (GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		//UE_LOG(LogHF, Error, TEXT("%s: OnSlotChange callback not authorative."), *GetNameSafe(this));
	}

	const FStaticSlotInfo& Slot = Slots[DynamicSlotInfo.SlotIndex];
	if (!AssetInstances.IsValidIndex(Slot.AssetIndex))
	{
		return;
	}
	FTransform Transform = Slot.Transform;

	// Dynamic scaling is applied to the default scale factor
	if (DynamicSlotInfo.Growth != 1.0)
	{
		Transform.SetScale3D(Transform.GetScale3D() * DynamicSlotInfo.Growth);
	}

	if (DynamicSlotInfo.bIsEmpty)
	{
		Transform.SetScale3D(FVector(0, 0, 0));
	}

	if (AssetInstances[Slot.AssetIndex])
	{
		AssetInstances[Slot.AssetIndex]->UpdateInstanceTransform(DynamicSlotInfo.SlotIndex, Transform);
	}
	KamoDirty = true;
}


void AHarvestableFieldSection::HarvestAllPointsWithinBounds(const FBox& Box)
{
	SCOPE_CYCLE_COUNTER(STAT_HF_HarvestAllPointsWithinBounds);

	int SlotIndex = 0;
	for (auto Slot : Slots)
	{
		if (IsSlotActive(SlotIndex) && Box.IsInsideOrOn(Slot.Transform.GetLocation()))
		{
			HarvestRaw(SlotIndex);
		}
		SlotIndex++;
	}
}


void AHarvestableFieldSection::SpawnHarvestedProxy(const FTargetSlotInfo& Target)
{
	SCOPE_CYCLE_COUNTER(STAT_HF_SpawnHarvestedProxy);

	AHarvestedProxy* Proxy = GetWorld()->SpawnActorDeferred<AHarvestedProxy>(
		Target.HarvestedProxyClass,
		Target.StaticSlotInfo.Transform,
		nullptr, 
		Target.Instigator,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
		);
	Proxy->Target = Target;
	Proxy->FinishSpawning(Target.StaticSlotInfo.Transform, true);
}