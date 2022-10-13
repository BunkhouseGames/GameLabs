// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.


#include "KamoState.h"

#include "JsonObjectConverter.h"
#include "KamoRuntime.h" // just for log category, plz fix


DEFINE_LOG_CATEGORY(LogKamoState);

#define CHECKARG(ob, errmsg, ret) {if (!(ob)) {UE_LOG(LogKamoState, Error, errmsg); return ret;}}

UKamoState* UKamoState::CreateKamoState()
{
	return NewObject<UKamoState>();
}

void UKamoState::SetObjectField(const FString& key, const TSharedPtr< FJsonObject >& JsonObject)
{
	localState->SetObjectField(key, JsonObject);
}

void UKamoState::SetObjectField(const FString& key, UKamoState* ObjectState)
{
	localState->SetObjectField(key, ObjectState->localState);
}

void UKamoState::SetString(const FString& key, const FString& value) {
	localState->SetStringField(key, value);
}

void UKamoState::SetObjectReference(const FString& key, const FString& value) {
	localState->SetStringField(key, FString::Printf(TEXT("obref:%s"), *value));
}

void UKamoState::SetInt(const FString& key, const int& value) {
	localState->SetNumberField(key, value);
}

void UKamoState::SetFloat(const FString& key, const float& value) {
	localState->SetNumberField(key, value);
}

void UKamoState::SetBool(const FString& key, const bool& value) {
	localState->SetBoolField(key, value);
}

void UKamoState::SetVector(const FString& key, const FVector& value) {
	localState->SetObjectField(key, CreateJsonVector(value));
}

void UKamoState::SetQuat(const FString& key, const FQuat& value) {
	localState->SetObjectField(key, CreateJsonQuat(value));
}

void UKamoState::SetTransform(const FString& key, const FTransform& value) {
	localState->SetObjectField(key, CreateJsonTransform(value));
}

void UKamoState::SetStringArray(const FString& key, const TArray<FString>& valueArray) {
	TArray<TSharedPtr<FJsonValue>> ret;
	for (auto value : valueArray) {
		ret.Add(MakeShareable(new FJsonValueString(value)));
	}
	localState->SetArrayField(key, ret);
}

void UKamoState::SetIntArray(const FString& key, const TArray<int>& valueArray) {
	TArray<TSharedPtr<FJsonValue>> ret;
	for (auto value : valueArray) {
		ret.Add(MakeShareable(new FJsonValueNumber(value)));
	}
	localState->SetArrayField(key, ret);
}

void UKamoState::SetFloatArray(const FString& key, const TArray<float>& valueArray) {
	TArray<TSharedPtr<FJsonValue>> ret;
	for (auto value : valueArray) {
		ret.Add(MakeShareable(new FJsonValueNumber(value)));
	}
	localState->SetArrayField(key, ret);
}

void UKamoState::SetBoolArray(const FString& key, const TArray<bool>& valueArray) {
	TArray<TSharedPtr<FJsonValue>> ret;
	for (auto value : valueArray) {
		ret.Add(MakeShareable(new FJsonValueBoolean(value)));
	}
	localState->SetArrayField(key, ret);
}

void UKamoState::SetVectorArray(const FString& key, const TArray<FVector>& valueArray) {
	TArray<TSharedPtr<FJsonValue>> objectArray;
	for (auto value : valueArray) {
		objectArray.Add(MakeShareable(new FJsonValueObject(CreateJsonVector(value))));
	}
	localState->SetArrayField(key, objectArray);
}

void UKamoState::SetTransformArray(const FString& key, const TArray<FTransform>& valueArray) {
	TArray<TSharedPtr<FJsonValue>> objectArray;
	for (auto value : valueArray) {
		objectArray.Add(MakeShareable(new FJsonValueObject(CreateJsonTransform(value))));
	}
	localState->SetArrayField(key, objectArray);
}

void UKamoState::SetRotator(const FString& key, const FRotator& value) {
	localState->SetObjectField(key, CreateJsonRotator(value));
}

void UKamoState::SetRotatorArray(const FString& key, const TArray<FRotator>& valueArray) {
	TArray<TSharedPtr<FJsonValue>> objectArray;
	for (auto value : valueArray) {
		objectArray.Add(MakeShareable(new FJsonValueObject(CreateJsonRotator(value))));
	}
	localState->SetArrayField(key, objectArray);
}


