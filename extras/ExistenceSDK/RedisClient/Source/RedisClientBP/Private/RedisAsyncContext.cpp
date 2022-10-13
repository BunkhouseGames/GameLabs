
#include "RedisAsyncContext.h"
#include "RedisClientBPModule.h"

#include "LatentActions.h"

#include "hiredis/async.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
#include <winsock2.h> /* For struct timeval */
#include "Windows/PostWindowsApi.h" 
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#include "Templates/UniquePtr.h"


///////////////////////////////////////////////////////////////
// FRedisAsyncAdapter methods


void FRedisAsyncAdapter::Bind()
{
	auto context = GetAsyncContext();
	// and the ev read callbacks
	context->ev.addRead = addRead;
	context->ev.delRead = delRead;
	context->ev.addWrite = addWrite;
	context->ev.delWrite = delWrite;
	context->ev.scheduleTimer = scheduleTimer;
	context->ev.data = (void*)this;
}

void FRedisAsyncAdapter::Tick(float DeltaTime)
{
	if (!want_read && !want_write)
	{
		return;
	}
	auto context = GetAsyncContext();
	if (!context || context->err != 0)
	{
		return;
	}
	int fd = context->c.fd;
	fd_set sr, sw, se;
	if (want_read)
	{
		FD_ZERO(&sr);
		FD_SET(fd, &sr);
	}
	if (want_write)
	{
		// on Windows, connection failure is indicated with the Exception fdset.
		// handle it the same as writable.
		FD_ZERO(&sw);
		FD_SET(fd, &sw);
		FD_ZERO(&se);
		FD_SET(fd, &se);
	}
	timeval timeout = { 0 };
	int ns = select(fd + 1, want_read ? &sr : nullptr, want_write ? &sw : nullptr, want_write ? &se : nullptr, &timeout);
	if (ns)
	{
		// check these before doing any callbacks
		bool handleRead = want_read && FD_ISSET(fd, &sr);
		bool handleWrite = want_write && (FD_ISSET(fd, &sw) || FD_ISSET(fd, &se));
		if (handleRead)
		{
			redisAsyncHandleRead(context);
		}
		if (handleWrite)
		{
			/* context may have gone away during some disconnect callback from HandleRead */
			if (handleRead)
			{
				context = GetAsyncContext();
				if (!context)
				{
					return;
				}
			}
			redisAsyncHandleWrite(context);
		}
	}
	checkTimeout(context);
}

bool FRedisAsyncAdapter::IsTickable() const
{
	// quick avoid ticking if not asking for ticks
	return want_read || want_write;
}

TStatId FRedisAsyncAdapter::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FRedisAsyncAdapter, STATGROUP_Tickables);
}


void FRedisAsyncAdapter::addRead(void* privdata)
{
	auto self = static_cast<FRedisAsyncAdapter*>(privdata);
	self->want_read = true;
}
void FRedisAsyncAdapter::delRead(void* privdata)
{
	auto self = static_cast<FRedisAsyncAdapter*>(privdata);
	self->want_read = false;
}

void FRedisAsyncAdapter::addWrite(void* privdata)
{
	auto self = static_cast<FRedisAsyncAdapter*>(privdata);
	self->want_write = true;
}
void FRedisAsyncAdapter::delWrite(void* privdata)
{
	auto self = static_cast<FRedisAsyncAdapter*>(privdata);
	self->want_write = false; 
}

