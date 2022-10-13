// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.


#include "KamoCommand.h"

UKamoCommand* UKamoCommand::CreateKamoCommand(FString command, FString parent, FString id, UKamoState* parameters) {
    auto kamoCommand = NewObject<UKamoCommand>();
    
    kamoCommand->command = command;
    kamoCommand->parent = parent;
    kamoCommand->id = id;
    kamoCommand->parameters = parameters;
    
    return kamoCommand;
}
