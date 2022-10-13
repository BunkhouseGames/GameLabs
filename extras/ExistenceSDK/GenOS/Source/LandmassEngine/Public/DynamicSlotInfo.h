// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DynamicSlotInfo.generated.h"

struct FDynamicSlotInfoArray;

// Dynamic Slot Info - Fast Array Enabled
USTRUCT(BlueprintType)
struct FDynamicSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SlotIndex=-1; // The index of the slot in AHarvestableField.Slots array.

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsEmpty = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Growth = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString type;

	friend bool operator== (const FDynamicSlotInfo& A, const FDynamicSlotInfo& B)
	{
		return
			A.SlotIndex == B.SlotIndex
			&& A.bIsEmpty == B.bIsEmpty
			&& A.type == B.type
			&& A.Growth == B.Growth;
	};

};


USTRUCT(BlueprintType)
struct FDynamicSlotInfoItem : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FDynamicSlotInfo Info;

	void PreReplicatedRemove(const FDynamicSlotInfoArray& Serializer) const;
	void PostReplicatedAdd(const FDynamicSlotInfoArray& Serializer) const;
	void PostReplicatedChange(const FDynamicSlotInfoArray& Serializer) const;
};




/** Step 2: You MUST wrap your TArray in another struct that inherits from FFastArraySerializer */
USTRUCT()
struct FDynamicSlotInfoArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FDynamicSlotInfoItem>	Items;	/** Step 3: You MUST have a TArray named Items of the struct you made in step 1. */


	/** Step 4: Copy this, replace example with your names */
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);

	class AHarvestableFieldSection* Owner;
};


/** Step 5: Copy and paste this struct trait, replacing FExampleArray with your Step 2 struct. */
template<>
struct TStructOpsTypeTraits< FDynamicSlotInfoArray > : public TStructOpsTypeTraitsBase2< FDynamicSlotInfoArray >
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