void FRedisAsyncAdapter::scheduleTimer(void* privdata, struct timeval tv)
{
	auto self = static_cast<FRedisAsyncAdapter*>(privdata);
	double now = FPlatformTime::Seconds();
	self->deadline = now + (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
	self->have_timeout = true;
}

void FRedisAsyncAdapter::checkTimeout(redisAsyncContext *context)
{
	if (!have_timeout)
	{
		return;
	}
	double now = FPlatformTime::Seconds();
	if (now >= deadline) {
		have_timeout = false;
		redisAsyncHandleTimeout(context);
	}
}

/////////////////////////////////////////////////////////////////////////
// Response helpers.
// Objects that help us trigger events or latent functions when redis
// callbacks complete



// BaseResponse.  This can be used both for regular Async function calls
// and latent functions.  It is intended to be used as a callback from 
// the asyncContext, and either owns its own pointer or not.
class FBaseResponse :  public FPendingLatentAction
{
public:
	FBaseResponse() = default;
	FBaseResponse(const struct FLatentActionInfo& LatentInfo) :
		is_latent(true),
		ExecutionFunction(LatentInfo.ExecutionFunction),
		OutputLink(LatentInfo.Linkage),
		CallbackTarget(LatentInfo.CallbackTarget)
	{}

	virtual ~FBaseResponse() {
	};

	bool IsLatent() const { return is_latent; }
	
	redisCallbackFn* GetCallback() { return _redisCallback; }
	void* GetPrivdata() { return static_cast<void*>(this); }
	
	// Trigger this on failure
	void Fail(URedisContext* ctxt) {
		redisCallbackBase(ctxt, nullptr);
	}

	template<class T>
	void TakePosession(TUniquePtr<T>& old)
	{
		keepalive = MoveTemp(old);
	}
	
protected:
	static void _redisCallback(struct redisAsyncContext* ctx, void* _reply, void* privdata)
	{
		FBaseResponse* self = static_cast<FBaseResponse*>(privdata);
		URedisContext* uctxt = static_cast<URedisContext*>(ctx->data);
		redisReply* reply = static_cast<redisReply*>(_reply);
		self->redisCallbackBase(uctxt, reply);
	}
public:
	// this can be invoked to _emulate_ callbacks from redis
	void redisCallbackBase(URedisContext *uctxt, const redisReply* reply)
	{
		redisCallback(uctxt, reply);
		LatentReady();
		keepalive.Reset();
	}
protected:
	virtual void redisCallback(URedisContext* uctx, const redisReply* reply) = 0;
	void LatentReady() { Ready = true; }

	// Latent tick function.
	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		Response.FinishAndTriggerIf(Ready, ExecutionFunction, OutputLink, CallbackTarget);
	}

protected:
	// for latent functions
	bool is_latent = false;
	bool Ready = false;
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;

protected:
	// When used for async callbacks, the structure owns its own reference and releases it after
	// the call to the delegate.  When used for latent callbacks, it is a nullptr
	TUniquePtr<FBaseResponse> keepalive;
};


// Special case of this which takes the connect/disconnect signatures.
// Those callbacks are not queued in the normal sense but exists as properties on
// the context
class FConnectResponse: public FBaseResponse
{
public:
	FConnectResponse(const FAsyncResultSuccess& _Event) :
		Event(_Event)
	{}
	FConnectResponse(const FAsyncResultNone& _Event) :
		NoneEvent(_Event)
	{}

	FConnectResponse(const struct FLatentActionInfo& LatentInfo, ERedisSuccess* _Success) :
		FBaseResponse(LatentInfo),
		Success(_Success)
	{}


	// only triggered manually via Fail()
	virtual void redisCallback(URedisContext* uctxt, const redisReply* reply)
	{
		check(reply == nullptr);
		connectCallback(REDIS_ERR);
	}

	void connectCallback(int status)
	{
		if (!IsLatent()) {
			Event.ExecuteIfBound(status != REDIS_ERR);
			NoneEvent.ExecuteIfBound();
		}
		else
		{
			if (Success)
			{
				*Success = (status != REDIS_ERR) ? ERedisSuccess::Success : ERedisSuccess::Failure;
			}
			LatentReady();
		}
		keepalive.Reset();
	}

	FAsyncResultGeneric callback;

private:
	FAsyncResultNone NoneEvent;
	FAsyncResultSuccess Event;
	ERedisSuccess* Success = nullptr;
};

template <class DelegateType, class ValueType>
class FSimpleResponse : public FBaseResponse
{
public:
	FSimpleResponse(const DelegateType& _Delegate, const FString &cookie):
		Delegate(_Delegate),
		Cookie(cookie)
	{}

	FSimpleResponse(const struct FLatentActionInfo& LatentInfo, ValueType &ValueRef) :
		FBaseResponse(LatentInfo),
		ValuePtr(&ValueRef)
	{}

