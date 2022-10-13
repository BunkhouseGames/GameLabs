#pragma once

//#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RedisTypes.h"

#include "RedisContext.generated.h"


UCLASS(Abstract)
class URedisContextBase : public UObject
{
	GENERATED_BODY(Blueprintable)

public:

	bool Check_Context(const TCHAR *cmd=nullptr);
	
	// Set the error object
	void ClearError();
	void SetError(ERedisErr err, const FString& msg);
	void SetError(redisContext* context=nullptr);
	bool GetError(FRedisError& error);
	const FRedisError& GetError();
	void LogError(const TCHAR* msg);
	void LogLatentNoWorld(const TCHAR* func);
	void LogLatentInProgress(const TCHAR* func);


	// other accessors
	ERedisErr GetError(FString& message);
	ERedisErr GetError(FString& message, int &error_no);
	FString GetErrorString();

	// deprecated helpers
	bool Check_Response(int response, FString& message);
	bool Check_Reply(const redisReply* reply, FString& message);
	bool Check_Reply(const redisReply* reply);

protected:
	virtual redisContext* GetRedisContext() PURE_VIRTUAL(URedisContext::GetRedisContext, return nullptr;);
	
public:
	/** The most recent error encountered */
	UPROPERTY(BlueprintReadOnly)
	FRedisError Error;
};


// helper to assemble redis arguments
class RedisCommandArgs
{
public:
	RedisCommandArgs() {}
	RedisCommandArgs(const FString& cmd);
	RedisCommandArgs(const char *cmd);
	RedisCommandArgs(const char* cmd, const FString &arg);
	RedisCommandArgs(const FString& cmd, const FString& arg);
	RedisCommandArgs(const FString& cmd, const TArray<FString>& args);
	RedisCommandArgs(const FString& cmd, const TArray<FString>& args, const TArray<FRedisBinary>&bargs);
	RedisCommandArgs(const TArray<FRedisBinary>& args);
public:
	void Emplace(const char* str);
	void EmplaceUTF8(const FString& str);
	void EmplaceAnsi(const FString& str);
	void Emplace(const char *ptr, size_t len);
	void Emplace(const TArray<uint8> &bin);
	
	int argc() const { return _argv.Num(); }
	const char** argv() { return _argv.GetData(); }
	const size_t* argvlen() { return _argvlen.GetData(); }

	void Extend(const RedisCommandArgs& other);
	

private:
	TArray<TArray<uint8>> _data;
	TArray<const char*> _argv;
	TArray<size_t> _argvlen;
};


