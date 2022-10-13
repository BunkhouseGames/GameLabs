// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include <KamoPersistable.h>

#include "GameFramework/Actor.h"
#include "KamoActorWithProperties.generated.h"

UCLASS()
class KAMO_API AKamoActorWithProperties : public AActor, public IKamoPersistable
{
private:
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AKamoActorWithProperties();

	virtual TArray<FString> GetKamoPersistedProperties_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int IntProp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString StringProp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FString, FString> StringMapProp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSet<FString> StringSetProp;

};