	virtual void redisCallback(URedisContext* uctxt, const redisReply* reply)
	{
		if (IsLatent()) {
			if (reply)
			{
				*ValuePtr = FRedisReplyWrap(reply);
			}
		}
		else {
			if (reply)
			{
				Delegate.ExecuteIfBound(Cookie, FRedisReplyWrap(reply));
			}
			else
			{
				Delegate.ExecuteIfBound(Cookie, ValueType());
			}
		}
	}

private:
	ValueType* ValuePtr = nullptr;
	DelegateType Delegate;
	FString Cookie;
};


// Response for the "Command" style, returning a Reply and an array of Replies
class FGenericResponse : public FBaseResponse
{
public:

	FGenericResponse(const FAsyncResultGeneric& _cb, const FString &_cookie) :
		callback(_cb),
		cookie(_cookie)
	{}

	FGenericResponse(const struct FLatentActionInfo& LatentInfo, ERedisSuccess* _Success, FRedisReply &_Reply, TArray<FRedisReply> &_Elements) :
		FBaseResponse(LatentInfo),
		Success(_Success), 
		Reply(&_Reply),
		Elements(&_Elements)
	{}

private:
	virtual void redisCallback(URedisContext* uctxt, const redisReply* reply)
	{
		if (IsLatent()) {
			if (reply)
			{
				Reply->Init(reply);
				*Elements = FRedisReply::InitArray(reply);
			}
			if (Success)
			{
				*Success = (reply != nullptr) ? ERedisSuccess::Success : ERedisSuccess::Failure;
			}
		}
		else {
			bool success;
			FRedisReply element;
			TArray<FRedisReply> elements;
			if (reply)
			{
				element.Init(reply);
				elements = FRedisReply::InitArray(reply);
				success = true;
			}
			else
			{
				success = false;
			}
			callback.ExecuteIfBound(cookie, success, element, elements);
		}
	}

	ERedisSuccess* Success = nullptr;
	FAsyncResultGeneric callback;
	FString cookie;
	FRedisReply* Reply;
	TArray<FRedisReply> *Elements;
};

// Response for the "None" events.  just trigger the event, or launch the latent action
class FNoneResponse : public FBaseResponse
{
public:
	using FBaseResponse::FBaseResponse;

	FNoneResponse(const FAsyncResultNone& _Event) :
		Event(_Event)
	{}

private:
	virtual void redisCallback(URedisContext* uctxt, const redisReply* reply)
	{
		if (!IsLatent()) {
		Event.ExecuteIfBound();
	}
}

	FAsyncResultNone Event;
};


/////////////////////////////////////////////////////////////////////////
// Our Async context

URedisContext::URedisContext()
{}

URedisContext::~URedisContext()
{
	if (context)
	{
		redisAsyncFree(context);
	}
	context = nullptr;
	// trigger all the pubsub callbacks
	for (void *p : PubSubNotify)
	{
		auto action = static_cast<FNoneResponse*>(p);
		action->redisCallbackBase(this, nullptr);
	}
}


// Connecting ******************

bool URedisContext::IsConnected() const
{
	return context != nullptr && connected;
}


void URedisContext::Connect(UObject* WorldContextObject, FLatentActionInfo LatentInfo,
	ERedisSuccess& Exec,
	const FString& HostName, const int Port, float Timeout
){
	Exec = ERedisSuccess::Failure;
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FConnectResponse>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr) {
			auto action = MakeUnique<FConnectResponse>(LatentInfo, &Exec);
			bool ok = ConnectCommon(Exec, HostName, Port, Timeout);
			if (ok) {
				ConnectResponse = action.Get();
				// wait for callback from redis
			}
			else
			{
				// signal failure immediately
				action->Fail(this);
			}
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, action.Release());
		}
		else
		{
			LogLatentInProgress(TEXT("Connect"));
		}
	}
	else
	{
		LogLatentNoWorld(TEXT("Connect"));
	}
}


void URedisContext::ConnectEvent(
	const FAsyncResultSuccess& Completed,
	const FString& HostName, const int Port, float Timeout
)
{
	auto ptr = MakeUnique<FConnectResponse>(Completed);
	ERedisSuccess dummy;
	if (ConnectCommon(dummy, HostName, Port, Timeout))
	{
		// create a response that owns its own lifetime
		ConnectResponse = ptr.Get();
		ptr->TakePosession(ptr);
	}
	else
	{
		ptr->Fail(this);
	}
}


