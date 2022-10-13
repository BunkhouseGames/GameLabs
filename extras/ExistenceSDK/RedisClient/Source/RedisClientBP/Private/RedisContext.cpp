
#include "RedisContext.h"
#include "RedisClientBPModule.h"


bool URedisContextBase::Check_Context(const TCHAR *command)
{
	redisContext* context = GetRedisContext();
	if (!context || context->err)
	{
		if (!command)
		{
			command = TEXT("Redis Context");
		}
		LogError(command);
		return false;
	}
	return true;
}


// error handling
void URedisContextBase::ClearError()
{
	Error = FRedisError();
}

void URedisContextBase::SetError(ERedisErr err, const FString& msg)
{
	Error.Code = err;
	Error.Message = msg;
	if (err == ERedisErr::ERR_IO)
	{
		Error.err_no = errno;
#if PLATFORM_WINDOWS
		// hiredis has translated all WSAError codes to ERRNO codes
		char buffer[256];
		if (0 == strerror_s(buffer, sizeof(buffer), Error.err_no))
		{
			Error.Message = ANSI_TO_TCHAR(buffer);
		}
#else
		Error.Message = ANSI_TO_TCHAR(strerror(Error.err_no));
#endif
	}
	else
	{
		Error.err_no = 0;
	}
	LogError(TEXT("Error occurred"));
}

void URedisContextBase::SetError(redisContext* context)
{
	if (context == nullptr)
	{
		context = GetRedisContext();
		if (context == nullptr)
		{
			return;  //don't overwrite a dead context
			UE_LOG(LogRedisClientBP, Error, TEXT("error on a NULL context"));
		}
	}
	if (context->err != 0)
	{
		SetError((ERedisErr)context->err, ANSI_TO_TCHAR(context->errstr));
	}
}

const FRedisError& URedisContextBase::GetError()
{
	// if there is no error state, but also missing context, replace error with Not connected error
	if (!Error.IsError()) {
		auto* context = GetRedisContext();
		if (!context)
		{
			SetError(ERedisErr::ERR_UNINIT, TEXT("Not connected"));
		}
	}
	return Error;
}

bool URedisContextBase::GetError(FRedisError& _error)
{
	_error = GetError();
	return _error.IsError();
}


ERedisErr URedisContextBase::GetError(FString& message)
{
	auto & err = GetError();
	message = err.Message;
	return err.Code;
}


ERedisErr URedisContextBase::GetError(FString& message, int& error_number)
{
	auto & err = GetError();
	message = err.Message;
	error_number = err.err_no;
	return err.Code;
}

FString URedisContextBase::GetErrorString()
{
	FString message;
	GetError(message);
	return message;
}

void URedisContextBase::LogError(const TCHAR *msg)
{
	FString out = FString::Printf(TEXT("%s - %s"), msg, *GetError().Format());
	UE_LOG(LogRedisClientBP, Error, TEXT("%s"), *out);
}

void URedisContextBase::LogLatentNoWorld(const TCHAR* cmd)
{
	FString msg = FString::Printf(TEXT("%s: Latent action has no world"), cmd);
	LogError(*msg);
}

void URedisContextBase::LogLatentInProgress(const TCHAR* cmd)
{
	FString msg = FString::Printf(TEXT("%s: Latent action already in progress"), cmd);
	LogError(*msg);
}

bool URedisContextBase::Check_Reply(const redisReply* reply)
{
	FString msg;
	return Check_Reply(reply, msg);
}

bool URedisContextBase::Check_Reply(const redisReply* reply, FString& message)
{
	if (!reply) {
		SetError();
		message = GetErrorString();
		Error.ReplyErrorMessage = FRedisReply::ErrorMessage(reply);
		return false;
	}
	if (FRedisReply::IsError(reply))
	{
		message = FRedisReply::ErrorMessage(reply);
		UE_LOG(LogRedisClientBP, Error, TEXT("Reply error: %s"), *message);
		Error.ReplyErrorMessage = message;
		return false;
	}
	if (reply->type == REDIS_REPLY_STATUS)
	{
		message = ANSI_TO_TCHAR(reply->str);
	}
	return true;
}

bool URedisContextBase::Check_Response(int response, FString& message)
{
	if (response == REDIS_ERR)
	{
		SetError();
		message = GetErrorString();
		return false;
	}
	return true;
}



// Redis Commands helper

RedisCommandArgs::RedisCommandArgs(const FString& cmd)
{
	EmplaceAnsi(cmd);
}

RedisCommandArgs::RedisCommandArgs(const FString& cmd, const FString &arg)
{
	EmplaceAnsi(cmd);
	EmplaceUTF8(arg);
}

RedisCommandArgs::RedisCommandArgs(const char* cmd)
{
	Emplace(cmd);
}
RedisCommandArgs::RedisCommandArgs(const char* cmd, const FString &arg)
{
	Emplace(cmd);
	EmplaceUTF8(arg);
}

RedisCommandArgs::RedisCommandArgs(const TArray<FRedisBinary>& args)
{
	for (const auto &a : args)
	{
		Emplace(a.data);
	}
}

RedisCommandArgs::RedisCommandArgs(const FString& cmd, const TArray<FString>& args)
{
	EmplaceAnsi(cmd);
	for (const FString& s : args)
	{
		EmplaceUTF8(s);
	}
}

RedisCommandArgs::RedisCommandArgs(const FString &cmd, const TArray<FString> &args, const TArray<FRedisBinary> &bargs)
{
	EmplaceAnsi(cmd);
	for (const auto &s : args)
	{
		EmplaceUTF8(s);
	}
	for (const auto &b : bargs)
	{
		Emplace(b.data);
	}
}


// emplace ascii string.  Special case, we assume it to be static.
void RedisCommandArgs::Emplace(const char *str)
{
	_argv.Emplace(str);
	_argvlen.Emplace(TCString<char>::Strlen(str));

}
void RedisCommandArgs::EmplaceUTF8(const FString& string)
{
	FTCHARToUTF8 utf8(*string);
	Emplace(utf8.Get(), utf8.Length());
}

void RedisCommandArgs::EmplaceAnsi(const FString& string)
{
	auto r = StringCast<ANSICHAR>(*string);
	Emplace(r.Get(), r.Length());
}

void RedisCommandArgs::Emplace(const char *ptr, size_t len)
{
	_data.Emplace((uint8*)ptr, len);
	_argv.Emplace((char*)_data.Last().GetData());
	_argvlen.Emplace(_data.Last().Num());
}


void RedisCommandArgs::Emplace(const TArray<uint8>& binary)
{
	_data.Emplace(binary);
	_argv.Emplace((char*)_data.Last().GetData());
	_argvlen.Emplace(_data.Last().Num());
}

void RedisCommandArgs::Extend(const RedisCommandArgs& other)
{
	_argv += other._argv;
	_argvlen += other._argvlen;
}
	
	