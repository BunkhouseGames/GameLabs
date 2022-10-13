// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#include "KamoEditorModule.h"
#include "KamoActorCustomization.h"
#include "PropertyEditorModule.h"

DEFINE_LOG_CATEGORY(LogKamoEditorModule);


FKamoEditorModule::FKamoEditorModule()
{
}


FKamoEditorModule::~FKamoEditorModule()
{
}


void FKamoEditorModule::StartupModule()
{
    UE_LOG(LogTemp, Display, TEXT("KamoEditorModule module has started!"));

    // Register detail customizations
    auto& PropertyModule = FModuleManager::LoadModuleChecked< FPropertyEditorModule >("PropertyEditor");

    // Register our customization to be used by a class 'UMyClass' or 'AMyClass'. Note the prefix must be dropped.
    PropertyModule.RegisterCustomClassLayout(AActor::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FKamoActorCustomization::MakeInstance));
}

void FKamoEditorModule::ShutdownModule()
{
    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        auto& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

        PropertyModule.UnregisterCustomClassLayout("MyClass");
    }
}

IMPLEMENT_MODULE(FKamoEditorModule, KamoEditor)