const TSharedPtr< FJsonObject >& UKamoState::GetObjectField(const FString& FieldName) const
{
	return localState->GetObjectField(FieldName);
}


bool UKamoState::GetVector(const FString& key, FVector& result) {

	if (!localState->HasField(key))
	{
		return false;
	}

	FJsonObject* output = localState->GetObjectField(key).Get();
	result = ReadVectorFromJson(output);
	return true;
}

bool UKamoState::GetQuat(const FString& key, FQuat& result) {

	if (!localState->HasField(key))
	{
		return false;
	}

	FJsonObject* output = localState->GetObjectField(key).Get();
	result = ReadQuatFromJson(output);
	return true;
}

bool UKamoState::GetRotator(const FString& key, FRotator& result) {
	if (!localState->HasField(key))
	{
		return false;
	}

	FJsonObject* output = localState->GetObjectField(key).Get();
	result = ReadRotatorFromJson(output);
	return true;
}

bool UKamoState::GetRotatorArray(const FString& key, TArray<FRotator>& resultArray) {
	const TArray<TSharedPtr<FJsonValue>>* out;
	if (localState->TryGetArrayField(key, out)) {
		for (int i = 0; i < out->Num(); i++) {
			const TSharedPtr<FJsonObject>* elem;
			if (!(*out)[i]->TryGetObject(elem)) {
				continue;
			}
			resultArray.Add(ReadRotatorFromJson(elem->Get()));
		}
		return true;
	}
	return false;
}

bool UKamoState::GetTransform(const FString& key, FTransform& result) {
	if (!localState->HasField(key)) {
		return false;
	}

	FJsonObject* output = localState->GetObjectField(key).Get();
	result = ReadTransformFromJson(output);
	return true;
}

bool UKamoState::GetColor(const FString& key, FColor& result) {
	if (!localState->HasField(key)) {
		return false;
	}

	FJsonObject* output = localState->GetObjectField(key).Get();
	result = ReadColorFromJson(output);
	return true;
}


bool UKamoState::GetString(const FString& key, FString& result) {
	return localState->TryGetStringField(key, result);
}

bool UKamoState::GetObjectReference(const FString& key, FString& result) 
{
	if (localState->TryGetStringField(key, result))
	{
		if (result.StartsWith(TEXT("obref:")))
		{
			result.RightChopInline(6);
			return true;
		}
	}
	return false;
}


bool UKamoState::GetFloat(const FString& key, float& result) {
	double out;

	if (localState->TryGetNumberField(key, out))
	{
		result = out;
		return true;
	}
	
	return false;
}

bool UKamoState::GetInt(const FString& key, int& result) 
{
	return localState->TryGetNumberField(key, result);
}

bool UKamoState::GetBool(const FString& key, bool& result) 
{
	return localState->TryGetBoolField(key, result);
}

bool UKamoState::GetStringArray(const FString& key, TArray<FString>& result) 
{
	return localState->TryGetStringArrayField(key, result);
}

bool UKamoState::GetIntArray(const FString& key, TArray<int>& result) 
{
	const TArray<TSharedPtr<FJsonValue>>* out;
	if (localState->TryGetArrayField(key, out)) 
	{
		for (int i = 0; i < out->Num(); i++) 
		{
			int elem;
			if (!(*out)[i]->TryGetNumber(elem)) 
			{
				continue;
			}
			result.Add(elem);
		}
		return true;
	}
	return false;
}

bool UKamoState::GetFloatArray(const FString& key, TArray<float>& result) 
{
	const TArray<TSharedPtr<FJsonValue>>* out;
	if (localState->TryGetArrayField(key, out))
	{
		for (int i = 0; i < out->Num(); i++) {
			double elem;
			if (!(*out)[i]->TryGetNumber(elem)) {
				continue;
			}
			result.Add((float)elem);
		}
		return true;
	}
	return false;
}

bool UKamoState::GetBoolArray(const FString& key, TArray<bool>& result) 
{
	const TArray<TSharedPtr<FJsonValue>>* out;
	if (localState->TryGetArrayField(key, out)) 
	{
		for (int i = 0; i < out->Num(); i++) {
			bool elem;
			if (!(*out)[i]->TryGetBool(elem)) {
				continue;
			}
			result.Add(elem);
		}
		return true;
	}
	return false;
}

