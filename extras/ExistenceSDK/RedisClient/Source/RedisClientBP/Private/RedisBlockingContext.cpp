
#include "RedisBlockingContext.h"


// disable the blocking context
#if 0

#include "RedisClientBPModule.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
#include <winsock2.h> /* For struct timeval */
#include "Windows/PostWindowsApi.h" 
#include "Windows/HideWindowsPlatformTypes.h"
#endif

URedisBlockingContext::URedisBlockingContext()
{}

URedisBlockingContext::~URedisBlockingContext()
{
	if (context)
	{
		redisFree(context);
	}
}

bool URedisBlockingContext::Connect(const FString& HostName, const int Port, float timeout)
{
	// don't connect if already connected and there is no error state
	if (context && !context->err)
		return false;
	if (context) {
		redisFree(context);
	}
	if (timeout < 0.0f)
	{
		context = redisConnect(TCHAR_TO_ANSI(*HostName), Port);
	}
	else
	{
		UE_LOG(LogRedisClientBP, Log, TEXT("Connecting to %s:%d, timeout %f"), *HostName, Port, timeout);
		struct timeval s_timeout = { (int)timeout, (int)(1e6f * (timeout - (int)timeout))};
		context = redisConnectWithTimeout(TCHAR_TO_ANSI(*HostName), Port, s_timeout);
	}
	if (!context)
	{
		SetError(ERedisErr::ERR_OOM, TEXT("No Memory"));
		LogError(TEXT("Connect"));
	}
	else if (context->err)
	{ 
		SetError();
		LogError(TEXT("Connect"));
	}
	else
	{
		UE_LOG(LogRedisClientBP, Log, TEXT("Connect: connected"));
	}
	return (context && !context->err);
}

void URedisBlockingContext::Disconnect()
{
	if (context)
	{
		redisFree(context);
	}
	context = nullptr;
}


redisContext* URedisBlockingContext::GetRedisContext()
{
	return URedisBlockingContext::context;
}


void URedisBlockingContext::Command(const FString& cmd, const TArray<FString> &Args, const TArray<FRedisBinary> &BinaryArgs, bool &success, FRedisReply& result, TArray<FRedisReply>& results)
{
	if (!Check_Context(TEXT("Command")))
	{
		return;
	}
	RedisCommandArgs args(cmd, Args, BinaryArgs);
	
	redisReply* reply = (redisReply*)redisCommandArgv(context, args.argc(), args.argv(), args.argvlen());
	if (reply)
	{
		success = true;
		result.Init(reply);
		results = FRedisReply::InitArray(reply);
	}
	else
	{
		success = false;
	}
	freeReplyObject(reply);
}

// Incr and decr

void URedisBlockingContext::IncrBy(const FString& key, bool& success, int64& value, int64 by)
{
	FString message;
	redisReply* reply = Command_Key_Int("INCRBY", key, by, success, message);
	if (!reply) {
		return;
	}
	if (reply->type == REDIS_REPLY_INTEGER)
	{
		value = reply->integer;
	}
	else {
		FRedisReply r(reply);
		value = r.AsInt();
	}
	freeReplyObject(reply);
}
	

void URedisBlockingContext::IncrByFloat(const FString& key, bool& success, float& value, float by)
{
	FString message;
	redisReply* reply = Command_Key_Float("INCRBYFLOAT", key, by, success, message);
	if (!reply) {
		return;
	}
	if (reply->type == REDIS_REPLY_DOUBLE)
	{
		value = reply->dval;
	}
	else
	{
		FRedisReply r(reply);
		value = r.AsDouble();
	}
	freeReplyObject(reply);
}


// boilerplate for doing simple commands
redisReply* URedisBlockingContext::Command_Key(const char* cmd, const FString& key, bool& success, FString& message)
{
	success = Check_Context(TEXT("Command_Key"));
	if (!success)
	{
		message = GetErrorString();
		return nullptr;
	}
	redisReply* reply = (redisReply*)redisCommand(context, "%s %s", cmd, TCHAR_TO_UTF8(*key));
	if (!Check_Reply(reply, message))
	{
		freeReplyObject(reply);
		success = false;
		return nullptr;
	}
	return reply;
}

redisReply* URedisBlockingContext::Command_Key_Int(const char* cmd, const FString& key, int64 arg, bool& success, FString& message)
{
	success = Check_Context(TEXT("Command_Key_Int"));
	if (!success)
	{
		message = GetErrorString();
		return nullptr;
	}
	redisReply* reply = (redisReply*)redisCommand(context, "%s %s %lld", cmd, TCHAR_TO_UTF8(*key), arg);
	if (!Check_Reply(reply, message))
	{
		freeReplyObject(reply);
		success = false;
		return nullptr;
	}
	return reply;
}

redisReply* URedisBlockingContext::Command_Key_Float(const char* cmd, const FString& key, float arg, bool& success, FString& message)
{
	success = Check_Context(TEXT("Command_Key_Float"));
	if (!success)
	{
		message = GetErrorString();
		return nullptr;
	}
	redisReply* reply = (redisReply*)redisCommand(context, "%s %s %f", cmd, TCHAR_TO_UTF8(*key), arg);
	if (!Check_Reply(reply, message))
	{
		freeReplyObject(reply);
		success = false;
		return nullptr;
	}
	return reply;
}


#endif /* WITH_EDITORONLY_DATA */