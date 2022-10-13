#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "hiredis/hiredis.h"

#include "RedisTypes.generated.h"


// Enumfor redis Context error values
UENUM(BlueprintType)
enum class ERedisErr : uint8
{
	ERR_NONE = 0,
	ERR_IO = REDIS_ERR_IO,
	ERR_EOF = REDIS_ERR_EOF,
	ERR_PROTOCOL = REDIS_ERR_PROTOCOL,
	ERR_OOM = REDIS_ERR_OOM,
	ERR_TIMEOUT = REDIS_ERR_TIMEOUT,
	ERR_OTHER = REDIS_ERR_OTHER,
	ERR_UNINIT = 254,
};


UENUM(BlueprintType)
enum class ERedisReplyType : uint8
{
	REPLY_NONE = 0,  // something wrong with the context
	REPLY_STRING = REDIS_REPLY_STRING,
	REPLY_ARRAY = REDIS_REPLY_ARRAY,
	REPLY_INTEGER = REDIS_REPLY_INTEGER,
	REPLY_NIL = REDIS_REPLY_NIL,
	REPLY_STATUS = REDIS_REPLY_STATUS,
	REPLY_ERROR = REDIS_REPLY_ERROR,
	REPLY_DOUBLE = REDIS_REPLY_DOUBLE,
	REPLY_BOOL = REDIS_REPLY_BOOL,
	REPLY_MAP = REDIS_REPLY_MAP,
	REPLY_SET = REDIS_REPLY_SET,
	REPLY_ATTR = REDIS_REPLY_ATTR,
	REPLY_PUSH = REDIS_REPLY_PUSH,
	REPLY_VERB = REDIS_REPLY_VERB
};

// Helper to get verbatim enum error
FString EnumToString(const TCHAR* Enum, int32 EnumValue);

// Enum used for output pins on blueprints
UENUM()
enum class ERedisSuccess : uint8
{
	Success,
	Failure,
};


// Encapsulates the error state of redis for convenience.
USTRUCT(BlueprintType)
struct FRedisError
{
	GENERATED_BODY()

	bool IsError() const { return Code != ERedisErr::ERR_NONE; }
	FString Format() const;
	void Clear() { *this = FRedisError(); }

	// The redis error code
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ERedisErr Code = ERedisErr::ERR_NONE;

	// The redis error message or errno message
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString	Message;

	// system errno code, if there is IO error
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="errno"))
	int	err_no = 0;

	// The last error reply
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString ReplyErrorMessage;
};

// Raw binary value for Redis
USTRUCT(BlueprintType)
struct FRedisBinary
{
	GENERATED_BODY()
	FRedisBinary();
	FRedisBinary(const TArray<uint8>& d);
	FRedisBinary(const char* str, size_t len);
	FRedisBinary(const redisReply* reply);

	// The raw data
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<uint8> data;

	// Conversion functions for the binary.  Static, here for scoping
	FString StringFromAnsi() const;
	FString StringFromUTF8() const;
};


// A single return value from a Redis call
USTRUCT(BlueprintType)
struct FRedisReply
{
	GENERATED_BODY()

	FRedisReply();
	FRedisReply(const redisReply *reply);
	void Init(const redisReply* reply);
	static TArray<FRedisReply> InitArray(const redisReply*);

	FString Format() const;
	bool IsError() const;
	FString ErrorMessage() const;
	static bool IsError(const redisReply*);
	static FString ErrorMessage(const redisReply *);

	
	// The type of reply
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ERedisReplyType type = ERedisReplyType::REPLY_NONE;

	// Integer value of reply
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int64 integer = 0;

	// The floating point value
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float dval = 0.0f;

	//String or error message
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString str;

	//Raw binary data
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FRedisBinary binary;

	//The vtyp for REPLY_VERB replies
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString vtype;

	//The number of returned array elements
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int NumElements = 0;

	// conversion functions.  They cannot be blueprint exposed, so we implement them here
	// but expose them from the BPLibrary
	FString AsString() const;
	const FString &AsStringRef() const;
	int64 AsInt() const;
	double AsDouble() const;
};


// helper class to provide type conversions
class FRedisReplyWrap
{
public:
	FRedisReplyWrap(const redisReply *_Reply) : Reply(_Reply) {}

	// helper conversions for templates classes
	operator int64() const { return Reply->integer; }
	operator double() const {
		if (Reply->type == REDIS_REPLY_DOUBLE)
			return (float)Reply->dval;
		return FCString::Atod(*Ansi());
	}
	operator float() const { return (float)(double)*this; }
	operator FString() const { return UTF8(); }

	// conversion functions
public:
	FString Ansi() const;
	static FString Ansi(const char* str, size_t len);
	FString UTF8() const;
	static FString UTF8(const char* str, size_t len);
	FRedisBinary Binary();

private:
	const redisReply* Reply;
};
