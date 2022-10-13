#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "RedisTypes.h"
#include "RedisBlockingContext.h"
#include "RedisAsyncContext.h"
#include "RedisBPLibrary.generated.h"



/*
*	Function library class.
*	Each function in it is expected to be static and represents blueprint node that can be called in any blueprint.
*
*	When declaring function you can define metadata for the node. Key function specifiers will be BlueprintPure and BlueprintCallable.
*	BlueprintPure - means the function does not affect the owning object in any way and thus creates a node without Exec pins.
*	BlueprintCallable - makes a function which can be executed in Blueprints - Thus it has Exec pins.
*	DisplayName - full name of the node, shown when you mouse over the node and in the blueprint drop down menu.
*				Its lets you name the node using characters not allowed in C++ function names.
*	CompactNodeTitle - the word(s) that appear on the node.
*	Keywords -	the list of keywords that helps you to find node when you search for it using Blueprint drop-down menu.
*				Good example is "Print String" node which you can find also by using keyword "log".
*	Category -	the category your node will be under in the Blueprint drop-down menu.
*
*	For more info on custom blueprint nodes visit documentation:
*	https://wiki.unrealengine.com/Custom_Blueprint_Node_Creation
*/


UCLASS()
class REDISCLIENTBP_API URedisBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

#if 0
	// Create A new BlockingContext
	UFUNCTION(BlueprintCallable, Category = "RedisClient", meta = (Keywords = "redis create blocking"))
	static URedisBlockingContext* CreateBlockingContext();
#endif

	// Create a new Redis Context
	UFUNCTION(BlueprintCallable, Category = "RedisClient", meta = (DisplayName="Create Context", Keywords = "redis create async"))
	static URedisContext* CreateAsyncContext();

	// *********************
	// data conversion functions

	// FRedisBinary conversion

	/** Converts UTF8 encoded redis binary to string */
	UFUNCTION(BlueprintPure, Category = "RedisClient|Convert", meta = (Keywords = "utf8 binary decode", BlueprintAutocast))
	static FString FromUTF8(const FRedisBinary& binary);

	/** Encode string as UTF8 encoded Redis Binary */
	UFUNCTION(BlueprintPure, Category = "RedisClient|Convert", meta = (Keywords = "utf8 binary encode"))
	static FRedisBinary ToUTF8(const FString& text);

	/** Convenience, Convert reply array to string array, assuming UTF8 encoding */
	UFUNCTION(BlueprintPure, Category = "RedisClient|Convert", meta = (Keywords = "array"))
	static TArray<FString> ReplyArrayToStrings(const TArray<FRedisReply>& array);

	/** Converts a 64 bit integer to a string */
	UFUNCTION(BlueprintPure, Category = "RedisClient|Convert", meta = (DisplayName = "ToString (Int64)", Keywords = "cast convert", CompactNodeTitle = "->", BlueprintAutocast))
	static FString Int64ToString(int64 integer);

	
	// *********************************
	// FRedisError

	/** Format the error state as string */
	UFUNCTION(BlueprintPure, Category = "RedisClient|Error", meta =(BlueprintAutocast))
	static FString FormatError(const FRedisError& error);

	/** Is there an error state? */
	UFUNCTION(BlueprintPure, Category = "RedisClient|Error", meta = (BlueprintAutocast))
	static bool IsError(const FRedisError& error);
	

	// *********************************
	// FRedisReply helper functions

	/** Format full reply information as as string */
	UFUNCTION(BlueprintPure, Category = "RedisClient|Reply", meta = (DisplayName="Format (Reply)"))
	static FString FormatReply(const FRedisReply& error);

	/** Was the command successful? */
	UFUNCTION(BlueprintPure, Category = "RedisClient|Reply", meta = (Keywords = "success"))
	static bool IsSuccess(const FRedisReply& reply);

	/** Get the reply error message or empty string if command was successful */
	UFUNCTION(BlueprintPure, Category = "RedisClient|Reply", meta = (Keywords = "error", DisplayName="ErrorMessage"))
	static FString ReplyErrorMessage(const FRedisReply& reply);

	/** Does the reply contain an array? */
	UFUNCTION(BlueprintPure, Category = "RedisClient|Reply", meta = (Keywords = "array"))
	static bool IsArray(const FRedisReply& reply);

	/** Return the integer result, or 0 */
	UFUNCTION(BlueprintPure, Category = "RedisClient|Reply", meta = (Keywords = "integer", BlueprintAutocast))
	static int64 AsInteger(const FRedisReply& reply);

	/** If this was a boolean result, return the boolean value */
	UFUNCTION(BlueprintPure, Category = "RedisClient|Reply", meta = (Keywords = "boolean", BlueprintAutocast))
	static bool AsBoolean(const FRedisReply& reply);

	/** Return the floating point result or 0.0 */
	UFUNCTION(BlueprintPure, Category = "RedisClient|Reply", meta = (Keywords = "float", BlueprintAutocast))
	static float AsFloat(const FRedisReply& reply);

	/** Return the string result, from UTF8 encoded data */
	UFUNCTION(BlueprintPure, Category = "RedisClient|Reply", meta = (Keywords = "string", BlueprintAutocast))
	static FString AsString(const FRedisReply& reply);
};


/**
 * A base object providing world access.  Useful to create blueprint event graphs utilizing latent
 * blueprint nodes.
 */
UCLASS(Blueprintable)
class URedisWorldObject : public UObject
{
	GENERATED_BODY()

		/** Getter for the cached world pointer, will return null if the actor is not actually spawned in a level */
		virtual UWorld* GetWorld() const override;

public:
	/** An object to provide the world context, instead of the 'outer' */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		UObject* WorldContextObject;

};