bool URedisContext::ConnectCommon(
	ERedisSuccess& Exec,
	const FString& HostName, const int Port, float Timeout
)
{
	Exec = ERedisSuccess::Failure;
	
	bool can_connect = !context && !connecting;
	
	// we require the user to disconnect before trying to reconnect.
	// we could theoretically automatically delete an existing connection but
	// that is not quite explicit enough.
	// We also don't allow more than one connection attempt at one time, hence
	// the check for "connecting" above.
	if (!can_connect)
		return false;  // can't reconnect to an existing connection

	// ok, go ahead
	check(!connecting && !connected);
	
	redisOptions options = { 0 }; 
	auto AnsiHostName = StringCast<ANSICHAR>(*HostName);
	REDIS_OPTIONS_SET_TCP(&options, AnsiHostName.Get(), Port);

	struct timeval connect_timeout;
	if (Timeout >= 0.0f)
	{
		struct timeval s_timeout = { (int)Timeout, (int)(1e6f * (Timeout - (int)Timeout))};
		connect_timeout = s_timeout;
		options.connect_timeout = &connect_timeout;
	}
	UE_LOG(LogRedisClientBP, Log, TEXT("AsyncConnect: Connecting to %s:%d, timeout %f"), *HostName, Port, Timeout);
	connecting = true;
	Error.Clear();
	context = redisAsyncConnectWithOptions(&options);
	if (!context)
	{
		connecting = false;
		SetError(ERedisErr::ERR_OOM, TEXT("No Memory"));
		LogError(TEXT("AsyncConnect Failed:"));
	}
	else if (context->err)
	{ 
		SetError();
		LogError(TEXT("AsyncConnect Failed:"));
		redisAsyncFree(context);
		context = nullptr;
		connecting = false;
	}
	else
	{
		UE_LOG(LogRedisClientBP, Log, TEXT("AsyncConnect: Connect in progress"));
	}
	if (context)
	{
		// Bind async ticking
		Bind();

		// Hook up the callbacks
		context->data = this;
		redisAsyncSetConnectCallback(context, _connectCallback);
		redisAsyncSetDisconnectCallback(context, _disconnectCallback);
		redisAsyncSetPushCallback(context, _pushCallback);
		
		Exec = ERedisSuccess::Success;
		return true;
	}
	else
	{
		return false;
	}
}

// disconnecting.  We don't do much error checking but provide latent funtions so that at
// least callbacks and other things finish before we return
void URedisContext::Disconnect(
	UObject* WorldContextObject, FLatentActionInfo LatentInfo, bool Hard
)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FConnectResponse>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
		{
			auto action = MakeUnique<FConnectResponse>(LatentInfo, nullptr);
			// logic to figure out if we will even get a callback.
			// sadly, hiredis won´t tell us

			bool can_disconnect = context && connected && !disconnecting;
			if (can_disconnect) {
				// wait for callback from redis
				check(DisconnectResponse == nullptr);
				DisconnectResponse = action.Get();  // store a borrowed ref here.
				disconnecting = true;
				if (!Hard)
				{
					redisAsyncDisconnect(context);
				}
				else
				{
					// callbacks may run here, with error response, so we set the pointer
					// to null only on return
					redisAsyncFree(context);
					context = nullptr;
				}
			}
			else
			{
				// signal failure immediately
				action->connectCallback(REDIS_ERR);
			}
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, action.Release());
		}
		else
		{
			LogLatentInProgress(TEXT("Disconnect"));
		}
	}
	else
	{
		LogLatentNoWorld(TEXT("Disconnect"));
	}
}


void URedisContext::DisconnectEvent(
	const FAsyncResultNone &event,
	bool Hard
)
{
	// This will invoke the DisconnectCallback
	// and will then free the context.  We let the disconnect
	// callback clear the context pointer, so that a new connection can be
	// attempted from the disconnect callback.
	bool can_disconnect = context && connected && !disconnecting;
	if (can_disconnect)
	{
		auto ptr = MakeUnique<FConnectResponse>(event);
		check(DisconnectResponse == nullptr);
		DisconnectResponse = ptr.Get();
		ptr->TakePosession(ptr);
		disconnecting = true;
		if (!Hard)
		{
			redisAsyncDisconnect(context);
		}
		else
		{
			redisAsyncFree(context);
			context = nullptr;
		}
	}
	else
	{
		event.ExecuteIfBound();
	}

}


