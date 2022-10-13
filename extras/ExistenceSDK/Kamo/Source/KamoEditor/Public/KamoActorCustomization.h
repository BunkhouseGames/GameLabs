#pragma once

#include "IDetailCustomization.h"

class FKamoActorCustomization: public IDetailCustomization
{
private:

    /* Contains references to all selected objects inside in the viewport */
    TArray<TWeakObjectPtr<UObject>> SelectedObjects;

public:
    // IDetailCustomization interface
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
    //

    static TSharedRef< IDetailCustomization > MakeInstance();

    /* The code that fires when we click the "ChangeColor" button */
    FReply ClickedOnButton();
};