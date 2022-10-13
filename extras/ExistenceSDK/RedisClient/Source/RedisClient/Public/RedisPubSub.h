// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include <redis++/redis++.h>

#include "CoreMinimal.h"
#include "Misc/ScopeLock.h"
#include "HAL/Runnable.h"

#include "RedisPubSub.generated.h"



DECLARE_LOG_CATEGORY_EXTERN(LogRedisPubSub, Log, All);


UCLASS(Blueprintable)
class REDISCLIENT_API URedisPubSub : public UObject, public FRunnable
{
	GENERATED_BODY()

public:
	
	bool Subscribe(const FString& RedisURL, const FString& Channel);

protected:
	
	// FRunnable
	virtual uint32 Run() override;

private:

	void Consume();

	std::unique_ptr<sw::redis::Subscriber> SubscriberPtr;
	FCriticalSection Mutex;
	class FRunnableThread* Thread;

	typedef std::shared_ptr<sw::redis::Redis> RedisSP;

	RedisSP RedisPtr;


	TArray<FString> Payloads;

};