bool UKamoState::GetVectorArray(const FString& key, TArray<FVector>& result) 
{
	const TArray<TSharedPtr<FJsonValue>>* out;
	if (localState->TryGetArrayField(key, out)) 
	{
		for (int i = 0; i < out->Num(); i++) {
			const TSharedPtr<FJsonObject>* elem;
			if (!(*out)[i]->TryGetObject(elem)) {
				continue;
			}
			result.Add(ReadVectorFromJson(elem->Get()));
		}
		return true;
	}
	return false;
}

bool UKamoState::GetTransformArray(const FString& key, TArray<FTransform>& result) 
{
	const TArray<TSharedPtr<FJsonValue>>* out;
	if (localState->TryGetArrayField(key, out)) 
	{
		for (int i = 0; i < out->Num(); i++) {
			const TSharedPtr<FJsonObject>* elem;
			if (!(*out)[i]->TryGetObject(elem)) {
				continue;
			}
			result.Add(ReadTransformFromJson(elem->Get()));
		}
		return true;
	}
	return false;
}


void UKamoState::GetKeys(const FString& dataType, TArray<FString>& result)
{
	EJson type = EJson::None;

	if (dataType == "int")
	{
		type = EJson::Number;
	}

	for (auto val : localState->Values)
	{
		if (type == EJson::None || type == val.Value.Get()->Type)
		{
			result.Add(val.Key);
		}
	}
}

FString UKamoState::GetStateAsString() const
{

	//UE_LOG(LogKamoRt, Display, TEXT("State before it is returned to be serialized: %s"), *InternalJsonState);

	FString json_text;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&json_text);
	FJsonSerializer::Serialize(localState, Writer);

	return json_text;
}

void UKamoState::SetKamoState(const FString& key, UKamoState* kamo_state)
{
	CHECKARG(kamo_state, TEXT("SetKamoState: 'kamo_state' must be valid."), ;);
	localState->SetObjectField(key, kamo_state->localState);
}

void UKamoState::SetKamoStateArray(const FString& key, const TArray<UKamoState*>& array)
{
	TArray<TSharedPtr<FJsonValue>> objectArray;

	for (auto value : array)
	{
		if (value)
		{
			objectArray.Add(MakeShareable(new FJsonValueObject(value->localState)));
		}
	}

	localState->SetArrayField(key, objectArray);
}

void UKamoState::AddToKamoStateList(const FString& key, UKamoState* kamo_state)
{
	CHECKARG(kamo_state, TEXT("AddToKamoStateList: 'kamo_state' must be valid."), ;);

	// Create a shared pointed to the passed in kamo state json state
	TSharedPtr<FJsonObject> kamo_object_shared_pointer = MakeShared<FJsonObject>(kamo_state->localState.Get());

	// Convert the passed in kamo state to a JsonValueObject
	TSharedPtr<FJsonValueObject> json_object_to_add = MakeShared<FJsonValueObject>(FJsonValueObject(kamo_object_shared_pointer));

	const TArray<TSharedPtr<FJsonValue>>* array_read_from_input;
	bool exists = localState->TryGetArrayField(key, array_read_from_input);

	TArray<TSharedPtr<FJsonValue>> new_array;
	if (exists) {

		new_array.Append(*array_read_from_input);

	}

	new_array.Add(json_object_to_add);
	localState->SetArrayField(key, new_array);

}

bool UKamoState::GetKamoState(const FString& key, UKamoState*& result) 
{
	const TSharedPtr<FJsonObject>* out;
	if (localState->TryGetObjectField(key, out)) 
	{
		auto state = NewObject<UKamoState>(this);
		state->localState = out->ToSharedRef();
		result = state;
		return true;
	}
	return false;
}

TArray<FString> UKamoState::GetKamoStateKeys() const
{
	TArray<FString> keys;
	localState->Values.GetKeys(keys);
	return keys;
}

bool UKamoState::GetKamoStateArray(const FString& key, TArray<UKamoState*>& result) 
{
	const TArray<TSharedPtr<FJsonValue>>* array_ptr;
	if (localState->TryGetArrayField(key, array_ptr)) 
	{
		auto array = *array_ptr;

		for (auto elem : array) {

			const TSharedPtr<FJsonObject>* shared_ptr_ptr;

			if (!elem->TryGetObject(shared_ptr_ptr)) {
				continue;
			}

			auto state = NewObject<UKamoState>(this);

			state->localState = shared_ptr_ptr->ToSharedRef();

			result.Add(state);
		}
		return true;
	}
	return false;
}

