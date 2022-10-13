#pragma once

#include "RedisContext.h"
#include "Tickable.h"
#include "Engine/LatentActionManager.h"

#include "RedisAsyncContext.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAsyncContextDisconnect, URedisContext*, context, bool, Success, FRedisError, error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAsyncPublishedMessage, FString, Pattern, FString, Channel, FRedisBinary, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAsyncPushedMessage, const FRedisReply&, Reply, const TArray<FRedisReply>&, Elements);


// A special context which supports the FTickable interface and
// pumps the async redis socket each frame.
class FRedisAsyncAdapter: public FTickableGameObject
{
public:
	
	// access the context
	virtual redisAsyncContext *GetAsyncContext() = 0;
	// Bind to the context
	void Bind();

	// FTickableGameObject overrides
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

private:
	static void addRead(void* privdata);
	static void delRead(void* privdata);
	static void addWrite(void* privdata);
	static void delWrite(void* privdata);
	static void scheduleTimer(void* privdata, struct timeval tv);
	void checkTimeout(redisAsyncContext *context);

private:
	bool want_read = false;
	bool want_write = false;
	bool have_timeout = false;
	double deadline;
};



DECLARE_DYNAMIC_DELEGATE(FAsyncResultNone);
DECLARE_DYNAMIC_DELEGATE_OneParam(FAsyncResultSuccess, bool, success);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FAsyncResultInt64, FString, Cookie, int64, Result);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FAsyncResultFloat, FString, Cookie, float, Result);
DECLARE_DYNAMIC_DELEGATE_FourParams(FAsyncResultGeneric, FString, Cookie, bool, success, const FRedisReply&, Reply, const TArray<FRedisReply>&, Elements);

class URedisAsyncSession;

UCLASS(Blueprintable)
class REDISCLIENTBP_API URedisContext : public URedisContextBase, public FRedisAsyncAdapter
{
	GENERATED_BODY(Blueprintable)
public:

	URedisContext();
	~URedisContext();

	/** Is the context connected? */
	UFUNCTION(BlueprintPure, Category = "RedisClient|Context")
		bool IsConnected() const;

