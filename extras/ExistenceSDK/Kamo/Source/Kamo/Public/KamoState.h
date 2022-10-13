// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Json.h"
#include "Templates/SharedPointer.h"
#include "UObject/NoExportTypes.h"

#include "KamoState.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogKamoState, Log, All);


UCLASS(BlueprintType)
class KAMO_API UKamoState : public UObject
{
	GENERATED_BODY()

public:
	//Setters

	UFUNCTION(BlueprintPure, Category = "KamoState")
	static UPARAM(DisplayName = "KamoState") UKamoState* CreateKamoState();
	
	void SetObjectField(const FString& key, const TSharedPtr< FJsonObject >& JsonObject);
	void SetObjectField(const FString& key, UKamoState* ObjectState);
	
	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetString(const FString& key, const FString& value);

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetObjectReference(const FString& key, const FString& value);

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetInt(const FString& key, const int& value);

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetFloat(const FString& key, const float& value);

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetBool(const FString& key, const bool& value);

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetVector(const FString& key, const FVector& value);

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetQuat(const FString& key, const FQuat& value);

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetTransform(const FString& key, const FTransform& value);

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetStringArray(const FString& key, const TArray<FString>& valueArray);

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetIntArray(const FString& key, const TArray<int>& valueArray);

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetFloatArray(const FString& key, const TArray<float>& valueArray);

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetBoolArray(const FString& key, const TArray<bool>& valueArray);

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetVectorArray(const FString& key, const TArray<FVector>& valueArray);

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetTransformArray(const FString& key, const TArray<FTransform>& valueArray);

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetRotator(const FString& key, const FRotator& value);

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetRotatorArray(const FString& key, const TArray<FRotator>& valueArray);

	//Getters
	const TSharedPtr< FJsonObject >& GetObjectField(const FString& FieldName) const;

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	UPARAM(DisplayName = "Exists") bool GetRotator(const FString& key, FRotator& result);

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	UPARAM(DisplayName = "Exists") bool GetRotatorArray(const FString& key, TArray<FRotator>& resultArray);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	UPARAM(DisplayName = "Exists") bool GetString(const FString& key, FString& result);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	UPARAM(DisplayName = "Exists") bool GetObjectReference(const FString& key, FString& result);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	UPARAM(DisplayName = "Exists") bool GetFloat(const FString& key, float& result);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	UPARAM(DisplayName = "Exists") bool GetInt(const FString& key, int& result);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	UPARAM(DisplayName = "Exists") bool GetBool(const FString& key, bool& result);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	void GetKeys(const FString& dataType, TArray<FString>& result);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	UPARAM(DisplayName = "Exists") bool GetVector(const FString& key, FVector& result);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	UPARAM(DisplayName = "Exists") bool GetQuat(const FString& key, FQuat& result);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	UPARAM(DisplayName = "Exists") bool GetTransform(const FString& key, FTransform& result);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	UPARAM(DisplayName = "Exists") bool GetColor(const FString& key, FColor& result);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	UPARAM(DisplayName = "Exists") bool GetStringArray(const FString& key, TArray<FString>& result);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	UPARAM(DisplayName = "Exists") bool GetIntArray(const FString& key, TArray<int>& result);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	UPARAM(DisplayName = "Exists") bool GetFloatArray(const FString& key, TArray<float>& result);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	UPARAM(DisplayName = "Exists") bool GetBoolArray(const FString& key, TArray<bool>& result);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	UPARAM(DisplayName = "Exists") bool GetVectorArray(const FString& key, TArray<FVector>& result);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	UPARAM(DisplayName = "Exists") bool GetTransformArray(const FString& key, TArray<FTransform>& result);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	FString GetStateAsString() const;

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetKamoState(const FString& key, UKamoState* kamo_state);

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetKamoStateArray(const FString& key, const TArray<UKamoState*>& array);

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void AddToKamoStateList(const FString& key, UKamoState* kamo_state);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	bool GetKamoState(const FString& key, UKamoState*& result);

	UFUNCTION(BlueprintPure, Category = "KamoState")
	TArray<FString> GetKamoStateKeys() const;

	UFUNCTION(BlueprintCallable, Category = "KamoState")
	bool GetKamoStateArray(const FString& key, TArray<UKamoState*>& result);

	UFUNCTION(Category = "KamoState")
	bool SetState(const FString& jsonString);

	/*Replaces the state from json string, if deserialization is unsuccessful the state will be empty*/
	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetStateFromJsonString(const FString& jsonString, bool& success);

	void SetStateFromProperty(UObject* Object, FBoolProperty* BoolProp);
	void SetStateFromProperty(UObject* Object, FIntProperty* IntProp);
	void SetStateFromProperty(UObject* Object, FFloatProperty* FloatProp);
	void SetStateFromProperty(UObject* Object, FStrProperty* StringProp);
	void SetStateFromProperty(UObject* Object, FNameProperty* NameProp);
	void SetStateFromProperty(UObject* Object, FStructProperty* StructProp);
	void SetStateFromProperty(UObject* Object, FArrayProperty* ArrayProp);
	void SetStateFromProperty(UObject* Object, FMapProperty* MapProp);
	void SetStateFromProperty(UObject* Object, FSetProperty* SetProp);

	void SetPropertyFromState(UObject* Object, FBoolProperty* BoolProp);
	void SetPropertyFromState(UObject* Object, FIntProperty* IntProp);
	void SetPropertyFromState(UObject* Object, FFloatProperty* FloatProp);
	void SetPropertyFromState(UObject* Object, FStrProperty* StringProp);
	void SetPropertyFromState(UObject* Object, FNameProperty* NameProp);
	void SetPropertyFromState(UObject* Object, FStructProperty* StructProp);
	void SetPropertyFromState(UObject* Object, FArrayProperty* ArrayProp);
	void SetPropertyFromState(UObject* Object, FMapProperty* ArrayProp);
	void SetPropertyFromState(UObject* Object, FSetProperty* SetProp);


	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetStateFromProperty(UObject* Object, const FString& PropertyName);
	
	UFUNCTION(BlueprintCallable, Category = "KamoState")
	void SetStateFromProperties(UObject* Object, const TArray<FString>& PropertyNames);
	void SetPropertyFromState(UObject* Object, const FString& PropertyName);

	void SetPropertiesFromState(UObject* Object, const TArray<FString>& PropertyNames);

	const TSharedRef<FJsonObject>& GetJsonObjectState() const 
	{
		return localState;
	}

	void PopulateFromField(UKamoState* Other, const FString& FieldName);

private:
	TSharedRef<FJsonObject> localState = MakeShareable(new FJsonObject);

	TSharedPtr<FJsonObject> CreateJsonVector(const FVector& value);
	TSharedPtr<FJsonObject> CreateJsonQuat(const FQuat& value);
	TSharedRef<FJsonObject> CreateJsonTransform(const FTransform& value);
	TSharedPtr<FJsonObject> CreateJsonRotator(const FRotator& value);

	FVector ReadVectorFromJson(FJsonObject* value);
	FQuat ReadQuatFromJson(FJsonObject* value);
	FTransform ReadTransformFromJson(FJsonObject* value);
	FColor ReadColorFromJson(FJsonObject* value);
	FRotator ReadRotatorFromJson(FJsonObject* value);
};