bool UKamoState::SetState(const FString& jsonString)
{
	TSharedPtr<FJsonObject> jsonData = MakeShareable(new FJsonObject);

	TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create(jsonString);
	bool success = FJsonSerializer::Deserialize(Reader, jsonData);

	localState.Get() = *jsonData.Get();
	return success;
}

void UKamoState::SetStateFromJsonString(const FString& jsonString, bool& success)
{
	success = SetState(jsonString);
	if (!success)
	{
		UE_LOG(LogKamoRt, Warning, TEXT("Could not set state from string: %s"), *jsonString);
	}
}

void UKamoState::SetStateFromProperty(UObject* Object, FBoolProperty* BoolProp)
{
	bool Value = BoolProp->GetPropertyValue_InContainer(Object);
	localState->SetBoolField(BoolProp->GetName(), Value);
}

void UKamoState::SetStateFromProperty(UObject* Object, FIntProperty* IntProp)
{
	int Value = IntProp->GetPropertyValue_InContainer(Object);
	localState->SetNumberField(IntProp->GetName(), Value);
}

void UKamoState::SetStateFromProperty(UObject* Object, FFloatProperty* FloatProp)
{
	float Value = FloatProp->GetPropertyValue_InContainer(Object);
	localState->SetNumberField(FloatProp->GetName(), Value);
}

void UKamoState::SetStateFromProperty(UObject* Object, FStrProperty* StringProp)
{
	const FString& Value = StringProp->GetPropertyValue_InContainer(Object);
	localState->SetStringField(StringProp->GetName(), Value);
}

void UKamoState::SetStateFromProperty(UObject* Object, FNameProperty* NameProp)
{
	const FName& Value = NameProp->GetPropertyValue_InContainer(Object);
	localState->SetStringField(NameProp->GetName(), Value.ToString());
}

void UKamoState::SetStateFromProperty(UObject* Object, FStructProperty* StructProp)
{
	FString Value;
	StructProp->ExportText_InContainer(0, Value, Object, nullptr, Object, 0);
	
	localState->SetStringField(StructProp->GetName(), Value);
}

void UKamoState::SetStateFromProperty(UObject* Object, FArrayProperty* ArrayProp)
{
	void* ArrayPtr = ArrayProp->ContainerPtrToValuePtr<void>(Object);
	FScriptArrayHelper ArrayHelper(ArrayProp, ArrayPtr);
	TArray<TSharedPtr<FJsonValue>> JArray;

	int N = ArrayHelper.Num();
	for (int I = 0; I < N; I++)
	{
		TSharedPtr<FJsonValue> Value = FJsonObjectConverter::UPropertyToJsonValue(
			ArrayProp->Inner, ArrayHelper.GetRawPtr(I), 0, CPF_Transient);
		if (Value.IsValid() && !Value->IsNull())
		{
			JArray.Push(Value);
		}
	}

	localState->SetArrayField(ArrayProp->GetName(), JArray);
}

void UKamoState::SetStateFromProperty(UObject* Object, FMapProperty* MapProp)
{
	TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>>Keys;
	TArray<TSharedPtr<FJsonValue>>Values;

	void* MapPtr = MapProp->ContainerPtrToValuePtr<void>(Object);
	FScriptMapHelper Helper(MapProp, MapPtr);

	int N = Helper.Num();
	for (int I = 0; I < N; I++)
	{
		TSharedPtr<FJsonValue> Key = FJsonObjectConverter::UPropertyToJsonValue(
			MapProp->KeyProp, Helper.GetKeyPtr(I), 0, CPF_Transient);
		TSharedPtr<FJsonValue> Value = FJsonObjectConverter::UPropertyToJsonValue(
			MapProp->ValueProp, Helper.GetValuePtr(I), 0, CPF_Transient);

		if (Key.IsValid() && !Key->IsNull() && Value.IsValid() && !Value->IsNull())
		{
			Keys.Push(Key);
			Values.Push(Value);
		}
	}

	Json->SetArrayField(TEXT("keys"), Keys);
	Json->SetArrayField(TEXT("values"), Values);
	localState->SetObjectField(MapProp->GetName(), Json);
}

