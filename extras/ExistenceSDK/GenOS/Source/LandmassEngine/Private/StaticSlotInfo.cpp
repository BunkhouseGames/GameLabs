// Fill out your copyright notice in the Description page of Project Settings.


#include "StaticSlotInfo.h"

bool FStaticSlotInfo::Serialize(FArchive& Ar)
{
	Ar << Transform;
	Ar << AssetIndex;

	return true;
}

bool FStaticSlotInfo::operator==(const FStaticSlotInfo& Other) const
{
	return Transform.Equals(Other.Transform) && AssetIndex == Other.AssetIndex;
}
