// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "KamoDriver.h"

#include "CoreMinimal.h"
#include "Templates/UniquePtr.h"
#include "KamoStructs.h"
#include "KamoDriver.h"

class KAMORUNTIME_API IKamoMQ : public virtual IKamoDriver
{
    
public:
    virtual ~IKamoMQ() { };

    virtual class IKamoDriver* GetDriver() = 0;

    static TUniquePtr<IKamoMQ> CreateDriver(const FString& name);
    
    virtual bool CreateMessageQueue() = 0;
    virtual bool DeleteMessageQueue() = 0;    
    virtual bool ReceiveMessage(KamoMessage& message) = 0;
    virtual bool SendMessage(const FString& inbox_address, const FString& message_type, const FString& payload) = 0;
};
