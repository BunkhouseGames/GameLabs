#include "KamoState.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#if WITH_AUTOMATION_TESTS
#define CHECKTYPE(T,C) bool Check##T(C Value) \
{ \
	UKamoState* KS = NewObject<UKamoState>(); \
	C Result; \
	KS->Set##T("Test", Value); \
	bool Exists = KS->Get##T("Test", Result); \
	if (!Exists) \
	{ \
		UE_LOG(LogEditorAutomationTests, Error, TEXT("Expected key did not exist")); \
		return false; \
	} \
	if(Result != Value) \
	{ \
		UE_LOG(LogEditorAutomationTests, Error, TEXT("Value retrieved did not match value set")); \
		return false; \
	} \
	return true; \
}

#define CHECKTYPENOEXISTS(T,C) bool Check##T(C Value) \
{ \
	UKamoState* KS = NewObject<UKamoState>(); \
	C Result; \
	KS->Set##T("Test", Value); \
	KS->Get##T("Test", Result); \
	if(Result != Value) \
	{ \
		UE_LOG(LogEditorAutomationTests, Error, TEXT("Value retrieved did not match value set")); \
		return false; \
	} \
	return true; \
}

CHECKTYPE(Bool, bool)
#define CHECKBOOL(X) if (!CheckBool(X)) return false

CHECKTYPE(Int, int)
#define CHECKINT(X) if (!CheckInt(X)) return false

CHECKTYPE(Float, float)
#define CHECKFLOAT(X) if (!CheckFloat(X)) return false

CHECKTYPE(String, FString)
#define CHECKSTRING(X) if (!CheckString(X)) return false

CHECKTYPENOEXISTS(Vector, FVector)
#define CHECKVECTOR(X) if (!CheckVector(X)) return false

CHECKTYPE(Rotator, FRotator)
#define CHECKROTATOR(X) if (!CheckRotator(X)) return false

CHECKTYPENOEXISTS(Quat, FQuat)
#define CHECKQUAT(X) if (!CheckQuat(X)) return false

static const int Flags = EAutomationTestFlags::EditorContext
					   | EAutomationTestFlags::ClientContext
					   | EAutomationTestFlags::EngineFilter;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestKamoStateBool, "Kamo.KamoState.bool", Flags)

bool FTestKamoStateBool::RunTest(const FString& Parameters)
{
	CHECKBOOL(true);
	CHECKBOOL(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestKamoStateInt, "Kamo.KamoState.int", Flags)

bool FTestKamoStateInt::RunTest(const FString& Parameters)
{
	CHECKINT(0);
	CHECKINT(-1);
	CHECKINT(1);
	CHECKINT(42);
	CHECKINT(MAX_int32);
	CHECKINT(-MAX_int32);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestKamoStateFloat, "Kamo.KamoState.float", Flags)

bool FTestKamoStateFloat::RunTest(const FString& Parameters)
{
	CHECKFLOAT(0.0f);
	CHECKFLOAT(-1.0f);
	CHECKFLOAT(1.0f);
	CHECKFLOAT(3.14);
	CHECKFLOAT(MAX_FLT);
	CHECKFLOAT(-MAX_FLT);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestKamoStateString, "Kamo.KamoState.string", Flags)

bool FTestKamoStateString::RunTest(const FString& Parameters)
{
	CHECKSTRING(TEXT(""));
	CHECKSTRING(TEXT("This is a test"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestKamoStateVector, "Kamo.KamoState.vector", Flags)

bool FTestKamoStateVector::RunTest(const FString& Parameters)
{
	CHECKVECTOR(FVector(0.0f, 0.0f, 0.0f));
	CHECKVECTOR(FVector(-1.0f, -1.0f, -1.0f));
	CHECKVECTOR(FVector(MAX_FLT, MAX_FLT, MAX_FLT));
	CHECKVECTOR(FVector(-MAX_FLT, -MAX_FLT, -MAX_FLT));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestKamoStateRotator, "Kamo.KamoState.rotator", Flags)

bool FTestKamoStateRotator::RunTest(const FString& Parameters)
{
	CHECKROTATOR(FRotator(0.0f, 0.0f, 0.0f));
	CHECKROTATOR(FRotator(-1.0f, -1.0f, -1.0f));
	CHECKROTATOR(FRotator(MAX_FLT, MAX_FLT, MAX_FLT));
	CHECKROTATOR(FRotator(-MAX_FLT, -MAX_FLT, -MAX_FLT));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestKamoStateQuat, "Kamo.KamoState.quat", Flags)

bool FTestKamoStateQuat::RunTest(const FString& Parameters)
{
	CHECKQUAT(FQuat(0.0f, 0.0f, 0.0f, 0.0f));
	CHECKQUAT(FQuat(-1.0f, -1.0f, -1.0f, -1.0f));
	CHECKQUAT(FQuat(MAX_FLT, MAX_FLT, MAX_FLT, MAX_FLT));
	CHECKQUAT(FQuat(-MAX_FLT, -MAX_FLT, -MAX_FLT, -MAX_FLT));

	return true;
}
#endif