// Pub sub support.

void URedisContext::PSubscribe(
	UObject* WorldContextObject, struct FLatentActionInfo LatentInfo,
	const TArray<FString>& Patterns)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FGenericResponse>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr) {
			auto action = MakeUnique<FNoneResponse>(LatentInfo);
			int success = REDIS_ERR;
			if (Check_Context(TEXT("PSubscribe")))
			{
				RedisCommandArgs args(TEXT("PSUBSCRIBE"), Patterns, TArray<FRedisBinary>());
				success = redisAsyncCommandArgv(context, _pubSubCallback, static_cast<void*>(this),
					args.argc(), args.argv(), args.argvlen());
			}
			if (success != REDIS_OK)
			{
				action->Fail(this);
			}
			// hand off a borrowed pointer to the callback into a queue
			PubSubNotify.Emplace(static_cast<void*>(action.Get()));
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, action.Release());
		}
		else
		{
			LogLatentInProgress(TEXT("PSubscribe"));
		}
	} 
	else
	{
		LogLatentNoWorld(TEXT("PSubscribe"));
	}
}

void URedisContext::PSubscribeEvent(const TArray<FString>& Patterns,
	const FAsyncResultNone& Completed)
{
	TUniquePtr < FNoneResponse > action;
	bool isBound = Completed.IsBound();
	if (Completed.IsBound())
	{
		action = MakeUnique<FNoneResponse>(Completed);
	}
	if (Check_Context(TEXT("PSubsctibe (Event)")))
	{
		RedisCommandArgs args(TEXT("PSUBSCRIBE"), Patterns, TArray<FRedisBinary>());
		int success = redisAsyncCommandArgv(context, _pubSubCallback, static_cast<void*>(this),
			args.argc(), args.argv(), args.argvlen());
		if (success == REDIS_OK)
		{
			// hand off a borrowed pointer to the callback into a queue, even if it is null.
			PubSubNotify.Emplace(static_cast<void*>(action.Get()));
			if (action.IsValid())
			{
				action->TakePosession(action);
			}
		}
	}
	// if it is still valid, somethign failed, and we should let know.
	if (action.IsValid())
	{
		action->Fail(this);
	}
}


void URedisContext::PUnsubscribe(
	UObject* WorldContextObject, struct FLatentActionInfo LatentInfo,
	const TArray<FString>& Patterns)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FGenericResponse>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr) {

			auto action = MakeUnique<FNoneResponse>(LatentInfo);
			int success = REDIS_ERR;
			if (Check_Context(TEXT("PSubscribe")))
			{
				RedisCommandArgs args(TEXT("PUNSUBSCRIBE"), Patterns, TArray<FRedisBinary>());
				success = redisAsyncCommandArgv(context, _pubSubCallback, static_cast<void*>(this),
					args.argc(), args.argv(), args.argvlen());
			}
			if (success != REDIS_OK)
			{
				action->Fail(this);
			}
			// hand off a borrowed pointer to the callback into a queue
			PubSubNotify.Emplace(static_cast<void*>(action.Get()));
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, action.Release());
		}
		else
		{
			LogLatentInProgress(TEXT("PUnsubscribe"));
		}
	}
	else
	{
		LogLatentNoWorld(TEXT("PUnsubscribe"));
	}
}

void URedisContext::PUnsubscribeEvent(const TArray<FString>& Patterns,
	const FAsyncResultNone& Completed)
{
	TUniquePtr < FNoneResponse > action;
	bool isBound = Completed.IsBound();
	if (Completed.IsBound())
	{
		action = MakeUnique<FNoneResponse>(Completed);
	}
	if (Check_Context(TEXT("PUnsubsctibe (Event)")))
	{
		RedisCommandArgs args(TEXT("PUNSUBSCRIBE"), Patterns, TArray<FRedisBinary>());
		int success = redisAsyncCommandArgv(context, _pubSubCallback, static_cast<void*>(this),
			args.argc(), args.argv(), args.argvlen());
		if (success == REDIS_OK)
		{
			// hand off a borrowed pointer to the callback into a queue, even if it is null.
			PubSubNotify.Emplace(static_cast<void*>(action.Get()));
			if (action.IsValid())
			{
				action->TakePosession(action);
			}
		}
	}
	// if it is still valid, somethign failed, and we should let know.
	if (action.IsValid())
	{
		action->Fail(this);
	}
}


