// Fill out your copyright notice in the Description page of Project Settings.


#include "MasterHarvestableField.h"

#include "DrawDebugHelpers.h"
#include "HarvestableFieldSection.h"
#include "Components/BillboardComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "VisualLogger/VisualLogger.h"

// Sets default values
AMasterHarvestableField::AMasterHarvestableField()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = SceneComponent;
	RootComponent->CreationMethod = EComponentCreationMethod::Instance;
	RootComponent->Mobility = EComponentMobility::Static;

#if WITH_EDITORONLY_DATA
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> TerrainTexture;
		FConstructorStatics()
			: TerrainTexture(TEXT("/Engine/EditorResources/S_Terrain"))
		{
		}
	};

	UBillboardComponent* SpriteComponent = CreateDefaultSubobject<UBillboardComponent>(FName(TEXT("Sprite")));	
	
	if (SpriteComponent)
	{
		SpriteComponent->SetFlags(RF_Transactional | RF_Transient | RF_TextExportTransient);
		SpriteComponent->CreationMethod = EComponentCreationMethod::Instance;
		static FConstructorStatics ConstructorStatics;
		SpriteComponent->Sprite = ConstructorStatics.TerrainTexture.Get();
		SpriteComponent->Mobility = EComponentMobility::Static;
		SpriteComponent->SetRelativeScale3D(FVector(5.0f, 5.0f, 5.0f));
		SpriteComponent->SetRelativeLocation(FVector(0, 0, 100));
		SpriteComponent->SetUsingAbsoluteScale(true);
		SpriteComponent->SetIsVisualizationComponent(true);
		SpriteComponent->bIsScreenSizeScaled = true;
		SpriteComponent->bUseInEditorScaling = true;
		SpriteComponent->SetupAttachment(RootComponent);
	}
#endif
}

void AMasterHarvestableField::PostLoad()
{
	Super::PostLoad();
	Assets.Remove(nullptr);
}

void AMasterHarvestableField::GetSlotBounds(const TArray<FStaticSlotInfo>& SlotArray, FVector& MinBounds,
	FVector& MaxBounds)
{
	MinBounds = FVector(FLT_MAX);
	MaxBounds = FVector(-FLT_MAX);
	for (auto SlotInfo : SlotArray)
	{
		MinBounds = MinBounds.ComponentMin(SlotInfo.Transform.GetLocation());
		MaxBounds = MaxBounds.ComponentMax(SlotInfo.Transform.GetLocation());
	}
}

#if WITH_EDITOR

void AMasterHarvestableField::SplitSlots(const TArray<FStaticSlotInfo>& Input, TArray<FStaticSlotInfo>& Sorted, TArray<FStaticSlotInfo>& Upper)
{
	FVector MinBounds, MaxBounds;
	GetSlotBounds(Input, MinBounds, MaxBounds);

	Sorted = Input;
	float SizeX = MaxBounds.X - MinBounds.X;
	float SizeY = MaxBounds.Y - MinBounds.Y;
	if (SizeX > SizeY)
	{
		Sorted.Sort([](const FStaticSlotInfo& A, const FStaticSlotInfo& B){
			return A.Transform.GetLocation().X < B.Transform.GetLocation().X;
		});		
	} else
	{
		Sorted.Sort([](const FStaticSlotInfo& A, const FStaticSlotInfo& B){
			return A.Transform.GetLocation().Y < B.Transform.GetLocation().Y;
		});
	}

	int32 StartIndex = Sorted.Num() / 2;
	int32 NumInSorted = Sorted.Num();
	int32 NumMoved = NumInSorted - StartIndex;

	Upper.Reserve(NumMoved);

	for (int32 Index = StartIndex; Index < NumInSorted; ++Index)
	{
		Upper.Emplace(Sorted[Index]);
	}
	Sorted.SetNum(NumInSorted - NumMoved);
}

void AMasterHarvestableField::SplitSlotsRecursive(const TArray<FStaticSlotInfo>& Input,
                                                  TArray<TArray<FStaticSlotInfo>>& OutputSlots)
{
	if (Input.Num() == 0)
	{
		return;
	}
	if (Input.Num() <= MaxNumSlotsPerSection)
	{
		OutputSlots.Add(Input);
		return;
	}
	TArray<FStaticSlotInfo> Lower;
	TArray<FStaticSlotInfo> Upper;
	SplitSlots(Input, Lower, Upper);

	SplitSlotsRecursive(Lower, OutputSlots);
	SplitSlotsRecursive(Upper, OutputSlots);
}

