// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.


#include "KamoVolume.h"
#include "Components/BoxComponent.h"

AKamoVolume::AKamoVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
    // Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    // Our root component will be a sphere that reacts to physics
    UBoxComponent* BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("RootComponent"));
    RootComponent = BoxComponent;
}


#if WITH_EDITOR

void AKamoVolume::LoadedFromAnotherClass(const FName& OldClassName)
{
	Super::LoadedFromAnotherClass(OldClassName);

}

#endif