void URedisContext::Command(UObject* WorldContextObject, struct FLatentActionInfo LatentInfo, const FString& cmd, const TArray<FString>& Args, const TArray<FRedisBinary>& BinaryArgs, FRedisReply& Reply, TArray<FRedisReply>& Elements, ERedisSuccess &Exec)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FGenericResponse>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr) {
			auto action = MakeUnique<FGenericResponse>(LatentInfo, &Exec, Reply, Elements);
			int success = REDIS_ERR;
			if (Check_Context(TEXT("Command")))
			{
				RedisCommandArgs args(cmd, Args, BinaryArgs);
				success = redisAsyncCommandArgv(context, action->GetCallback(), action->GetPrivdata(),
					args.argc(), args.argv(), args.argvlen());
			}
			if (success != REDIS_OK)
			{
				action->Fail(this);
			}
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, action.Release());
		}
		else
		{
			LogLatentInProgress(TEXT("Command"));			
		}
	}
	else
	{
		LogLatentNoWorld(TEXT("Command"));
	}
}


void URedisContext::CommandEvent(const FString& cmd, const TArray<FString>& Args, const TArray<FRedisBinary>& BinaryArgs, const FAsyncResultGeneric& complete, const FString &cookie)
{
	TUniquePtr<FGenericResponse> response;
	if (complete.IsBound())
	{
		response = MakeUnique<FGenericResponse>(complete, cookie);
	}
	int success = REDIS_ERR;
	if (Check_Context(TEXT("Command (Event)")))
	{
		RedisCommandArgs args(cmd, Args, BinaryArgs);
		if (response.IsValid())
		{
			success = redisAsyncCommandArgv(context, response->GetCallback(), response->GetPrivdata(),
				args.argc(), args.argv(), args.argvlen());
		}
		else
		{
			success = redisAsyncCommandArgv(context, nullptr, nullptr,
				args.argc(), args.argv(), args.argvlen());
		}
	}
	if (response.IsValid())
	{
		if (success == REDIS_OK)
		{
			response->TakePosession(response);
		}
		else
		{
			response->Fail(this);
		}
	}
}


// INCR helpers
// Increment/decrement an integer by a given amount
void URedisContext::Incr(
	UObject* WorldContext, struct FLatentActionInfo LatentInfo,
	const FString& key, int64& result, int64 by)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FGenericResponse>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr) {
			auto action = MakeUnique<FSimpleResponse<FAsyncResultInt64, int64> >(LatentInfo, result);
			int success = REDIS_ERR;
			if (Check_Context(TEXT("Incr")))
			{
				RedisCommandArgs cmd;
				if (by == 1)
				{
					cmd.Emplace("INCR");
					cmd.EmplaceUTF8(key);
				}
				else {
					cmd.Emplace("INCRBY");
					cmd.EmplaceUTF8(key);
					cmd.EmplaceAnsi(FString::Printf(TEXT("%lld"), by));
				}
				success = redisAsyncCommandArgv(context, action->GetCallback(), action->GetPrivdata(),
					cmd.argc(), cmd.argv(), cmd.argvlen());
			}
			if (success == REDIS_ERR)
			{
				action->Fail(this);
			}
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, action.Release());
		}
		else
		{
			LogLatentInProgress(TEXT("Incr"));
		}
	}
	else
	{
		LogLatentNoWorld(TEXT("Incr"));
	}
}


