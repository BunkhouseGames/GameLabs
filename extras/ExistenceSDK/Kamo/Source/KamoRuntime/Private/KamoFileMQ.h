// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "KamoMQ.h"
#include "KamoStructs.h"
#include "HAL/PlatformFileManager.h"
#include "KamoFileDriver.h"

class KAMORUNTIME_API KamoFileMQ : public IKamoMQ, public KamoFileDriver {
    
public:
	KamoFileMQ();
	virtual ~KamoFileMQ() override;

	// IKamoDriver
	FString GetDriverType() const override { return "mq";  }
	void CloseSession() override;

	// IKamoMQ
	class IKamoDriver* GetDriver() override { return nullptr; }
	
	virtual bool CreateMessageQueue() override;
	virtual bool DeleteMessageQueue() override;
    virtual bool SendMessage(const FString& inbox_address, const FString& message_type, const FString& payload) override;
    virtual bool ReceiveMessage(KamoMessage& message) override;

};
