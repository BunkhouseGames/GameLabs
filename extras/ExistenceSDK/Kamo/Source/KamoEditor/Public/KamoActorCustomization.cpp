// MyCustomization.cpp
#include "KamoActorCustomization.h"
#include "Input/Reply.h"
#include "PropertyEditing.h"

#define LOCTEXT_NAMESPACE "KamoEditorModule"

TSharedRef< IDetailCustomization > FKamoActorCustomization::MakeInstance()
{
	GLog->Log("FKamoActorCustomization::MakeInstance()");
    return MakeShareable(new FKamoActorCustomization);
}

void FKamoActorCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	GLog->Log("CustomizeDetails");
	IDetailCategoryBuilder& KamoCategory = DetailBuilder.EditCategory("Kamo");

	//Store the currently selected objects from the viewport to the SelectedObjects array.
	DetailBuilder.GetObjectsBeingCustomized(SelectedObjects);

	//Adding a custom row
	KamoCategory.AddCustomRow(FText::FromString("Outline Color Changing Category"))
		.ValueContent()
		.VAlign(VAlign_Center) // set vertical alignment to center
		.MaxDesiredWidth(250)
		[ //With this operator we declare a new slate object inside our widget row
		  //In this case the slate object is a button
			SNew(SButton)
			.VAlign(VAlign_Center)
		.OnClicked(this, &FKamoActorCustomization::ClickedOnButton) //Binding the OnClick function we want to execute when this object is clicked
		.Content()
		[ //We create a new slate object inside our button. In this case a text block with the content of "Change Color"
		  //If you ever coded a UMG button with a text on top of it you will notice that the process is quite the same
		  //Meaning, you first declare a button which has various events and properties and then you place a Text Block widget
		  //inside the button's widget to display text
			SNew(STextBlock).Text(FText::FromString("Generate Unique ID"))
		]
		];


}FReply FKamoActorCustomization::ClickedOnButton()
{
	GLog->Log("fancy cubes got a nice random color!");


	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE