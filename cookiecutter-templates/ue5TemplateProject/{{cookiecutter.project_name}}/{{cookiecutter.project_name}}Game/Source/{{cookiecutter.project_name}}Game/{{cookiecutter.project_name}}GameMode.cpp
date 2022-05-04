// Copyright Epic Games, Inc. All Rights Reserved.

#include "{{cookiecutter.project_name}}GameMode.h"
#include "DefaultCharacter.h"
#include "UObject/ConstructorHelpers.h"

A{{cookiecutter.project_name}}GameMode::A{{cookiecutter.project_name}}GameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
