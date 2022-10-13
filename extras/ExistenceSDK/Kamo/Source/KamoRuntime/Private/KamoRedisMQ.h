// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "KamoMQ.h"
#include "KamoRedisDriver.h"

#include "CoreMinimal.h"
#include "Misc/ScopeLock.h"
#include "HAL/Runnable.h"


#if WITH_REDIS_CLIENT

class KAMORUNTIME_API KamoRedisMQ : public IKamoMQ, public KamoRedisDriver, public FRunnable
{
    
public:
	KamoRedisMQ();
	virtual ~KamoRedisMQ() override;

	// IKamoDriver
	FString GetDriverType() const override { return "mq";  }
	bool OnSessionCreated() override;
	void Tick(float DeltaTime) override;


	// IKamoMQ
	class IKamoDriver* GetDriver() override { return nullptr; }
	
	virtual bool CreateMessageQueue() override;
	virtual bool DeleteMessageQueue() override;
    virtual bool ReceiveMessage(KamoMessage& message) override;
	virtual bool SendMessage(const FString& inbox_address, const FString& message_type, const FString& payload) override;

	// FRunnable
	virtual uint32 Run() override;

private:

	void Consume();

	std::unique_ptr<sw::redis::Subscriber> subscriberPtr;
	FCriticalSection mutex;
	class FRunnableThread* thread;
	

	TArray<FString> payloads;
};

#endif // WITH_REDIS_CLIENT
