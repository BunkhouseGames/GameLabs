
#include "RedisTypes.h"



// magic code to get enum display name.  taken from the internet
FString EnumToString(const TCHAR* Enum, int32 EnumValue)
{
	const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, Enum, true);
	if (!EnumPtr)
		return NSLOCTEXT("Invalid", "Invalid", "Invalid").ToString();

	return EnumPtr->GetDisplayNameTextByValue(EnumValue).ToString();
}


FString FRedisError::Format() const
{
	FString err = EnumToString(TEXT("ERedisErr"), (int32) Code);
	FString result;
	if (err_no) {
		result = FString::Printf(TEXT("Error: %s:\"%s\" (Errno:%d)"), *err, *Message, err_no);
	}
	else
	{
		result = FString::Printf(TEXT("Error: %s:\"%s\""), *err, *Message);
	}
	if (ReplyErrorMessage.Len())
	{
		result += FString::Printf(TEXT(" Reply Err: \"%s\""), *ReplyErrorMessage);
	}
	return result;
}


FRedisBinary::FRedisBinary()
{}

FRedisBinary::FRedisBinary(const TArray<uint8>& d)
	: data(d)
{}
FRedisBinary::FRedisBinary(const char* str, size_t len)
	: data((uint8*)str, len)
{}
FRedisBinary::FRedisBinary(const redisReply *reply)
	: data((uint8*)reply->str, reply->len)
{}

FString FRedisBinary::StringFromAnsi() const
{
	return FRedisReplyWrap::Ansi((char*)data.GetData(), data.Num());
}

FString FRedisBinary::StringFromUTF8() const
{
	return FRedisReplyWrap::UTF8((char*)data.GetData(), data.Num());
}


// FRedisReply, a simple reply struct


FRedisReply::FRedisReply()
{}

FRedisReply::FRedisReply(const redisReply* reply) :
	type((ERedisReplyType)reply->type),
	integer(reply->integer),
	dval(reply->dval),
	binary(reply->str, reply->len),
	vtype(ANSI_TO_TCHAR(reply->vtype)),
	NumElements(reply->elements)
{
	str = binary.StringFromUTF8();
}

bool FRedisReply::IsError() const
{
	return type == ERedisReplyType::REPLY_NONE || type == ERedisReplyType::REPLY_ERROR;
}

bool FRedisReply::IsError(const redisReply* reply)
{
	return reply == nullptr || reply->type == REDIS_REPLY_ERROR;
}

FString FRedisReply::ErrorMessage() const
{
	if (type == ERedisReplyType::REPLY_NONE)
	{
		return TEXT("No reply (connection error)");
	}
	if (type == ERedisReplyType::REPLY_ERROR)
	{
		return AsStringRef();
	}
	return FString();
}

FString FRedisReply::ErrorMessage(const redisReply *reply)
{
	if (reply == nullptr)
	{
		return TEXT("No reply (connection error)");
	}
	if (reply->type == REDIS_REPLY_ERROR)
	{
		return ANSI_TO_TCHAR(reply->str);
	}
	return FString();
}


void FRedisReply::Init(const redisReply* reply)
{
	if (reply) {
		*this = FRedisReply(reply);
	}
	else
	{
		*this = FRedisReply();
	}
}

TArray<FRedisReply> FRedisReply::InitArray(const redisReply* reply)
{
	TArray<FRedisReply> result;
	for (int i = 0; i < reply->elements; i++)
	{
		auto e = reply->element[i];
		result.Emplace(e);
	}
	return result;
}


FString FRedisReply::Format() const
{
	FString tstr = EnumToString(TEXT("ERedisReplyType"), (int)type);
	if (!IsError()) {
		FString msg = AsString();
		if (msg.Len() > 40) {
			msg = msg.Left(40);
			msg += TEXT("...");
		}
		return FString::Printf(TEXT("Type:%s, str:\"%s\", int:%lld, float:%f, elements:%d"),
			*tstr, *msg, AsInt(), AsDouble(), NumElements);
	}
	else {
		return FString::Printf(TEXT("Type:%s, err:\"%s\""), *ErrorMessage());
	}
}


FString FRedisReply::AsString() const
{
	return str;
}

const FString &FRedisReply::AsStringRef() const
{
	return str;
}

int64 FRedisReply::AsInt() const
{
	if (type == ERedisReplyType::REPLY_INTEGER)
		return integer;
	return FCString::Atoi64(*AsString());
}

double FRedisReply::AsDouble() const
{
	if (type == ERedisReplyType::REPLY_DOUBLE)
		return dval;
	return FCString::Atod(*AsString());
}


// The redis reply wrap, helper to work with replies directly
FString FRedisReplyWrap::UTF8() const
{
	return UTF8(Reply->str, Reply->len);
}
FString FRedisReplyWrap::UTF8(const char* str, size_t len)
{
	FUTF8ToTCHAR convert(str, len);
	FString result(convert.Length(), convert.Get());  // string constructor takes length first if provided.
	return result;
}

FString FRedisReplyWrap::Ansi() const
{
	return Ansi(Reply->str, Reply->len);
}
FString FRedisReplyWrap::Ansi(const char* str, size_t len)
{
	auto r = StringCast<TCHAR>(str, len);
	return FString(r.Length(), r.Get());
}

FRedisBinary FRedisReplyWrap::Binary()
{
	return FRedisBinary(Reply->str, Reply->len);
}