// Increment/decrement an integer by a given amount (Event version)
void URedisContext::IncrEvent(
	const FAsyncResultInt64& Completed,
	const FString &cookie,
	const FString& key, int64 by)
{
	TUniquePtr < FSimpleResponse<FAsyncResultInt64, int64> > action;
	bool isBound = Completed.IsBound();
	if (Completed.IsBound())
	{
		action = MakeUnique<FSimpleResponse<FAsyncResultInt64, int64> >(Completed, cookie);
	}
	int success = REDIS_ERR;
	if (Check_Context(TEXT("Incr (Event)")))
	{
		RedisCommandArgs cmd;
		if (by == 1)
		{
			cmd.Emplace("INCR");
			cmd.EmplaceUTF8(key);
		}
		else {
			cmd.Emplace("INCRBY");
			cmd.EmplaceUTF8(key);
			cmd.EmplaceAnsi(FString::Printf(TEXT("%lld"), by));
		}
		if (action.IsValid())
		{
			success = redisAsyncCommandArgv(context, action->GetCallback(), action->GetPrivdata(),
				cmd.argc(), cmd.argv(), cmd.argvlen());
		}
		else
		{
			success = redisAsyncCommandArgv(context, nullptr, nullptr,
				cmd.argc(), cmd.argv(), cmd.argvlen());
		}
	}
	if (action.IsValid())
	{
		if (success == REDIS_OK)
		{
			action->TakePosession(action);
		}
		else
		{
			action->Fail(this);
		}
	}
}


void URedisContext::IncrFloat(
	UObject* WorldContext, struct FLatentActionInfo LatentInfo,
	const FString& key, float& result, float by)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FGenericResponse>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr) {
			auto action = MakeUnique<FSimpleResponse<FAsyncResultFloat, float> >(LatentInfo, result);
			int success = REDIS_ERR;
			if (Check_Context(TEXT("IncrFloat")))
			{
				RedisCommandArgs cmd("INCRBYFLOAT", key);
				cmd.EmplaceAnsi(FString::Printf(TEXT("%f"), by));
				success = redisAsyncCommandArgv(context, action->GetCallback(), action->GetPrivdata(),
					cmd.argc(), cmd.argv(), cmd.argvlen());
			}
			if (success == REDIS_ERR)
			{
				action->Fail(this);
			}
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, action.Release());
		}
		else
		{
			LogLatentInProgress(TEXT("IncrFloat"));
		}
	}
	else
	{
		LogLatentNoWorld(TEXT("IncrFloat"));
	}
}


void URedisContext::IncrFloatEvent(
	const FAsyncResultFloat& Completed,
	const FString &Cookie,
	const FString& key, float by)
{
	TUniquePtr<FSimpleResponse<FAsyncResultFloat, float> > action;
	if (Completed.IsBound())
	{
		action = MakeUnique<FSimpleResponse<FAsyncResultFloat, float> >(Completed, Cookie);
	}
	int success = REDIS_ERR;
	if (Check_Context(TEXT("IncrFloat (Event)")))
	{
		RedisCommandArgs cmd("INCRBYFLOAT", key);
		cmd.EmplaceAnsi(FString::Printf(TEXT("%f"), by));
		if (action.IsValid())
		{
			success = redisAsyncCommandArgv(context, action->GetCallback(), action->GetPrivdata(),
				cmd.argc(), cmd.argv(), cmd.argvlen());
		}
		else
		{
			success = redisAsyncCommandArgv(context, nullptr, nullptr,
				cmd.argc(), cmd.argv(), cmd.argvlen());
		}
	}
	if (action.IsValid())
	{
		if (success == REDIS_OK)
		{
			action->TakePosession(action);
		}
		else
		{
			action->Fail(this);
		}
	}
}


// PUB/SUB


TSet<FString> URedisContext::GetSubscribedPatterns()
{
	return SubscribedPatterns;
}



// Redis async callback thunkers
void URedisContext::_connectCallback(const redisAsyncContext* ctx, int status)
{
	URedisContext* self = static_cast<URedisContext*>(ctx->data);
	self->connectCallback(status);
}
void URedisContext::_disconnectCallback(const redisAsyncContext* ctx, int status)
{
	URedisContext* self = static_cast<URedisContext*>(ctx->data);
	self->disconnectCallback(status);
}

void URedisContext::_pubSubCallback(struct redisAsyncContext* ctx, void* _reply, void* privdata)
{
	URedisContext* self = static_cast<URedisContext*>(ctx->data);
	self->pubSubCallback(ctx, static_cast<const redisReply*>(_reply));
}

