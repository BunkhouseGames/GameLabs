// TEMP HACK: To enable component reflection we check for this interface.
// Proper solution is to have something similar to the Kamo class map but for components.
#pragma once

#include "IKamoComponentReflection.generated.h"

UINTERFACE()
class UKamoComponentReflection : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class KAMO_API IKamoComponentReflection
{
	GENERATED_BODY()

		// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void UpdateKamoStateFromActor(UKamoState* state);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KamoObject")
	void ApplyKamoStateToActor(UKamoState* state);
};

