// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#include "KamoRedisMQ.h"
#include "KamoFileHelper.h"

#include "KamoRedisMQ.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/RunnableThread.h"

#if WITH_REDIS_CLIENT
using namespace sw::redis;

KamoRedisMQ::KamoRedisMQ() :
    thread(nullptr)
{
}


KamoRedisMQ::~KamoRedisMQ()
{
}


bool KamoRedisMQ::OnSessionCreated()
{
    if (KamoRedisDriver::OnSessionCreated())
    {
        subscriberPtr = std::make_unique<sw::redis::Subscriber>(redisPtr->subscriber());
        return true;
    }
    else
    {
        return false;
    }
}

bool KamoRedisMQ::CreateMessageQueue()
{   
    if (!initialized)
    { 
        UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisMQ::CreateMessageQueue: Driver not initialized, tenant=%s, session=%s"), *tenant_name, *session_info); 
        return false;	
    }

    try
    {
        subscriberPtr->subscribe(StringCast<char>(*Key()).Get());
        subscriberPtr->on_message([this](std::string channel, std::string msg)
        {
            // Process message of MESSAGE type.
            if (FString(channel.c_str()) == Key())
            {
                FScopeLock lock(&mutex);
                payloads.Add(UTF8_TO_TCHAR(msg.c_str()));
            }
        });
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogKamoDriver, Error, TEXT("Redis subscribe failed: %S"), e.what());
        return false;
    }

    // Create thread for subscription channel
    thread = FRunnableThread::Create(this, TEXT("KamoRedisMQ"), 0, TPri_Normal);
    return true;
}


bool KamoRedisMQ::DeleteMessageQueue()
{
    try
    {
        subscriberPtr->unsubscribe(StringCast<char>(*Key()).Get());
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisMQ::DeleteMessageQueue failed: %S"), e.what());
        return false;
    }

    // Deleting the object will trigger a disconnect on the subscriber, which will in turn end the Consume loop 
    // and the worker thread will exit.
    // Note: It is a bad design pattern in redis++ that one needs to delete the object to trigger a disconnect.
    // As the pointer reference is defined as unique we can be somewhat certain that this is the only reference
    // to the object and a reset() will call 'delete' on the instance.
    subscriberPtr.reset();

    if (thread)
    {
        thread->WaitForCompletion();
        thread = nullptr;
    }

    return true;
}


bool KamoRedisMQ::SendMessage(const FString& inbox_address, const FString& message_type, const FString& payload)
{
    /*
    +		inbox_address	L"mq+redis://kamooneproject/ue4server.7777"	const FString &
    +		inbox_path	    L"ue4server.7777"	FString
    +		message_type	L"command"	const FString &
    +		tenant_name	    L"kamooneproject"	FString
    +		session_path	L"ue4server.7777"	FString
    +		session_url	    L"mq+redis://kamooneproject/ue4server.7777"	FString
            initialized	false	bool
    */

    FString key;
    if (inbox_address.IsEmpty())
    {
        key = Key();
    }
    else
    {
        KamoURLParts parts;
        if (!KamoURLParts::ParseURL(inbox_address, parts))
        {
            return false;
        }
        key = Key(parts);
    }

    if (message_type == "command") 
    {
        try
        {
            long long retval = redisPtr->publish(TCHAR_TO_UTF8(*key), TCHAR_TO_UTF8(*payload));
        }
        catch (const std::exception& e)
        {
            UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisMQ::SendMessage failed: %S"), e.what());
            return false;
        }
        return true;
    }
    
    return false;
}

bool KamoRedisMQ::ReceiveMessage(KamoMessage& message)
{
    if (payloads.Num())
    {
        FScopeLock lock(&mutex);
        message.message_type = "command";
        message.payload = payloads.Pop();
        return true;
    }
    else
    {
        return false;
    }
}

void KamoRedisMQ::Consume()
{
    for (;;)
    {
        try
        {
            subscriberPtr->consume();
        }
        catch (const TimeoutError& e)
        {
            // ignore
            UE_LOG(LogKamoDriver, Warning, TEXT("KamoRedisMQ::ReceiveMessage timed out: %S"), e.what());
        }
        catch (const sw::redis::IoError& e)
        {
            // Most likely "Failed to get reply: connection aborted". We simply quit now.
            return;
        }
        catch (const std::exception& e)
        {
            UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisMQ::ReceiveMessage failed: %S"), e.what());
            return;
        }
    }
}


void KamoRedisMQ::Tick(float DeltaTime)
{
}


uint32 KamoRedisMQ::Run()
{
    Consume();
    return 0;
}

#endif // WITH_REDIS_CLIENT