	/**
	* Connects the context
	*/
	UFUNCTION(BlueprintCallable, Category = "RedisClient|Context", meta = (Latent, WorldContext = "WorldContextObject", LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Exec"))
		void Connect(UObject* WorldContextObject, FLatentActionInfo LatentInfo,
			ERedisSuccess& Exec,
			const FString& HostName = FString(TEXT("127.0.0.1")), const int Port = 6379, float Timeout = -1.0f
		);

	/**
	* Connects the context, event version
	*/
	UFUNCTION(BlueprintCallable, Category = "RedisClient|Context", meta = (DisplayName = "Connect (Event)"))
		void ConnectEvent(
			const FAsyncResultSuccess& Completed,
			const FString& HostName = FString(TEXT("127.0.0.1")), const int Port = 6379, float Timeout = -1.0f
		);
	bool ConnectCommon(
		ERedisSuccess& Exec,
		const FString& HostName, const int Port, float
	);

	 /**
	  * Disconnect the context.
	  * Pending requests will finish but no new ones will be accepted.
	  * @param Hard If true, pending requests will be terminated with error.
	  */
	UFUNCTION(BlueprintCallable, Category = "RedisClient|Context", meta = (Latent, WorldContext = "WorldContextObject", LatentInfo = "LatentInfo"))
		void Disconnect(UObject* WorldContextObject, FLatentActionInfo LatentInfo, bool Hard=false);
	/**
	 * Disconnect the context, Event version
	 * Pending requests will finish but no new ones will be accepted.
	 * @param Hard If true, pending requests will be terminated with error.
	 */
	UFUNCTION(BlueprintCallable, Category = "RedisClient|Context", meta = (DisplayName = "Disconnect (Event)", AutoCreateRefTerm="Completed"))
		void DisconnectEvent(
			const FAsyncResultNone &Completed, 
			bool Hard=false);

	
	/**
	 * Perform a generic Redis call
	 * @param BinaryArgs	Arguments in raw
	 * @param complete		Completion event
	 */
	UFUNCTION(BlueprintCallable, Category = "RedisClient|Context", meta = (Latent, LatentInfo="LatentInfo", ExpandEnumAsExecs="Exec", WorldContext="WorldContext", AutoCreateRefTerm = "Args,BinaryArgs"))
		void Command(UObject *WorldContext, struct FLatentActionInfo LatentInfo, const FString& cmd, const TArray<FString>& Args, const TArray<FRedisBinary>& BinaryArgs, FRedisReply& Reply, TArray<FRedisReply>& Elements, ERedisSuccess &Exec);

	/**
	 * Perform a generic Redis call (Event version)
	 * @param BinaryArgs	Arguments in raw
	 */
	UFUNCTION(BlueprintCallable, Category = "RedisClient|Context", meta = (AutoCreateRefTerm = "Args,BinaryArgs,Completed", DisplayName = "Command (Event)"))
		void CommandEvent(const FString &cmd, const TArray<FString> &Args, const TArray<FRedisBinary> &BinaryArgs,
			const FAsyncResultGeneric &Completed,
			const FString& Cookie
			);
	
	
	/** Increment/decrement an integer by a given amount */
	UFUNCTION(BlueprintCallable, Category = "RedisClient|Context", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContext"))
		void Incr(
			UObject* WorldContext, struct FLatentActionInfo LatentInfo,
			const FString& key, int64& result,int64 by = 1);

	/** Increment/decrement an integer by a given amount (Event version) */
	UFUNCTION(BlueprintCallable, Category = "RedisClient|Context", meta = (DisplayName="Incr (Event)", AutoCreateRefTerm = "Completed"))
		void IncrEvent(
			const FAsyncResultInt64& Completed,
			const FString& Cookie,
			const FString& key, int64 by = 1);

	/** Increment/decrement an floating point value by a given amount */
	UFUNCTION(BlueprintCallable, Category = "RedisClient|Context", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContext"))
		void IncrFloat(
			UObject* WorldContext, struct FLatentActionInfo LatentInfo,
			const FString& key, float& result, float by = 1);

	/** Increment/decrement an floating point value by a given amount (Event version) */
	UFUNCTION(BlueprintCallable, Category = "RedisClient|Context", meta = (DisplayName = "IncrFloat (Event)", AutoCreateRefTerm = "Completed"))
		void IncrFloatEvent(
			const FAsyncResultFloat& Completed,
			const FString& Cookie,
			const FString& key, float by = 1.0);

	/** Event to handle unexpected disconnects */
	UPROPERTY(BlueprintAssignable, EditAnywhere)
	FAsyncContextDisconnect OnDisconnected;


	// PUB SUB support

	/** Subscribe to a list of channel patterns */
	UFUNCTION(BlueprintCallable, Category = "RedisClient|Context|PubSub", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContext"))
	void PSubscribe(
		UObject* WorldContext, struct FLatentActionInfo LatentInfo,
		const TArray<FString>& Patterns);

	UFUNCTION(BlueprintCallable, Category = "RedisClient|Context|PubSub", meta = (DisplayName = "PSubscribe (Event)", AutoCreateRefTerm = "Completed"))
	void PSubscribeEvent(const TArray<FString>& Patterns,
		const FAsyncResultNone& Completed);
		

	/** Unsubscribe from a list of channel patterns */
	UFUNCTION(BlueprintCallable, Category = "RedisClient|Context|PubSub", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContext"))
	void PUnsubscribe(
		UObject* WorldContext, struct FLatentActionInfo LatentInfo,
		const TArray<FString>& Patterns);

	UFUNCTION(BlueprintCallable, Category = "RedisClient|Context|PubSub", meta = (DisplayName = "PUnsubscribe (Event)", AutoCreateRefTerm = "Completed"))
	void PUnsubscribeEvent(const TArray<FString>& Patterns,
		const FAsyncResultNone& Completed);


	/** Get set of subscribed channel patterns */
	UFUNCTION(BlueprintPure, Category = "RedisClient|Context|PubSub")
	TSet<FString> GetSubscribedPatterns();

	/** Event to handle delivered published message */
	UPROPERTY(BlueprintAssignable, EditAnywhere)
	FAsyncPublishedMessage OnPubSubReceived;

	/** Event to handle delivered push message */
	UPROPERTY(BlueprintAssignable, EditAnywhere)
	FAsyncPushedMessage OnPushReceived;


// for FRedisAsyncAdapter
	virtual redisAsyncContext* GetAsyncContext()
	{
		return context;
	}

protected:
	virtual redisContext* GetRedisContext() override;	

private:
	// async callback thunkers
	static void _connectCallback(const redisAsyncContext* ctx, int status);
	static void _disconnectCallback(const redisAsyncContext* ctx, int status);
	static void _pubSubCallback(struct redisAsyncContext* ctx, void* _reply, void* privdata);
	static void _pushCallback(struct redisAsyncContext* ctx, void*);

		// async callbacks
	void connectCallback(int status);
	void disconnectCallback(int status);
	void pubSubCallback(struct redisAsyncContext* ctx, const redisReply* reply);
	void pushCallback(struct redisAsyncContext* ctx, const redisReply* reply);

private:
	redisAsyncContext* context = nullptr;
	// Need a simple state machine to know when we can expect callbacks
	// from the api, and what api methods are valid.
	bool connecting = false;
	bool disconnecting = false;
	bool connected = false;
	FRedisError ConnectError;
	TSet<FString> SubscribedPatterns;

	// naked pointer to the connection response.  It owned either by itself, or the
	// latent action manager.  It is cleared after calling it.
	class FConnectResponse* ConnectResponse = nullptr;
	// Same for disconnects
	class FConnectResponse* DisconnectResponse = nullptr;

	// a queue of events for notification callbacks when subsribe/unsubscribe is done.
	TArray<void*> PubSubNotify;
};
