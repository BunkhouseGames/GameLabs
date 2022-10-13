// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#include "KamoFileDB.h"
#include "KamoFileMQ.h"
#include "KamoRedisDB.h"
#include "KamoRedisMQ.h"

static FString GetDriverName(const FString& name, bool& is_url)
{
	int32 index;
	if (name.FindChar(TCHAR(':'), index))
	{
		// url is like "mq+redis://kamooneproject/ue4server.7777"
		FString driverandtype;
		FString driver;
		name.Split(TEXT(":"), &driverandtype, nullptr);
		driverandtype.Split(TEXT("+"), nullptr, &driver);
		is_url = true;
		return driver;
	}
	else
	{
		is_url = false;
		return name;
	}
}


template<typename T>
static TUniquePtr<T> CreateAndInitialize(const FString& name, bool is_url)
{
	T* driver = new T;

	if (is_url)
	{
		driver->CreateSession(name);
	}

	return TUniquePtr<T>(driver);
}


TUniquePtr<IKamoDB> IKamoDB::CreateDriver(const FString& name)
{
	bool is_url;
	FString driver_name = GetDriverName(name, is_url);

	if (driver_name == "file")
	{
		return CreateAndInitialize<KamoFileDB>(name, is_url);
	}
#if WITH_REDIS_CLIENT
	else if (driver_name == "redis")
	{
		return CreateAndInitialize<KamoRedisDB>(name, is_url);
	}
#endif
	else
	{
		UE_LOG(LogKamoDriver, Error, TEXT("Can't create DB driver of type: %s"), *driver_name);
		return nullptr;
	}
}


TUniquePtr<IKamoMQ> IKamoMQ::CreateDriver(const FString& name)
{
	TUniquePtr<IKamoMQ> mq;
	bool is_url;
	FString driver_name = GetDriverName(name, is_url);

	if (driver_name == "file")
	{
		mq = CreateAndInitialize<KamoFileMQ>(name, is_url);
	}
#if WITH_REDIS_CLIENT
	else if (driver_name == "redis")
	{
		mq = CreateAndInitialize<KamoRedisMQ>(name, is_url);
	}
#endif
	else
	{
		UE_LOG(LogKamoDriver, Error, TEXT("Can't create MQ driver of type: %s"), *driver_name);		
	}

	if (mq && is_url)
	{
		if (!mq->CreateMessageQueue())
		{
			mq.Reset();
		}
	}

	return mq;
}

