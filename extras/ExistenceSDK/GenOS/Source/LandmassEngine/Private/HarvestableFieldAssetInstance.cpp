// Fill out your copyright notice in the Description page of Project Settings.


#include "HarvestableFieldAssetInstance.h"

#include "HarvestableFieldAsset.h"
#include "InstanceCollisionComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "HarvestableFieldsStats.h"


DECLARE_CYCLE_STAT(TEXT("FinalizeUpdate"), STAT_HF_FinalizeUpdate, STATGROUP_HarvestableFields);
DECLARE_CYCLE_STAT(TEXT("MapHitResultToSlotIndex"), STAT_HF_MapHitResultToSlotIndex, STATGROUP_HarvestableFields);
DECLARE_CYCLE_STAT(TEXT("RemoveAndDestroyComponent"), STAT_HF_RemoveAndDestroyComponent, STATGROUP_HarvestableFields);


void UHarvestableFieldAssetInstance::RefreshAsset(AActor* HarvestableField, const TArray<FStaticSlotInfo>& Slots, int32 AssetIndex)
{
	SCOPE_CYCLE_COUNTER(STAT_HF_RefreshAsset);

	if (!HarvestableField->GetWorld() || Asset == nullptr)
	{
		return;
	}

	InstanceIndices.Empty();

	TArray<FTransform> TransformsForCollision;
	for (int SlotIndex = 0; SlotIndex < Slots.Num(); SlotIndex++)
	{
		const FStaticSlotInfo& Slot = Slots[SlotIndex];
		if (Slot.AssetIndex == AssetIndex)
		{
			InstanceIndices.Add(SlotIndex, InstanceIndices.Num());
			TransformsForCollision.Add(Slot.Transform);
		}
	}

	if (InstanceIndices.IsEmpty())
	{
		return;
	}

	const FName Name(FString::Printf(TEXT("%s_%d_HarvestableFieldAsset"), *Asset->AssetName.ToString(), AssetIndex));
	TArray<UActorComponent*> CurrentMeshComponents = HarvestableField->GetComponentsByTag(UHierarchicalInstancedStaticMeshComponent::StaticClass(), Name);

	CleanupAsset();

	MeshComponent = NewObject<UHierarchicalInstancedStaticMeshComponent>(
		HarvestableField,
		UHierarchicalInstancedStaticMeshComponent::StaticClass(),
		MakeUniqueObjectName(HarvestableField, UHierarchicalInstancedStaticMeshComponent::StaticClass(), Name),
		RF_Transient | RF_DuplicateTransient | RF_TextExportTransient
		);

	// Change the creation method so the component is listed in the details panels
	MeshComponent->CreationMethod = EComponentCreationMethod::Instance;
	MeshComponent->SetMobility(EComponentMobility::Stationary);
	MeshComponent->SetAbsolute(true, true, true);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCollisionProfileName(TEXT("NoCollision"));
	MeshComponent->ComponentTags.Add(Name);
	MeshComponent->SetStaticMesh(Asset->Mesh);
	MeshComponent->SetCullDistances(Asset->StartCullDistance, Asset->EndCullDistance);
	MeshComponent->SetIsReplicated(true);

	HarvestableField->AddOwnedComponent(MeshComponent);
	MeshComponent->RegisterComponent();



	MeshComponent->SetStaticMesh(nullptr);
	MeshComponent->SetStaticMesh(Asset->Mesh);
	if (!Asset->OverrideMaterials.IsEmpty())
	{
		for (int Ix = 0; Ix < Asset->OverrideMaterials.Num(); Ix++)
		{
			MeshComponent->SetMaterial(Ix, Asset->OverrideMaterials[Ix]);
		}
	}

	for (auto Pair : InstanceIndices)
	{
		const FTransform& Transform = Slots[Pair.Key].Transform;
		MeshComponent->AddInstance(Transform, true);
	}
	INC_DWORD_STAT_BY(STAT_HF_NumHarvestableInstances, MeshComponent->GetInstanceCount());


	CollisionComponent = NewObject<UInstanceCollisionComponent>(
		HarvestableField,
		UInstanceCollisionComponent::StaticClass(),
		MakeUniqueObjectName(HarvestableField, UInstanceCollisionComponent::StaticClass(), Name),
		RF_Transient | RF_DuplicateTransient | RF_TextExportTransient
		);
	CollisionComponent->CreationMethod = EComponentCreationMethod::Instance;
	HarvestableField->AddOwnedComponent(CollisionComponent);
	CollisionComponent->RegisterComponent();
	CollisionComponent->StaticMesh = Asset->Mesh;
	FName CollisionProfile = Asset->OverrideCollisionProfile ? Asset->CollisionProfile.Name : FName();
	CollisionComponent->SetInstances(TransformsForCollision, MeshComponent, CollisionProfile);
}


