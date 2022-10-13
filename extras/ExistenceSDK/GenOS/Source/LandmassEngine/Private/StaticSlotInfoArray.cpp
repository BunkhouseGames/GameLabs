// 


#include "StaticSlotInfoArray.h"

bool FStaticSlotInfoArray::Serialize(FArchive& Ar)
{
	int32 Num;
	if (!Ar.IsLoading())
	{
		Num = Items.Num();
	}

	Ar << Num;

	if (Ar.IsLoading())
	{
		Items.AddUninitialized(Num);
	}

	for (int32 Index = 0; Index < Num; ++Index)
	{
		FStaticSlotInfo& Item = Items[Index];
		Item.Serialize(Ar);
	}
	return true;
}

bool FStaticSlotInfoArray::operator==(const FStaticSlotInfoArray& Other) const
{
	return Version == Other.Version && Items == Other.Items;
}
