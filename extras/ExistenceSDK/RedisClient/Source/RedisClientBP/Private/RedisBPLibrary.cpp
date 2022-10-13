
#include "RedisBPLibrary.h"
#include "RedisClientBPModule.h"



#if 0
// Create A new context
URedisBlockingContext* URedisBPLibrary::CreateBlockingContext()
{
	URedisBlockingContext* context = NewObject<URedisBlockingContext>();
	return context;
}

#endif

URedisContext* URedisBPLibrary::CreateAsyncContext()
{
	URedisContext* context = NewObject<URedisContext>();
	return context;
}



FString URedisBPLibrary::FromUTF8(const FRedisBinary& binary)
{
	FUTF8ToTCHAR convert((char*)binary.data.GetData(), binary.data.Num());
	return FString(convert.Length(), convert.Get());
}

FRedisBinary URedisBPLibrary::ToUTF8(const FString& text)
{
	FTCHARToUTF8 utf8(*text);
	return FRedisBinary(TArray<uint8>((uint8*)utf8.Get(), utf8.Length()));
}


TArray<FString> URedisBPLibrary::ReplyArrayToStrings(const TArray<FRedisReply> &Array)
{
	TArray<FString> result;
	for (const auto &b : Array)
	{
		result.Emplace(b.AsString());
	}
	return result;
}

FString URedisBPLibrary::Int64ToString(int64 integer)
{
	return FString::Printf(TEXT("%lld"), integer);
}

FString URedisBPLibrary::FormatError(const FRedisError& error)
{
	return error.Format();
}

bool URedisBPLibrary::IsError(const FRedisError& error)
{
	return error.IsError();
}

FString URedisBPLibrary::FormatReply(const FRedisReply& reply)
{
	return reply.Format();
}


bool URedisBPLibrary::IsSuccess(const FRedisReply& reply)
{
	return !reply.IsError();
}

FString URedisBPLibrary::ReplyErrorMessage(const FRedisReply& reply)
{
	return reply.ErrorMessage();
}

bool URedisBPLibrary::IsArray(const FRedisReply& reply)
{
	return
		reply.type != ERedisReplyType::REPLY_ARRAY ||
		reply.type != ERedisReplyType::REPLY_MAP ||
		reply.type != ERedisReplyType::REPLY_SET ||
		reply.type != ERedisReplyType::REPLY_ATTR;
}

int64 URedisBPLibrary::AsInteger(const FRedisReply& reply)
{
	return reply.AsInt();
}

bool URedisBPLibrary::AsBoolean(const FRedisReply& reply)
{
	return AsInteger(reply) != 0;
}

float URedisBPLibrary::AsFloat(const FRedisReply& reply)
{
	return reply.AsDouble();
}

FString URedisBPLibrary::AsString(const FRedisReply& reply)
{
	if (IsSuccess(reply))
	{
		return reply.AsStringRef();
	}
	return TEXT("");
}


// our WorldObject, an object that has a world reference and can thus
// execute latent blueprints and other such things.
// The CDO verison must return nullptr, so the blueprint editoc can know that this is an object
// that defines a non-default GetWorld(), and thus normally has access to a world.
UWorld* URedisWorldObject::GetWorld() const
{
	// CDO objects do not belong to a world
	// If the object's outer is destroyed or unreachable we are shutting down and the world should be nullptr
	if (!HasAnyFlags(RF_ClassDefaultObject) && ensureMsgf(GetOuter(), TEXT("RedsWorldObject: %s has a null OuterPrivate in AActor::GetWorld()"), *GetFullName())
		&& !GetOuter()->HasAnyFlags(RF_BeginDestroyed) && !GetOuter()->IsUnreachable())
	{
		// Try custom world context object first.
		if (WorldContextObject)
		{
			UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
			if (World)
			{
				return World;
			}
		}
		UObject* outer = GetOuter();
		if (outer)
		{
			return outer->GetWorld();
		}
	}
	return nullptr;
}
