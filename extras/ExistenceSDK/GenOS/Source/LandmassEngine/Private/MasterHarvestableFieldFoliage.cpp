// 


#include "MasterHarvestableFieldFoliage.h"

#include "HarvestableFieldsStats.h"
#include "Foliage/Public/InstancedFoliageActor.h"

#if WITH_EDITOR
#include "EditorAssetLibrary.h"
#include "AssetTypeActions_Base.h"
#include "LandmassEngineEditorModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ActorPartition/ActorPartitionSubsystem.h"


#include "Kismet/GameplayStatics.h"

class UHarvestableFieldAssetFactory;

int32 AMasterHarvestableFieldFoliage::FindOrAddAsset(UFoliageType* FoliageType, const TArray<FAssetData>& AssetDataList)
{
	UFoliageType_InstancedStaticMesh* MeshFoliageType = Cast<UFoliageType_InstancedStaticMesh>(FoliageType);
	if (!MeshFoliageType)
	{
		return -1;
	}
	UStaticMesh* StaticMesh = MeshFoliageType->Mesh;
	if (!StaticMesh)
	{
		return -1;
	}

	// See if we already have this mesh in our asset list
	for (int32 Index = 0; Index < Assets.Num(); ++Index)
	{
		UHarvestableFieldAsset* Asset = Cast<UHarvestableFieldAsset>(Assets[Index]);
		if (Asset && Asset->Mesh == StaticMesh && Asset->OverrideMaterials == MeshFoliageType->OverrideMaterials)
		{
			if (!Asset->ConvertFromFoliage)
			{
				UE_LOG(LogHF, Display, TEXT("Skipping foliage type %s"), *GetNameSafe(Asset));
				return -1;
			}
			return Index;
		}
	}

	// See if we find a match in available HF assets
	for (const FAssetData& AssetData : AssetDataList)
	{
		UHarvestableFieldAsset* HFCandidate = Cast<UHarvestableFieldAsset>(AssetData.GetAsset());
		if (HFCandidate && HFCandidate->Mesh == StaticMesh && HFCandidate->OverrideMaterials == MeshFoliageType->OverrideMaterials)
		{
			if (!HFCandidate->ConvertFromFoliage)
			{
				UE_LOG(LogHF, Display, TEXT("Skipping foliage type %s"), *GetNameSafe(HFCandidate));
				return -1;
			}
			return Assets.Add(HFCandidate);
		}
	}

	// Let's create a new HF asset. We just use the mesh name but change the prefix if applicable.
	if (!UEditorAssetLibrary::MakeDirectory(PackagePath.Path))
	{
		UE_LOG(LogHF, Warning, TEXT("%s: Can't make directory %s so I Can't create assets."), *GetNameSafe(this), *PackagePath.Path);
		return -1;
	}

	FString AssetName = StaticMesh->GetName();
	AssetName.RemoveFromStart(TEXT("SM_"));
	AssetName.InsertAt(0, TEXT("HF_"));

	FString Path = PackagePath.Path;
	Path /= AssetName;

	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
	FString NewPackageName;
	UFactory* Factory = Cast<UFactory>(GetClass()->GetDefaultObject());
	FString PackageName = PackagePath.Path;
	PackageName /= AssetName;
	FString NewAssetName;	AssetTools.CreateUniqueAssetName(PackageName, FString(), NewPackageName, NewAssetName);
	UObject* AssetObject = AssetTools.CreateAsset(NewAssetName, PackagePath.Path, UHarvestableFieldAsset::StaticClass(), Factory);


	//UObject* AssetObject = FLandmassEngineEditorModule::Get().CreateAsset(UHarvestableFieldAsset::StaticClass(), FPackageName::GetLongPackagePath(Path), FName(AssetName));
	UHarvestableFieldAsset* Asset = Cast<UHarvestableFieldAsset>(AssetObject);
	Asset->Mesh = StaticMesh;
	Asset->OverrideMaterials = MeshFoliageType->OverrideMaterials;
	Asset->StartCullDistance = FoliageType->CullDistance.Min;
	Asset->EndCullDistance = FoliageType->CullDistance.Max;
	return Assets.Add(Asset);
}

bool AMasterHarvestableFieldFoliage::IsDuplicate(FStaticSlotInfo SlotInfo, int32 AssetIndex, int32 Index)
{
	for (const FStaticSlotInfo& CheckSlot : Slots)
	{
		if (SlotInfo == CheckSlot)
		{
			UE_LOG(LogHF, Warning, TEXT("Foliage %i:%i in %s has already been added to slots. Ignoring it."), AssetIndex, Index, *GetNameSafe(this));
			return true;
		}
	}
	return false;
}

