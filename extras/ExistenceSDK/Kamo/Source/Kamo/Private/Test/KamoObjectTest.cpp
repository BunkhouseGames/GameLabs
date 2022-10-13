#include "CoreMinimal.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "KamoActorWithProperties.h"
#include "KamoSettings.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

DECLARE_LOG_CATEGORY_EXTERN(LogKamoObjectTest, Log, All);
DEFINE_LOG_CATEGORY(LogKamoObjectTest);

UWorld* GetTestingWorld()
{
	UWorld* TestWorld = nullptr;
	const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
	for (const FWorldContext& Context : WorldContexts)
	{
		if (((Context.WorldType == EWorldType::Editor) || (Context.WorldType == EWorldType::PIE) || (Context.WorldType == EWorldType::Game)) && (Context.World() != NULL))
		{
			TestWorld = Context.World();
			break;
		}
	}

	// Override mark and sync rate so tests don't have to wait for persistence to happen
	UKamoProjectSettings::Get()->mark_and_sync_rate = 0.0f;

	return TestWorld;
}

#define TESTFLAGS EAutomationTestFlags::EditorContext \
					| EAutomationTestFlags::ClientContext \
					| EAutomationTestFlags::EngineFilter

const float SLEEPTIME = 1.0f;

DEFINE_LATENT_AUTOMATION_COMMAND(FAddKamoObjects);

bool FAddKamoObjects::Update()
{
	UWorld* TestWorld = GetTestingWorld();

	// Create one object with default values
	TestWorld->SpawnActor<AKamoActorWithProperties>(FVector(0.0f, 0.0f, 0.0f), FRotator::ZeroRotator);

	// Create another object where we set some specific values
	AKamoActorWithProperties* CustomActor = TestWorld->SpawnActor<AKamoActorWithProperties>(FVector(0.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	CustomActor->IntProp = -37;
	CustomActor->StringProp = "This has been changed";
	CustomActor->StringMapProp.Add(TEXT("bingo"), TEXT("bongo"));
	CustomActor->StringMapProp.Add(TEXT("foo"), TEXT("bar"));
	CustomActor->StringSetProp.Add(TEXT("bingo"));
	CustomActor->StringSetProp.Add(TEXT("bongo"));
	CustomActor->StringSetProp.Add(TEXT("bingo"));
	CustomActor->StringSetProp.Add(TEXT("bingo"));
	CustomActor->StringSetProp.Add(TEXT("foo"));
	CustomActor->StringSetProp.Add(TEXT("bar"));
	
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND(FValidateKamoObjects);

bool FValidateKamoObjects::Update()
{
	UWorld* TestWorld = GetTestingWorld();

	if (!TestWorld)
	{
		UE_LOG(LogKamoObjectTest, Error, TEXT("No test world found"));
		return true;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(TestWorld, AKamoActorWithProperties::StaticClass(), FoundActors);

	if (FoundActors.Num() != 2)
	{
		UE_LOG(LogKamoObjectTest, Error, TEXT("Incorrect number of objects in level - expected %d, found %d"), 2, FoundActors.Num());
	} else
	{
		UE_LOG(LogKamoObjectTest, Display, TEXT("Found expected number of objects"));
	}

	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND(FDeleteKamoObjects);

bool FDeleteKamoObjects::Update()
{
	UWorld* TestWorld = GetTestingWorld();

	if (!TestWorld)
	{
		UE_LOG(LogKamoObjectTest, Error, TEXT("No test world found"));
		return true;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(TestWorld, AKamoActorWithProperties::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		Actor->Destroy();
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKamoDbCreateTestObjects, "KamoDbTest.CreateTestObjects", TESTFLAGS)

bool FKamoDbCreateTestObjects::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = GetTestingWorld();
	if (!TestWorld)
	{
		UE_LOG(LogKamoObjectTest, Error, TEXT("No test world found"));
		return true;
	}

	ADD_LATENT_AUTOMATION_COMMAND(FAddKamoObjects());

	// Ensure world gets ticked so objects are serialized
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(SLEEPTIME));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKamoDbValidateTestObjects, "KamoDbTest.ValidateTestObjects", TESTFLAGS)

bool FKamoDbValidateTestObjects::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = GetTestingWorld();
	if (!TestWorld)
	{
		UE_LOG(LogKamoObjectTest, Error, TEXT("No test world found"));
		return true;
	}

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(SLEEPTIME));
	ADD_LATENT_AUTOMATION_COMMAND(FValidateKamoObjects());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKamoDbDeleteTestObjects, "KamoDbTest.DeleteTestObjects", TESTFLAGS)

bool FKamoDbDeleteTestObjects::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = GetTestingWorld();
	if (!TestWorld)
	{
		UE_LOG(LogKamoObjectTest, Error, TEXT("No test world found"));
		return true;
	}

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(SLEEPTIME));
	ADD_LATENT_AUTOMATION_COMMAND(FValidateKamoObjects());
	ADD_LATENT_AUTOMATION_COMMAND(FDeleteKamoObjects());
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(SLEEPTIME));

	return true;
}

#endif