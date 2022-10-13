// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicSlotInfo.h"

#include "HarvestableFieldsStats.h"
#include "HarvestableFieldSection.h"

void FDynamicSlotInfoItem::PreReplicatedRemove(const FDynamicSlotInfoArray& Serializer) const
{
	if (Serializer.Owner)
	{
		FDynamicSlotInfo SlotInfoDefault;
		SlotInfoDefault.SlotIndex = Info.SlotIndex;
		Serializer.Owner->OnSlotChange(SlotInfoDefault);
	}
}


void FDynamicSlotInfoItem::PostReplicatedAdd(const FDynamicSlotInfoArray& Serializer) const
{
	if (Serializer.Owner)
	{
		Serializer.Owner->OnSlotChange(this->Info);
	}
}


void FDynamicSlotInfoItem::PostReplicatedChange(const FDynamicSlotInfoArray& Serializer) const
{
	if (Serializer.Owner)
	{
		Serializer.Owner->OnSlotChange(this->Info);
	}
}

bool FDynamicSlotInfoArray::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
	return FastArrayDeltaSerialize<FDynamicSlotInfoItem, FDynamicSlotInfoArray>(Items, DeltaParms, *this);
}

