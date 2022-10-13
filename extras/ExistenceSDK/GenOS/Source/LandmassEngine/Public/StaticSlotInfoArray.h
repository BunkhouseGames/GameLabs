// 

#pragma once

#include "CoreMinimal.h"
#include "StaticSlotInfo.h"
#include "StaticSlotInfoArray.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FStaticSlotInfoArray
{
	GENERATED_BODY()

	int32 Num() const { return Items.Num(); }
	FStaticSlotInfo& operator [](int32 Index) { return Items[Index]; }
	void operator=(const TArray<FStaticSlotInfo>& Other) { Items = Other; }
	TArray<FStaticSlotInfo>::RangedForIteratorType begin() { return Items.begin(); }
	TArray<FStaticSlotInfo>::RangedForIteratorType end() { return Items.end(); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Version = 1;
	
	TArray<FStaticSlotInfo> Items;

	bool Serialize(FArchive& Ar);

	bool operator==(const FStaticSlotInfoArray& Other) const;
};

template<>
struct TStructOpsTypeTraits<FStaticSlotInfoArray> : TStructOpsTypeTraitsBase2<FStaticSlotInfoArray>
{
	enum
	{
		WithSerializer = true,
		WithIdenticalViaEquality = true
	};
};