void AMasterHarvestableField::CreateSections()
{
	ClearSections();
	PrepareSlots();
	TArray<TArray<FStaticSlotInfo>> SectionSlots;
	SplitSlotsRecursive(Slots, SectionSlots);
	for (auto SlotsForSection : SectionSlots)
	{
		CreateFieldSection(SlotsForSection);
	}
}

void AMasterHarvestableField::ClearSections()
{
	for (AHarvestableFieldSection* Section : GetSections())
	{
		Section->Destroy();
	}
}

void AMasterHarvestableField::AddKamoIdTag(AHarvestableFieldSection* Section)
{
	auto UniqueID = FString(TEXT("kamoid:")) + Section->Id;
	Section->Tags.Add(FName(UniqueID));
}

AHarvestableFieldSection* AMasterHarvestableField::CreateFieldSection(const TArray<FStaticSlotInfo>& SlotsForSection)
{
	FVector MinBounds;
	FVector MaxBounds;
	GetSlotBounds(SlotsForSection, MinBounds, MaxBounds);
	MinBounds.X += -300;
	MaxBounds.X += 300;
	MinBounds.Y += -300;
	MaxBounds.Y += 300;
	MinBounds.Z += -1000;
	MaxBounds.Z += 1000;
	FBox Box(MinBounds, MaxBounds);

	AHarvestableFieldSection* Section = GetWorld()->SpawnActorDeferred<AHarvestableFieldSection>(
		AHarvestableFieldSection::StaticClass(), FTransform(Box.GetCenter()), this);
	Section->SetFolderPath("HarvestableFieldSections");
	Section->Master = this;
	Section->Id = FGuid::NewGuid().ToString().ToLower();
	Section->RegrowRate = RegrowRate;
	Section->RegrowTickRate = RegrowTickRate;
	Section->Slots = SlotsForSection;
	Section->BoxComponent->SetBoxExtent(Box.GetExtent());
	AddKamoIdTag(Section);
	
	Section->FinishSpawning(FTransform(Box.GetCenter()), true);
	

	return Section;
}

void AMasterHarvestableField::DrawDebugBoundingBox(const TArray<FStaticSlotInfo>& SlotsArray, const FColor& Color)
{
	FVector MinBounds, MaxBounds;
	GetSlotBounds(SlotsArray, MinBounds, MaxBounds);

	FBox Bounds(MinBounds, MaxBounds);
	UE_VLOG_BOX(this, LogHF, Display, Bounds, Color, TEXT("%d"), SlotsArray.Num());
	DrawDebugBox(GetWorld(), Bounds.GetCenter(), Bounds.GetExtent(), Color, false, 5.0f);
}

void AMasterHarvestableField::StartSplit()
{
	SectionSlotsForVisualDebugging.Empty();
	SectionSlotsForVisualDebugging.Add(Slots);
}

void AMasterHarvestableField::ContinueSplit()
{
	TArray<FStaticSlotInfo> Lower;
	TArray<FStaticSlotInfo> Upper;
	int32 SlotsToSplitIndex = -1;
	for (int32 Index = 0; Index < SectionSlotsForVisualDebugging.Num(); ++Index)
	{
		if (SectionSlotsForVisualDebugging[Index].Num() > MaxNumSlotsPerSection)
		{
			SlotsToSplitIndex = Index;
			break;			
		}
	}

	if (SlotsToSplitIndex == -1)
	{
		return;
	}

	const TArray<FStaticSlotInfo>& Input = SectionSlotsForVisualDebugging[SlotsToSplitIndex];
	SplitSlots(Input, Lower, Upper);
	DrawDebugBoundingBox(Input, FColor::Yellow);
	DrawDebugBoundingBox(Lower, FColor::Blue);
	DrawDebugBoundingBox(Upper, FColor::Green);
	SectionSlotsForVisualDebugging.RemoveAt(SlotsToSplitIndex);
	SectionSlotsForVisualDebugging.Add(Lower);
	SectionSlotsForVisualDebugging.Add(Upper);
}

TArray<AHarvestableFieldSection*> AMasterHarvestableField::GetSections() const
{
	
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHarvestableFieldSection::StaticClass(), OutActors);
	TArray<AHarvestableFieldSection*> Sections;
	for (AActor* Actor : OutActors)
	{
		Sections.Add(Cast<AHarvestableFieldSection>(Actor));
	}

	return Sections;
}

#endif

