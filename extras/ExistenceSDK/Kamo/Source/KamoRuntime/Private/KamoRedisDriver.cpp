// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.


#include "KamoRedisDriver.h"
#include "KamoRuntimeModule.h"
#include "Misc/CString.h"

#if WITH_REDIS_CLIENT

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif

#include <redis++/redis++.h>

// If target is editor then it's probably a .dll project which means we need to explicitly pull the code in here
#if WITH_EDITOR
#include <Redis++Library.inl>
#endif

#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif

using namespace sw::redis;

KamoRedisDriver::KamoRedisDriver()
{
}


bool KamoRedisDriver::OnSessionCreated()
{
	// Redis URL is specified in environment variable or on command line.
	// The default is localhost:6379/0 and no password.
	// Example: redis://localhost:6379/0

	FString origin = "environment variable KAMO_REDIS_URL";
	FString redis_url = FPlatformMisc::GetEnvironmentVariable(TEXT("KAMO_REDIS_URL"));

	if (redis_url.IsEmpty())
	{
		FParse::Value(FCommandLine::Get(), TEXT("-kamoredisurl="), redis_url);
		origin = "command line argument -kamoredisurl";
	}

	if (redis_url.IsEmpty())
	{
		redis_url = "redis://127.0.0.1:6379/0";
		origin = "default values";
	}

	FString host = "127.0.0.1";
	int32 port = 6379;
	int database = 0;

	if (!redis_url.StartsWith("redis://"))
	{
		UE_LOG(LogKamoDriver, Error, TEXT("Malformed redis driver URL set from %s: %s "), *origin, *redis_url);
		return false;
	}

	FString url = redis_url.RightChop(8); // Chop 'redis://' from the string
	FString left, right;

	if (url.Split("/", &left, &right))
	{
		database = FCString::Atoi(*right);
		url = left;
	}

	if (url.Split(":", &left, &right))
	{
		port = FCString::Atoi(*right);
		url = left;
	}

	if (!url.IsEmpty())
	{
		host = url;
	}

	ConnectionOptions connectionOptions;
	connectionOptions.host = StringCast<char>(*host).Get();
	connectionOptions.port = port;
	connectionOptions.password = "";
	connectionOptions.db = database;
	connectionOptions.socket_timeout = std::chrono::milliseconds(0);

	ConnectionPoolOptions poolOptions;
	poolOptions.size = 3;  // Pool size, i.e. max number of connections.

	// Optional. Max time to wait for a connection. 0ms by default, which means wait forever.
	// Say, the pool size is 3, while 4 threds try to fetch the connection, one of them will be blocked.
	poolOptions.wait_timeout = std::chrono::milliseconds(100);

	// Optional. Max lifetime of a connection. 0ms by default, which means never expire the connection.
	// If the connection has been created for a long time, i.e. more than `connection_lifetime`,
	// it will be expired and reconnected.
	poolOptions.connection_lifetime = std::chrono::minutes(10);


	UE_LOG(LogKamoDriver, Display, TEXT("Initializing Redis driver for %s using %s: %s "), *GetSessionURL(), *origin, *redis_url);
	
	redisPtr = CreateRedisInstance(connectionOptions, poolOptions);
	if (!redisPtr)
	{
		return false;
	}

	return true;
}


RedisSP KamoRedisDriver::CreateRedisInstance(const sw::redis::ConnectionOptions& connectionOptions, const sw::redis::ConnectionPoolOptions& poolOptions) const
{

	RedisSP redis = std::make_shared<Redis>(connectionOptions, poolOptions);

	if (redis)
	{
		try
		{
			redis->command("client", "setname", StringCast<char>(*Key()).Get());
			auto r = redis->command("client", "getname");

			// If the command returns a single element,
			// use `reply::parse<T>(redisReply&)` to parse it.
			auto val = reply::parse<OptionalString>(*r);

			if (val)
			{
				UE_LOG(LogKamoDriver, Display, TEXT("Redis connection active: %S"), val->c_str());
			}
		}
		catch (const std::exception& e)
		{
			UE_LOG(LogKamoDriver, Error, TEXT("Redis connection failed: %S"), e.what());
			redis.reset();
		}
	}

	return redis;
}


void KamoRedisDriver::CloseSession()
{
	IKamoDriver::CloseSession();
	redisPtr.reset();
}


FString KamoRedisDriver::Key(const FString& key) const
{
	KamoURLParts parts;
	parts.type = GetDriverType();
	parts.tenant = tenant_name;
	parts.session_info = session_info;

	return Key(parts, key);
}


FString KamoRedisDriver::Key(const KamoURLParts& parts, const FString& key)
{
	// Format: ko:<tenant>:<type>[:session info][:key]
	FString fullkey = FString::Printf(TEXT("ko:%s:%s"), *parts.tenant, *parts.type);

	if (!parts.session_info.IsEmpty())
	{
		fullkey += TEXT(":");
		fullkey += parts.session_info;
	}

	if (!key.IsEmpty())
	{
		fullkey += TEXT(":");
		fullkey += key;
	}

	return fullkey;
}

#endif // WITH_REDIS_CLIENT
