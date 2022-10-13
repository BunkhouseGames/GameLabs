// Fill out your copyright notice in the Description page of Project Settings.


#include "KamoActorWithProperties.h"


AKamoActorWithProperties::AKamoActorWithProperties() :
	IntProp(42),
	StringProp(TEXT("This is a test"))
{
	PrimaryActorTick.bCanEverTick = false;
}

TArray<FString> AKamoActorWithProperties::GetKamoPersistedProperties_Implementation()
{
	static TArray<FString> Props = {TEXT("IntProp"), TEXT("StringProp"), TEXT("StringMapProp"), TEXT("StringSetProp") };
	return Props;
}
