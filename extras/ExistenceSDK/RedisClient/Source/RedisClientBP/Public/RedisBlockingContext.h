#pragma once

#include "RedisContext.h"
#include "RedisTypes.h"
#include "RedisBlockingContext.generated.h"

// The blocking context is more of an experimental playground, useless in production.
// Therefore, it is possible to just skip it.

#if 0

UCLASS(Blueprintable)
class URedisBlockingContext : public URedisContextBase
{
	GENERATED_BODY()
public:

	URedisBlockingContext();
	~URedisBlockingContext();

	// connections and error getting
	UFUNCTION(BlueprintCallable, Category = "RedisClient|BlockingContext")
	bool Connect(const FString &HostName = FString(TEXT("127.0.0.1")), const int Port = 6379, float Timeout = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "RedisClient|BlockingContext")
	void Disconnect();


	// Perform a generic Redis call
	// @param BinaryArgs	Arguments in raw
	// @param Success	Did communication succeed?
	// @param Result	A single result
	// @param Results	An array of results
	UFUNCTION(BlueprintCallable, Category = "RedisClient|BlockingContext", meta = (AutoCreateRefTerm = "Args,BinaryArgs"))
	void Command(const FString& cmd, const TArray<FString> &Args, const TArray<FRedisBinary> &BinaryArgs,
		bool &Success, FRedisReply& Result, TArray<FRedisReply>& Results);


	// Specializations  We can add more as conveneitn

	// Increment/decrement by a value
	UFUNCTION(BlueprintCallable, Category = "RedisClient|BlockingContext", meta = (Keywords = "increment"))
	void IncrBy(const FString& key, bool& success, int64 &value, int64 by = 1);
	
	// Increment a by a floating point value
	UFUNCTION(BlueprintCallable, Category = "RedisClient|BlockingContext", meta = (Keywords = "increment float"))
	void IncrByFloat(const FString& key, bool& success, float& value, float by = 1.0f);

protected:
	virtual redisContext* GetRedisContext() override;


private:
	redisReply* Command_Key(const char* cmd, const FString& key, bool& success, FString& message);
	redisReply* Command_Key_Int(const char* cmd, const FString& key, int64  arg, bool& success, FString& message);
	redisReply* Command_Key_Float(const char* cmd, const FString& key, float arg, bool& success, FString& message);


private:
	redisContext* context = nullptr;
};

#endif /* WITH_ERDITORONLY_DATA */