void URedisContext::_pushCallback(struct redisAsyncContext* ctx, void* _reply)
{
	URedisContext* self = static_cast<URedisContext*>(ctx->data);
	auto reply = static_cast<redisReply*>(_reply);
	self->pushCallback(ctx, reply);
}

// Redis callbacks
void URedisContext::connectCallback(int status)
{
	if (status == REDIS_ERR)
	{
		SetError();
		LogError(TEXT("AsyncConnect Failed:"));
	}
	else
	{
		UE_LOG(LogRedisClientBP, Log, TEXT("AsyncConnect: Connected!"));
	}

	check(context);
	check(!connected);
	check(connecting);
	check(!disconnecting);
	connecting = false;
	if (status == REDIS_OK) {
		connected = true;
	}
	else
	{
		// context will be automatically freed after this callback.  Clear the context pointer
		// to allow for a reconnect from the callback.
		context = nullptr;
	}
	if (ConnectResponse != nullptr)
	{
		// Call our borrowed reference to the ConnectResponse and clear it.
		auto* tmp = ConnectResponse;
		ConnectResponse = nullptr;
		tmp->connectCallback(status);
	}
}


// disconnect.  The context is cleared after this call, so, we need
// to clear the context pointer in out UContext before calling the callback,
// so that the callbak may attempt to re-connect.
void URedisContext::disconnectCallback(int status)
{
	FRedisError error;
	if (status == REDIS_ERR)
	{
		SetError();
		LogError(TEXT("Disconnected unexpectedly"));
		error = GetError();
	}
	else
	{
		UE_LOG(LogRedisClientBP, Log, TEXT("Disconnected gracefully"));
	}
	// we should never get this while attempting to connect
	check(!connecting);
	check(context);
	// context will be automatically freed after this, clear it now so that
	// a reconnect can be attempted.
	context = nullptr;
	connected = false;
	connecting = false;
	disconnecting = false;
	SubscribedPatterns.Empty();
	if (DisconnectResponse != nullptr)
	{
		// A latent or event disconnect is in progress
		// We signal disconnect to the calling latent function or event
		auto* tmp = DisconnectResponse;
		DisconnectResponse = nullptr;
		tmp->connectCallback(status);
	}
	else
	{
		// invoke the unexpected disconnect callback
		OnDisconnected.Broadcast(this, status == REDIS_OK, error);
	}
}


// Pub-sub message was received!
void URedisContext::pubSubCallback(struct redisAsyncContext* ctx, const redisReply* reply)
{
	
	bool need_action = false;
	if (reply)
	{
		const int i = reply->elements;
		// We assume that the hiredis library will monitor the protocol for us
		if (i < 3)
		{
			return; // something weird
		}
		FString type = FRedisReplyWrap(reply->element[0]).Ansi();
		FString pattern = FRedisReplyWrap(reply->element[1]).UTF8();
		if (type == TEXT("psubscribe"))
		{
			SubscribedPatterns.Add(pattern);
			need_action = true;
		}
		else if (type == TEXT("punsubscribe"))
		{
			SubscribedPatterns.Remove(pattern);
			need_action = true;
		}
		else if (type == TEXT("pmessage"))
		{
			if (i >= 4) {
				FString channel = FRedisReplyWrap(reply->element[2]).UTF8();
				FRedisBinary data = FRedisReplyWrap(reply->element[3]).Binary();
				OnPubSubReceived.Broadcast(pattern, channel, data);
			}
		}
	}
	else
	{
		// failed subsctibe/unsubscribe event, need to notify
		need_action = true;
	}

	if (need_action)
	{
		// pop the Action pointer of our queue
		if (PubSubNotify.Num())
		{
			auto action = static_cast<FNoneResponse*>(PubSubNotify[0]);
			PubSubNotify.RemoveAt(0);
			if (action)
			{
				action->redisCallbackBase(this, reply);
			}
		}
	}
}


void URedisContext::pushCallback(struct redisAsyncContext* ctx, const redisReply* reply)
{
	FRedisReply element;
	TArray<FRedisReply> elements;
	element.Init(reply);
	elements = FRedisReply::InitArray(reply);
	OnPushReceived.Broadcast(element, elements);
}


redisContext* URedisContext::GetRedisContext()
{
	if (!context)
	{
		return nullptr;
	}
	return &context->c;
}

