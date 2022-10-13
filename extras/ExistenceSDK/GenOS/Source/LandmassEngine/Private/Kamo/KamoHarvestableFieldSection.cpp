// 


#include "KamoHarvestableFieldSection.h"

#include "DynamicSlotInfo.h"
#include "HarvestableFieldSection.h"

static const FString DynamicSlotsKey = {TEXT("DynamicSlots")};
static const FString SlotIndexKey = {TEXT("SlotIndex")};
static const FString IsEmptyKey = {TEXT("bIsEmpty")};
static const FString GrowthKey = {TEXT("Growth")};
static const FString TypeKey = {TEXT("Type")};

void UKamoHarvestableFieldSection::ApplyKamoStateToActor_Implementation(EKamoStateStage Stage_Stage)
{
	Super::ApplyKamoStateToActor_Implementation(Stage_Stage);

	FString JSON_Text;
	state->GetString(DynamicSlotsKey, JSON_Text);

	TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create(JSON_Text);
	TArray<TSharedPtr<FJsonValue>> ObjectArray;
	bool bSuccess = FJsonSerializer::Deserialize(Reader, ObjectArray);
	if (bSuccess)
	{
		AHarvestableFieldSection* Section = Cast<AHarvestableFieldSection>(object);
		TArray<FDynamicSlotInfoItem>& DynamicSlotInfoItems = Section->DynamicSlots.Items;
		DynamicSlotInfoItems.Empty(ObjectArray.Num());
		for (auto JsonValue : ObjectArray)
		{
			TSharedPtr<FJsonObject> InfoObject = JsonValue->AsObject();

			FDynamicSlotInfoItem Item;
			FDynamicSlotInfo& Info = Item.Info;
			Info.SlotIndex = InfoObject->GetNumberField(SlotIndexKey);
			Info.bIsEmpty = InfoObject->GetBoolField(IsEmptyKey);
			Info.Growth = InfoObject->GetNumberField(GrowthKey);
			Info.type = InfoObject->GetStringField(TypeKey);

			DynamicSlotInfoItems.Emplace(Item);
		}
		Section->DynamicSlots.MarkArrayDirty();
	}
}

void UKamoHarvestableFieldSection::UpdateKamoStateFromActor_Implementation()
{
	Super::UpdateKamoStateFromActor_Implementation();

	TArray<TSharedPtr<FJsonValue>> ObjectArray;

	AHarvestableFieldSection* Section = Cast<AHarvestableFieldSection>(object);
	for (auto Item : Section->DynamicSlots.Items)
	{
		const FDynamicSlotInfo& Info = Item.Info;

		TSharedPtr<FJsonObject> JSONData = MakeShareable(new FJsonObject);
	
		JSONData->SetNumberField(SlotIndexKey, Info.SlotIndex);
		JSONData->SetBoolField(IsEmptyKey, Info.bIsEmpty);
		JSONData->SetNumberField(GrowthKey, Info.Growth);
		JSONData->SetStringField(TypeKey, Info.type);
		
		ObjectArray.Add(MakeShareable(new FJsonValueObject(JSONData)));
	}

	typedef TJsonWriterFactory< TCHAR, TCondensedJsonPrintPolicy<TCHAR> > FCondensedJsonStringWriterFactory;
	typedef TJsonWriter< TCHAR, TCondensedJsonPrintPolicy<TCHAR> > FCondensedJsonStringWriter;
	
	FString JSON_Text;
	TSharedRef< FCondensedJsonStringWriter > Writer = FCondensedJsonStringWriterFactory::Create(&JSON_Text);
	FJsonSerializer::Serialize(ObjectArray, Writer);

	state->SetString(DynamicSlotsKey, JSON_Text);
}