void UKamoState::SetStateFromProperty(UObject* Object, FSetProperty* SetProp)
{
	TArray<TSharedPtr<FJsonValue>> Elements;

	void* SetPtr = SetProp->ContainerPtrToValuePtr<void>(Object);
	FScriptSetHelper Helper(SetProp, SetPtr);

	int N = Helper.Num();
	for (int I = 0; I < N; I++)
	{
		TSharedPtr<FJsonValue> Element = FJsonObjectConverter::UPropertyToJsonValue(
            SetProp->ElementProp, Helper.GetElementPtr(I), 0, CPF_Transient);

		if (Element.IsValid() && !Element->IsNull())
		{
			Elements.Push(Element);
		}
	}

	localState->SetArrayField(SetProp->GetName(), Elements);
}

void UKamoState::SetPropertyFromState(UObject* Object, FBoolProperty* BoolProp)
{
	bool Value = localState->GetBoolField(BoolProp->GetName());
	BoolProp->SetPropertyValue_InContainer(Object, Value);
}

void UKamoState::SetPropertyFromState(UObject* Object, FIntProperty* IntProp)
{
	int Value = localState->GetIntegerField(IntProp->GetName());
	IntProp->SetPropertyValue_InContainer(Object, Value);
}

void UKamoState::SetPropertyFromState(UObject* Object, FFloatProperty* FloatProp)
{
	float Value = localState->GetNumberField(FloatProp->GetName());
	FloatProp->SetPropertyValue_InContainer(Object, Value);
}

void UKamoState::SetPropertyFromState(UObject* Object, FStrProperty* StringProp)
{
	FString Value = localState->GetStringField(StringProp->GetName());
	StringProp->SetPropertyValue_InContainer(Object, Value);
}

void UKamoState::SetPropertyFromState(UObject* Object, FNameProperty* NameProp)
{
	FName Value(localState->GetStringField(NameProp->GetName()));
	NameProp->SetPropertyValue_InContainer(Object, Value);
}

void UKamoState::SetPropertyFromState(UObject* Object, FStructProperty* StructProp)
{
	FString Value = localState->GetStringField(StructProp->GetName());
	if (!Value.IsEmpty())
	{
		void* Data = StructProp->ContainerPtrToValuePtr<void>(Object, 0);
		StructProp->ImportText(*Value, Data, 0, Object, nullptr);
	}
}

void UKamoState::SetPropertyFromState(UObject* Object, FArrayProperty* ArrayProp)
{
	auto ValueArray = localState->GetArrayField(ArrayProp->GetName());
	void* ArrayPtr = ArrayProp->ContainerPtrToValuePtr<void>(Object);
	FScriptArrayHelper ArrayHelper(ArrayProp, ArrayPtr);
	ArrayHelper.Resize(ValueArray.Num());
	int N = ArrayHelper.Num();
	for (int I = 0; I < N; I++)
	{
		const auto& Value = ValueArray[I];
		if (Value.IsValid() && !Value->IsNull())
		{
			FJsonObjectConverter::JsonValueToUProperty(Value, ArrayProp->Inner, ArrayHelper.GetRawPtr(I), 0, CPF_Transient);
		}
	}
}

void UKamoState::SetPropertyFromState(UObject* Object, FMapProperty* MapProp)
{
	auto ValueMap = localState->GetObjectField(MapProp->GetName());
	auto Keys = ValueMap->GetArrayField(TEXT("keys"));
	auto Values = ValueMap->GetArrayField(TEXT("values"));

	if (Keys.Num() != Values.Num())
	{
		UE_LOG(LogKamoState, Warning, TEXT("Keys and values don't match for property %s"), *MapProp->GetName());
		return;
	}

	auto MapPtr = MapProp->ContainerPtrToValuePtr<void>(Object);
	FScriptMapHelper Helper(MapProp, MapPtr);
	Helper.EmptyValues();

	int N = Keys.Num();
	for (int I = 0; I < N; I++)
	{
		auto Key = Keys[I];
		auto Value = Values[I];
		auto IKey = Helper.AddDefaultValue_Invalid_NeedsRehash();
		FJsonObjectConverter::JsonValueToUProperty(Key, MapProp->KeyProp, Helper.GetKeyPtr(IKey), 0, CPF_Transient);
		FJsonObjectConverter::JsonValueToUProperty(Value, MapProp->ValueProp, Helper.GetValuePtr(I), 0, CPF_Transient);
	}
	Helper.Rehash();
}

