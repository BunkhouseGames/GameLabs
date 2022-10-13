// 


#include "MasterHarvestableFieldBox.h"

#include "Components/BoxComponent.h"

AMasterHarvestableFieldBox::AMasterHarvestableFieldBox()
{
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Harvestable Field Bounds"));
	SetRootComponent(Box);
	Box->SetBoxExtent(FVector(InitialSize, InitialSize, InitialSize));
}

FBox AMasterHarvestableFieldBox::GetBoundingBox() const
{
	return Box->CalcBounds(Box->GetComponentTransform()).GetBox();
}

bool AMasterHarvestableFieldBox::GetRandomLocation(const FBox& BBox, FVector& Location)
{
	Location.Set(
		Rand.FRandRange(BBox.Min.X, BBox.Max.X),
		Rand.FRandRange(BBox.Min.Y, BBox.Max.Y),
		Rand.FRandRange(BBox.Min.Z, BBox.Max.Z)
	);

	return true;
}
