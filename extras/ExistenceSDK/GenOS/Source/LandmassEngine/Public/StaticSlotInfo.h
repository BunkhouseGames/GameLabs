// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StaticSlotInfo.generated.h"

USTRUCT(BlueprintType)
struct FStaticSlotInfo
{
	GENERATED_BODY()

	FStaticSlotInfo() : AssetIndex(-1) {}
	FStaticSlotInfo(const FRotator& Rotation, const FVector& Location, const FVector& Scale, int32 AssetIndex) :
		Transform(Rotation, Location, Scale),
		AssetIndex(AssetIndex)
	{}
	
	/** Slot location. The orientation and scale are also applicable for certain types of slots.*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FTransform Transform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 AssetIndex;

	bool Serialize(FArchive& Ar);
	bool operator==(const FStaticSlotInfo& Other) const;
};
