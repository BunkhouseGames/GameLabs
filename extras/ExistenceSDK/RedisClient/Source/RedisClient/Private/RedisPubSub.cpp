// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#include "RedisPubSub.h"


DEFINE_LOG_CATEGORY(LogRedisPubSub);


bool URedisPubSub::Subscribe(const FString& RedisURL, const FString& Channel)
{
	sw::redis::ConnectionOptions ConnectionOptions(TCHAR_TO_UTF8(*RedisURL));
	ConnectionOptions.socket_timeout = std::chrono::milliseconds(0);

	sw::redis::ConnectionPoolOptions PoolOptions;
	PoolOptions.size = 1;

	// Optional. Max time to wait for a connection. 0ms by default, which means wait forever.
	// Say, the pool size is 3, while 4 threds try to fetch the connection, one of them will be blocked.
	PoolOptions.wait_timeout = std::chrono::milliseconds(100);

	// Optional. Max lifetime of a connection. 0ms by default, which means never expire the connection.
	// If the connection has been created for a long time, i.e. more than `connection_lifetime`,
	// it will be expired and reconnected.
	PoolOptions.connection_lifetime = std::chrono::minutes(10);


	UE_LOG(LogRedisPubSub, Display, TEXT("Subscribe to channel %s on %s"), *Channel, *RedisURL);


	RedisPtr = std::make_shared<sw::redis::Redis>(ConnectionOptions, PoolOptions);

	if (!RedisPtr)
	{
		return false;
	}

	try
	{
		RedisPtr->command("client", "setname", "RedisClient-PubSub");
		auto Reply = RedisPtr->command("client", "getname");

		// If the command returns a single element,
		// use `reply::parse<T>(redisReply&)` to parse it.
		auto Val = sw::redis::reply::parse<sw::redis::OptionalString>(*Reply);

		if (Val)
		{
			UE_LOG(LogRedisPubSub, Display, TEXT("Redis connection active: %S"), Val->c_str());
		}

		SubscriberPtr->subscribe(TCHAR_TO_UTF8(*Channel));
		SubscriberPtr->on_message([this](std::string Channel, std::string Msg)
		{
			// Process message of MESSAGE type.
			//if (FString(channel.c_str()) == Key())
			{
				FScopeLock Lock(&Mutex);
				Payloads.Add(UTF8_TO_TCHAR(Msg.c_str()));
			}
		});
	}
	catch (const std::exception& e)
	{
		UE_LOG(LogRedisPubSub, Error, TEXT("Redis connection/subscription failed: %S"), e.what());
		RedisPtr.reset();
		return false;
	}



	return true;

}



void URedisPubSub::Consume()
{
    for (;;)
    {
        try
        {
            SubscriberPtr->consume();
        }
        catch (const sw::redis::TimeoutError& e)
        {
            // ignore
            UE_LOG(LogRedisPubSub, Warning, TEXT("KamoRedisMQ::ReceiveMessage timed out: %S"), e.what());
        }
        catch (const sw::redis::IoError&)
        {
            // Most likely "Failed to get reply: connection aborted". We simply quit now.
            return;
        }
        catch (const std::exception& e)
        {
            UE_LOG(LogRedisPubSub, Error, TEXT("KamoRedisMQ::ReceiveMessage failed: %S"), e.what());
            return;
        }
    }
}

uint32 URedisPubSub::Run()
{
    Consume();
    return 0;
}