void UKamoState::SetPropertyFromState(UObject* Object, FSetProperty* SetProp)
{
	auto ValueArray = localState->GetArrayField(SetProp->GetName());

	auto SetPtr = SetProp->ContainerPtrToValuePtr<void>(Object);
	FScriptSetHelper Helper(SetProp, SetPtr);
	Helper.EmptyElements();

	int N = ValueArray.Num();
	for (int I = 0; I < N; I++)
	{
		auto Value = ValueArray[I];
		auto IKey = Helper.AddDefaultValue_Invalid_NeedsRehash();
		FJsonObjectConverter::JsonValueToUProperty(Value, SetProp->ElementProp, Helper.GetElementPtr(I), 0, CPF_Transient);
	}
	Helper.Rehash();
}

void UKamoState::SetStateFromProperty(UObject* Object, const FString& PropertyName)
{
	UClass* ObjectClass = Object->GetClass();
	FProperty* Property = ObjectClass->FindPropertyByName(FName(PropertyName));
	FBoolProperty* BoolProp = CastField<FBoolProperty>(Property);
	if(BoolProp)
	{
		SetStateFromProperty(Object, BoolProp);
		return;
	}
	FIntProperty* IntProp = CastField<FIntProperty>(Property);
	if(IntProp)
	{
		SetStateFromProperty(Object, IntProp);
		return;
	}
	FFloatProperty* FloatProp = CastField<FFloatProperty>(Property);
	if(FloatProp)
	{
		SetStateFromProperty(Object, FloatProp);
		return;
	}
	FStrProperty* StringProp = CastField<FStrProperty>(Property);
	if(StringProp)
	{
		SetStateFromProperty(Object, StringProp);
		return;
	}
	FNameProperty* NameProp = CastField<FNameProperty>(Property);
	if(NameProp)
	{
		SetStateFromProperty(Object, NameProp);
		return;
	}
	FStructProperty* StructProp = CastField<FStructProperty>(Property);
	if(StructProp)
	{
		SetStateFromProperty(Object, StructProp);
		return;
	}
	FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property);
	if (ArrayProp)
	{
		SetStateFromProperty(Object, ArrayProp);
		return;
	}
	FMapProperty* MapProp = CastField<FMapProperty>(Property);
	if (MapProp)
	{
		SetStateFromProperty(Object, MapProp);
		return;
	}
	FSetProperty* SetProp = CastField<FSetProperty>(Property);
	if (SetProp)
	{
		SetStateFromProperty(Object, SetProp);
	}
}

void UKamoState::SetStateFromProperties(UObject* Object, const TArray<FString>& PropertyNames)
{
	for (const FString& Name : PropertyNames)
	{
		SetStateFromProperty(Object, Name);
	}
}

void UKamoState::SetPropertyFromState(UObject* Object, const FString& PropertyName)
{
	UClass* ObjectClass = Object->GetClass();
	FProperty* Property = ObjectClass->FindPropertyByName(FName(PropertyName));
	FBoolProperty* BoolProp = CastField<FBoolProperty>(Property);
	if (BoolProp)
	{
		SetPropertyFromState(Object, BoolProp);
		return;
	}
	FIntProperty* IntProp = CastField<FIntProperty>(Property);
	if (IntProp)
	{
		SetPropertyFromState(Object, IntProp);
		return;
	}
	FFloatProperty* FloatProp = CastField<FFloatProperty>(Property);
	if (FloatProp)
	{
		SetPropertyFromState(Object, FloatProp);
		return;
	}
	FStrProperty* StringProp = CastField<FStrProperty>(Property);
	if (StringProp)
	{
		SetPropertyFromState(Object, StringProp);
		return;
	}
	FNameProperty* NameProp = CastField<FNameProperty>(Property);
	if (NameProp)
	{
		SetPropertyFromState(Object, NameProp);
		return;
	}
	FStructProperty* StructProp = CastField<FStructProperty>(Property);
	if (StructProp)
	{
		SetPropertyFromState(Object, StructProp);
		return;
	}
	FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property);
	if (ArrayProp)
	{
		SetPropertyFromState(Object, ArrayProp);
		return;
	}
	FMapProperty* MapProp = CastField<FMapProperty>(Property);
	if (MapProp)
	{
		SetPropertyFromState(Object, MapProp);
		return;
	}
	FSetProperty* SetProp = CastField<FSetProperty>(Property);
	if (SetProp)
	{
		SetPropertyFromState(Object, SetProp);
		return;
	}
}