void UHarvestableFieldAssetInstance::UpdateInstanceTransform(int32 SlotIndex, const FTransform& Transform)
{
	SCOPE_CYCLE_COUNTER(STAT_HF_UpdateInstanceTransform);

	int32* InstanceIndex = InstanceIndices.Find(SlotIndex);
	if (!InstanceIndex)
	{
		return;
	}

	if (!MeshComponent->UpdateInstanceTransform(*InstanceIndex, Transform, true, true, true))
	{
		UE_LOG(LogHF, Display, TEXT("In %s UpdateInstanceTransform failed for index %i"), *GetNameSafe(this), *InstanceIndex);
	}

	bool bIsEnabled = !Transform.GetScale3D().IsNearlyZero();
	CollisionComponent->SetInstanceEnabled(*InstanceIndex, bIsEnabled);
	IsModified = true; // Schedule a FinalizeUpdate callback
}


void UHarvestableFieldAssetInstance::FinalizeUpdate()
{
	SCOPE_CYCLE_COUNTER(STAT_HF_FinalizeUpdate);
	MeshComponent->BuildTreeIfOutdated(true, true);
}


void UHarvestableFieldAssetInstance::CleanupAsset()
{
	SCOPE_CYCLE_COUNTER(STAT_HF_CleanupAsset);

	if (MeshComponent)
	{
		DEC_DWORD_STAT_BY(STAT_HF_NumHarvestableInstances, MeshComponent->GetInstanceCount());
	}

	// Reset CollisionComponent and MeshComponent
	RemoveAndDestroyComponent(MeshComponent);
	MeshComponent = nullptr;

	RemoveAndDestroyComponent(CollisionComponent);
	CollisionComponent = nullptr;
}


int32 UHarvestableFieldAssetInstance::MapHitResultToSlotIndex(const FHitResult& HitResult)
{
	SCOPE_CYCLE_COUNTER(STAT_HF_MapHitResultToSlotIndex);

	// If the component in HitResult is our mesh component we search for the InstanceIndex
	// in our InstanceIndices map.
	if (HitResult.GetComponent() == MeshComponent)
	{
		for (auto pair : InstanceIndices)
		{
			if (pair.Value == HitResult.Item)
			{
				return pair.Key;
			}
		}
	}

	return -1;
}


bool UHarvestableFieldAssetInstance::RemoveAndDestroyComponent(UObject* InComponent)
{
	SCOPE_CYCLE_COUNTER(STAT_HF_RemoveAndDestroyComponent);

	USceneComponent* SceneComponent = Cast<USceneComponent>(InComponent);
	if (::IsValid(SceneComponent))
	{
		// Remove from the HoudiniAssetActor
		if (SceneComponent->GetOwner())
			SceneComponent->GetOwner()->RemoveOwnedComponent(SceneComponent);

		SceneComponent->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		SceneComponent->UnregisterComponent();
		SceneComponent->DestroyComponent();

		return true;
	}

	return false;
}

