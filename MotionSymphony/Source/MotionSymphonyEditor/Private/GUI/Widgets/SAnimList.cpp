// Copyright 2020 Kenneth Claassen. All Rights Reserved.


#include "SAnimList.h"

#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "SScrubControlPanel.h"
#include "EditorStyleSet.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "GUI/Dialogs/AddNewAnimDialog.h"
#include "GUI/Dialogs/BonePickerDialog.h"
#include "PropertyCustomizationHelpers.h"

#define LOCTEXT_NAMESPACE "AnimList"

void SAnimWidget::Construct(const FArguments& InArgs, int32 InFrameIndex, TWeakPtr<class FMotionPreProcessToolkit> InMotionPreProcessToolkit)
{
	AnimIndex = InFrameIndex;
	MotionPreProcessToolkitPtr = InMotionPreProcessToolkit;

	const auto BorderColorDelegate = [](TAttribute<UMotionDataAsset*> ThisMotionPreProcessorPtr, int32 TestIndex) -> FSlateColor
	{
		UMotionDataAsset* MotionPreProcessorAssetPtr = ThisMotionPreProcessorPtr.Get();
		const bool bFrameValid = (MotionPreProcessorAssetPtr != nullptr);
		return bFrameValid ? FLinearColor::White : FLinearColor::Black;
	};

	TSharedRef<SWidget> ClearButton = PropertyCustomizationHelpers::MakeDeleteButton(FSimpleDelegate::CreateSP(this, &SAnimWidget::OnRemoveAnim),
		LOCTEXT("RemoveContextToolTip", "Remove Context."), true);

	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("DialogueWaveDetails.HeaderBorder"))
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Fill)
				.ButtonStyle(FEditorStyle::Get(), "FlatButton")
				.ForegroundColor(FLinearColor::White)
				.OnClicked(this, &SAnimWidget::OnAnimClicked)
				[
					SNew(STextBlock)
					.Text(this, &SAnimWidget::GetAnimAssetName)
				]
			]
		]
		+ SHorizontalBox::Slot()
		.Padding(2.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			ClearButton
		]
	];
}

FReply SAnimWidget::OnAnimClicked()
{
	MotionPreProcessToolkitPtr.Pin().Get()->SetCurrentAnimation(AnimIndex);
	
	return FReply::Handled();
}

void SAnimWidget::OnRemoveAnim()
{
	MotionPreProcessToolkitPtr.Pin().Get()->DeleteAnimSequence(AnimIndex);
}

FText SAnimWidget::GetAnimAssetName() const
{
	return MotionPreProcessToolkitPtr.Pin().Get()->GetAnimationName(AnimIndex);
}

void SAnimList::Construct(const FArguments& InArgs, TWeakPtr<class FMotionPreProcessToolkit> InMotionPreProcessToolkitPtr)
{
	MotionPreProcessToolkitPtr = InMotionPreProcessToolkitPtr;

	MainBox = SNew(SVerticalBox);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.Padding(2.0f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.Text(LOCTEXT("ClearAnimations", "Clear Animations"))
					.HAlign(HAlign_Center)
					.ToolTipText(LOCTEXT("ClearAnimationsTooltip", "Clears all animations from the list"))
					.OnClicked(FOnClicked::CreateSP(this, &SAnimList::OnClearAnims))
				]
			]
			+ SVerticalBox::Slot()
			[
				SNew(SScrollBox)
				.Orientation(Orient_Vertical)
				.ScrollBarAlwaysVisible(true)
				+ SScrollBox::Slot()
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 2)
						[
							SNew(SBox)
							[
								MainBox.ToSharedRef()
							]
						]
					]
				]
			]
		]
	];

	Rebuild();
}

void SAnimList::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltatime)
{

}

FReply SAnimList::OnAddNewAnim()
{
	SAddNewAnimDialog::ShowWindow(MotionPreProcessToolkitPtr.Pin());
	return FReply::Handled();
}

FReply SAnimList::OnClearAnims()
{
	MotionPreProcessToolkitPtr.Pin().Get()->ClearAnimList();
	return FReply::Handled();
}

void SAnimList::Rebuild()
{
	MainBox->ClearChildren();

	const int animCount = MotionPreProcessToolkitPtr.Pin().Get()->GetActiveMotionDataAsset()->GetSourceAnimCount();
	AnimWidgets.Empty(animCount);

	if (animCount > 0)
	{
		for (int32 animIndex = 0; animIndex < animCount; ++animIndex)
		{
			MainBox->AddSlot()
				.FillHeight(0.5f)
				[
					SNew(SAnimWidget, animIndex, MotionPreProcessToolkitPtr)
				];
		}
	}
}

#undef LOCTEXT_NAMESPACE