void UKamoState::SetPropertiesFromState(UObject* Object, const TArray<FString>& PropertyNames)
{
	for (const FString& Name : PropertyNames)
	{
		SetPropertyFromState(Object, Name);
	}
}

void UKamoState::PopulateFromField(UKamoState* Other, const FString& FieldName)
{
	TSharedPtr<FJsonObject> State = Other->GetObjectField(FieldName);
	TArray<FString> Keys;
	State->Values.GetKeys(Keys);
	for (FString Key : Keys)
	{
		TSharedPtr<FJsonValue> Value = State->TryGetField(Key);
		localState->SetField(Key, Value);
	}
}

TSharedPtr<FJsonObject> UKamoState::CreateJsonVector(const FVector& value) {
	TSharedPtr<FJsonObject> VectorJson = MakeShareable(new FJsonObject);

	VectorJson->SetNumberField("x", value.X);
	VectorJson->SetNumberField("y", value.Y);
	VectorJson->SetNumberField("z", value.Z);

	return VectorJson;
}

TSharedPtr<FJsonObject> UKamoState::CreateJsonQuat(const FQuat& value) {
	TSharedPtr<FJsonObject> QuatJson = MakeShareable(new FJsonObject);

	QuatJson->SetNumberField("x", value.X);
	QuatJson->SetNumberField("y", value.Y);
	QuatJson->SetNumberField("z", value.Z);
	QuatJson->SetNumberField("w", value.W);

	return QuatJson;
}

TSharedPtr<FJsonObject> UKamoState::CreateJsonRotator(const FRotator& value) {
	TSharedPtr<FJsonObject> RotatorJson = MakeShareable(new FJsonObject);

	RotatorJson->SetNumberField("yaw", value.Yaw);
	RotatorJson->SetNumberField("pitch", value.Pitch);
	RotatorJson->SetNumberField("roll", value.Roll);

	return RotatorJson;
}

TSharedRef<FJsonObject> UKamoState::CreateJsonTransform(const FTransform& value) {

	TSharedRef<FJsonObject> TransformJson = MakeShareable(new FJsonObject);

	TransformJson.Get().SetObjectField("location", CreateJsonVector(value.GetLocation()));
	TransformJson.Get().SetObjectField("rotation", CreateJsonVector(value.GetRotation().Euler()));
	TransformJson.Get().SetObjectField("scale", CreateJsonVector(FVector(value.GetScale3D().X, value.GetScale3D().Y, value.GetScale3D().Z)));

	return TransformJson;
}

FVector UKamoState::ReadVectorFromJson(FJsonObject* value) {

	return FVector(value->GetNumberField("x"), value->GetNumberField("y"), value->GetNumberField("z"));
}

FQuat UKamoState::ReadQuatFromJson(FJsonObject* value) {

	return FQuat(value->GetNumberField("x"), value->GetNumberField("y"), value->GetNumberField("z"), value->GetNumberField("w"));
}

FRotator UKamoState::ReadRotatorFromJson(FJsonObject* value) {
	return FRotator(value->GetNumberField("pitch"), value->GetNumberField("yaw"), value->GetNumberField("roll"));
}

FTransform UKamoState::ReadTransformFromJson(FJsonObject* value) {

	FTransform out = FTransform();

	FJsonObject* loc = value->GetObjectField("location").Get();
	FJsonObject* rot = value->GetObjectField("rotation").Get();
	FJsonObject* scale = value->GetObjectField("scale").Get();

	out.SetLocation(FVector(loc->GetNumberField("x"), loc->GetNumberField("y"), loc->GetNumberField("z")));
	out.SetRotation(FQuat().MakeFromEuler(FVector(rot->GetNumberField("x"), rot->GetNumberField("y"), rot->GetNumberField("z"))));
	out.SetScale3D(FVector(scale->GetNumberField("x"), scale->GetNumberField("y"), scale->GetNumberField("z")));

	return out;
}

FColor UKamoState::ReadColorFromJson(FJsonObject* value) {

	double r, g, b, a;
	value->TryGetNumberField("r", r);
	value->TryGetNumberField("g", g);
	value->TryGetNumberField("b", b);
	value->TryGetNumberField("a", a);

	return FColor(r, g, b, a);

}

#undef CHECKARG