void AMasterHarvestableFieldFoliage::PrepareSlots()
{
	// for comparison
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AInstancedFoliageActor::StaticClass(), OutActors);

	FBox BigBox;
	FBox BiggerBox = BigBox.ExpandBy(250000.0);

	if (UActorPartitionSubsystem* ActorPartitionSubsystem = GetWorld()->GetSubsystem<UActorPartitionSubsystem>())
	{
		auto IFAOperation = [&](APartitionActor* Actor)
		{
			if (AInstancedFoliageActor* FoliageActor = Cast<AInstancedFoliageActor>(Actor))
			{
				PrepareSlotsForFoliageActor(FoliageActor);
			}

			return true;
		};

		ActorPartitionSubsystem->ForEachRelevantActor(AInstancedFoliageActor::StaticClass(), BiggerBox, IFAOperation);
	}
	else if (AInstancedFoliageActor* FoliageActor = GetLevel()->InstancedFoliageActor.Get())
	{
		PrepareSlotsForFoliageActor(FoliageActor);
	}
	else
	{
		UE_LOG(LogHF, Error, TEXT("Can't prepare slots - No foliage actors available."));
	}
}


void AMasterHarvestableFieldFoliage::PrepareSlotsForFoliageActor(AInstancedFoliageActor* FoliageActor)
{
	// Prepare a list of current HF assets
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetDataList;
	const UClass* Class = UHarvestableFieldAsset::StaticClass();
	AssetRegistryModule.Get().GetAssetsByClass(Class->GetFName(), AssetDataList);

	for (const TTuple<UFoliageType*, TUniqueObj<FFoliageInfo>>& It : FoliageActor->GetFoliageInfos())
	{
		int32 AssetIndex = FindOrAddAsset(It.Key, AssetDataList);
		if (AssetIndex < 0)
		{
			continue;
		}
		
		int32 Num = It.Value->Instances.Num();
		if (Num == 0)
		{
			continue;
		}

		TArray<int32> ToRemove;
		ToRemove.Reserve(Num);
		Slots.Reserve(Slots.Num() + Num);
		
		for (int32 Index = 0; Index < Num; ++Index)
		{
			const FFoliageInstance& FoliageInstance = It.Value->Instances[Index];
			ToRemove.Add(Index);

			// TODO: Figure out float vs double
			FVector3d DrawScale3D;
			DrawScale3D.X = FoliageInstance.DrawScale3D.X;
			DrawScale3D.Y = FoliageInstance.DrawScale3D.Y;
			DrawScale3D.Z = FoliageInstance.DrawScale3D.Z;

			if (IgnoreDuplicateSlots)
			{
				FStaticSlotInfo SlotInfo;
				SlotInfo.Transform = FTransform(FoliageInstance.Rotation, FoliageInstance.Location, DrawScale3D);
				SlotInfo.AssetIndex = AssetIndex;
				bool DuplicateFound = false;
				for (const FStaticSlotInfo& CheckSlot : Slots)
				{
					if (SlotInfo == CheckSlot)
					{
						UE_LOG(LogHF, Warning, TEXT("Foliage %i:%i in %s has already been added to slots. Ignoring it."), AssetIndex, Index, *GetNameSafe(this));
						DuplicateFound = true;
						break;
					}
				}
				if (!DuplicateFound)
				{
					Slots.Add(SlotInfo);
				}
			
			}
			else
			{
				Slots.Emplace(FoliageInstance.Rotation, FoliageInstance.Location, DrawScale3D, AssetIndex);
			}
		}

		UE_LOG(LogHF, Display, TEXT("Added %d instances for %s"), Num, *GetNameSafe(Assets[AssetIndex]));
				
		const_cast<FFoliageInfo*>(&It.Value.Get())->RemoveInstances(ToRemove, true);

	}
	UE_CLOG(Slots.Num() == 0, LogHF, Warning, TEXT("The foliage didn't contain any instances. Note that foliage in World Partition level is not supported yet!"));
}

void AMasterHarvestableFieldFoliage::ConvertToFoliage()
{
	for (auto Section : GetSections())
	{
		Section->AddToFoliage();
	}
	ClearSections();
	Slots.Reset();
}
#endif
