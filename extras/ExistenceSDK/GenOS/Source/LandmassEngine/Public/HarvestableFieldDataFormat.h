// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "StaticSlotInfo.h"

#include "HarvestableFieldDataFormat.generated.h"

USTRUCT(BlueprintType)
struct FHarvestableFieldDataFormat
{
	GENERATED_BODY()

	UPROPERTY()
	FString Version = TEXT("1.0");

	UPROPERTY()
	FName FieldName;

	UPROPERTY()
	TArray<FStaticSlotInfo> Slots;
	
	UPROPERTY()
	FName AssetName;

	UPROPERTY()
	FTransform Transform;

};
