// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

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
#include "PropertyCustomizationHelpers.h"
#include "MotionPreProcessToolkit.h"

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

	ButtonWidget = SNew(SButton)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Fill)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton")
		.ForegroundColor(FLinearColor::White)
		.OnClicked(this, &SAnimWidget::OnAnimClicked)
		[
			SNew(STextBlock)
			.Text(this, &SAnimWidget::GetAnimAssetName)
		];

	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("DialogueWaveDetails.HeaderBorder"))
			[
				ButtonWidget.ToSharedRef()
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
	MotionPreProcessToolkitPtr.Pin().Get()->SetCurrentAnimation(AnimIndex, EMotionAnimAssetType::Sequence);
	
	return FReply::Handled();
}

void SAnimWidget::OnRemoveAnim()
{
	MotionPreProcessToolkitPtr.Pin().Get()->DeleteAnimSequence(AnimIndex);
}

void SAnimWidget::SelectWidget()
{
	ButtonWidget->SetForegroundColor(FLinearColor::Yellow);
}

void SAnimWidget::DeselectWidget()
{
	ButtonWidget->SetForegroundColor(FLinearColor::White);
}

FText SAnimWidget::GetAnimAssetName() const
{
	return MotionPreProcessToolkitPtr.Pin().Get()->GetAnimationName(AnimIndex);
}

void SBlendSpaceWidget::Construct(const FArguments& InArgs, int32 InFrameIndex, TWeakPtr<FMotionPreProcessToolkit> InMotionPreProcessToolkit)
{
	BlendSpaceIndex = InFrameIndex;
	MotionPreProcessToolkitPtr = InMotionPreProcessToolkit;

	const auto BorderColorDelegate = [](TAttribute<UMotionDataAsset*> ThisMotionPreProcessorPtr, int32 TestIndex) -> FSlateColor
	{
		UMotionDataAsset* MotionPreProcessorAssetPtr = ThisMotionPreProcessorPtr.Get();
		const bool bFrameValid = (MotionPreProcessorAssetPtr != nullptr);
		return bFrameValid ? FLinearColor::White : FLinearColor::Black;
	};

	TSharedRef<SWidget> ClearButton = PropertyCustomizationHelpers::MakeDeleteButton(FSimpleDelegate::CreateSP(this, &SBlendSpaceWidget::OnRemoveBlendSpace),
		LOCTEXT("RemoveContextToolTip", "Remove Context."), true);

	ButtonWidget = SNew(SButton)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Fill)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton")
		.ForegroundColor(FLinearColor::White)
		.OnClicked(this, &SBlendSpaceWidget::OnBlendSpaceClicked)
		[
			SNew(STextBlock)
			.Text(this, &SBlendSpaceWidget::GetBlendSpaceAssetName)
		];

	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("DialogueWaveDetails.HeaderBorder"))
			[
				ButtonWidget.ToSharedRef()
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

FReply SBlendSpaceWidget::OnBlendSpaceClicked()
{
	MotionPreProcessToolkitPtr.Pin().Get()->SetCurrentAnimation(BlendSpaceIndex, EMotionAnimAssetType::BlendSpace);

	return FReply::Handled();
}

void SBlendSpaceWidget::OnRemoveBlendSpace()
{
	MotionPreProcessToolkitPtr.Pin().Get()->DeleteBlendSpace(BlendSpaceIndex);
}


void SBlendSpaceWidget::SelectWidget()
{
	ButtonWidget->SetForegroundColor(FLinearColor::Yellow);
}

void SBlendSpaceWidget::DeselectWidget()
{
	ButtonWidget->SetForegroundColor(FLinearColor::White);
}

FText SBlendSpaceWidget::GetBlendSpaceAssetName() const
{
	return MotionPreProcessToolkitPtr.Pin().Get()->GetBlendSpaceName(BlendSpaceIndex);
}

void SCompositeWidget::Construct(const FArguments& InArgs, int32 InFrameIndex, TWeakPtr<FMotionPreProcessToolkit> InMotionPreProcessToolkit)
{
	CompositeIndex = InFrameIndex;
	MotionPreProcessToolkitPtr = InMotionPreProcessToolkit;

	const auto BorderColorDelegate = [](TAttribute<UMotionDataAsset*> ThisMotionPreProcessorPtr, int32 TestIndex) -> FSlateColor
	{
		UMotionDataAsset* MotionPreProcessorAssetPtr = ThisMotionPreProcessorPtr.Get();
		const bool bFrameValid = (MotionPreProcessorAssetPtr != nullptr);
		return bFrameValid ? FLinearColor::White : FLinearColor::Black;
	};

	TSharedRef<SWidget> ClearButton = PropertyCustomizationHelpers::MakeDeleteButton(FSimpleDelegate::CreateSP(this, &SCompositeWidget::OnRemoveComposite),
		LOCTEXT("RemoveContextToolTip", "Remove Context."), true);

	ButtonWidget = SNew(SButton)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Fill)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton")
		.ForegroundColor(FLinearColor::White)
		.OnClicked(this, &SCompositeWidget::OnCompositeClicked)
		[
			SNew(STextBlock)
			.Text(this, &SCompositeWidget::GetCompositeAssetName)
		];

	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("DialogueWaveDetails.HeaderBorder"))
			[
				ButtonWidget.ToSharedRef()
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

FReply SCompositeWidget::OnCompositeClicked()
{
	MotionPreProcessToolkitPtr.Pin().Get()->SetCurrentAnimation(CompositeIndex, EMotionAnimAssetType::Composite);

	return FReply::Handled();
}

void SCompositeWidget::OnRemoveComposite()
{
	MotionPreProcessToolkitPtr.Pin().Get()->DeleteComposite(CompositeIndex);
}

void SCompositeWidget::SelectWidget()
{
	ButtonWidget->SetForegroundColor(FLinearColor::Yellow);
}

void SCompositeWidget::DeselectWidget()
{
	ButtonWidget->SetForegroundColor(FLinearColor::White);
}

FText SCompositeWidget::GetCompositeAssetName() const
{
	return MotionPreProcessToolkitPtr.Pin().Get()->GetCompositeName(CompositeIndex);
}


void SAnimList::Construct(const FArguments& InArgs, TWeakPtr<class FMotionPreProcessToolkit> InMotionPreProcessToolkitPtr)
{
	MotionPreProcessToolkitPtr = InMotionPreProcessToolkitPtr;

	AnimListBox = SNew(SVerticalBox);
	BlendSpaceListBox = SNew(SVerticalBox);
	CompositeListBox = SNew(SVerticalBox);

	FTextBlockStyle HeadingStyle = FTextBlockStyle::GetDefault();
	HeadingStyle.SetFontSize(16);
	HeadingStyle.SetColorAndOpacity(FSlateColor(FLinearColor::White));

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
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
						+SVerticalBox::Slot()
						.AutoHeight()
						.Padding(2, 2, 2, 2)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("SequenceHeading", "Sequences"))
							.TextStyle(&HeadingStyle)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 2)
						[
							SNew(SBox)
							[
								AnimListBox.ToSharedRef()
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(2, 5, 2, 2)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("CompositeHeading", "Composites"))
							.TextStyle(&HeadingStyle)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 2)
						[
							SNew(SBox)
							[
								CompositeListBox.ToSharedRef()
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(2, 5, 2, 2)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("BlendSpaceHeading", "Blend Spaces"))
							.TextStyle(&HeadingStyle)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 2)
						[
							SNew(SBox)
							[
								BlendSpaceListBox.ToSharedRef()
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

void SAnimList::SelectAnim(EMotionAnimAssetType AnimType, int32 AnimIndex)
{
	if (AnimIndex < 0)
		return;

	switch (AnimType)
	{
		case EMotionAnimAssetType::Sequence:
		{
			if (AnimIndex >= AnimWidgets.Num())
				return;

			AnimWidgets[AnimIndex]->SelectWidget();

		} break;
		case EMotionAnimAssetType::BlendSpace:
		{
			if (AnimIndex >= BlendSpaceWidgets.Num())
				return;

			BlendSpaceWidgets[AnimIndex]->SelectWidget();

		} break;
		case EMotionAnimAssetType::Composite:
		{
			if (AnimIndex >= CompositeWidgets.Num())
				return;

			CompositeWidgets[AnimIndex]->SelectWidget();

		} break;
	}
}

void SAnimList::DeselectAnim(EMotionAnimAssetType AnimType, int32 AnimIndex)
{
	if (AnimIndex < 0)
		return;

	switch (AnimType)
	{
		case EMotionAnimAssetType::Sequence:
		{
			if(AnimIndex >= AnimWidgets.Num())
				return;

			AnimWidgets[AnimIndex]->DeselectWidget();

		} break;
		case EMotionAnimAssetType::BlendSpace:
		{
			if (AnimIndex >= BlendSpaceWidgets.Num())
				return;
			
			BlendSpaceWidgets[AnimIndex]->DeselectWidget();

		} break;
		case EMotionAnimAssetType::Composite:
		{
			if (AnimIndex >= CompositeWidgets.Num())
				return;

			CompositeWidgets[AnimIndex]->DeselectWidget();

		} break;
	}
}

void SAnimList::Rebuild()
{
	AnimListBox->ClearChildren();
	BlendSpaceListBox->ClearChildren();
	CompositeListBox->ClearChildren();

	UMotionDataAsset* MotionDataAsset = MotionPreProcessToolkitPtr.Pin().Get()->GetActiveMotionDataAsset();

	if(!MotionDataAsset)
		return;

	const int32 AnimCount = MotionDataAsset->GetSourceAnimCount();
	AnimWidgets.Empty(AnimCount + 1);

	if (AnimCount > 0)
	{
		for (int32 AnimIndex = 0; AnimIndex < AnimCount; ++AnimIndex)
		{
			AnimWidgets.Emplace(SNew(SAnimWidget, AnimIndex, MotionPreProcessToolkitPtr));

			AnimListBox->AddSlot()
				.FillHeight(0.5f)
				[
					AnimWidgets[AnimIndex].ToSharedRef()
				];
		}
	}

	const int32 BlendSpaceCount = MotionDataAsset->GetSourceBlendSpaceCount();
	BlendSpaceWidgets.Empty(BlendSpaceCount + 1);

	if (BlendSpaceCount > 0)
	{
		for (int32 BlendSpaceIndex = 0; BlendSpaceIndex < BlendSpaceCount; ++BlendSpaceIndex)
		{
			BlendSpaceWidgets.Emplace(SNew(SBlendSpaceWidget, BlendSpaceIndex, MotionPreProcessToolkitPtr));

			BlendSpaceListBox->AddSlot()
				.FillHeight(0.5f)
				[
					BlendSpaceWidgets[BlendSpaceIndex].ToSharedRef()
				];
		}
	}

	const int32 CompositeCount = MotionDataAsset->GetSourceCompositeCount();
	CompositeWidgets.Empty(CompositeCount+1);

	if (CompositeCount > 0)
	{
		for (int32 CompositeIndex = 0; CompositeIndex < CompositeCount; ++CompositeIndex)
		{
			CompositeWidgets.Emplace(SNew(SCompositeWidget, CompositeIndex, MotionPreProcessToolkitPtr));

			CompositeListBox->AddSlot()
				.FillHeight(0.5f)
				[
					CompositeWidgets[CompositeIndex].ToSharedRef()
				];
		}
	}
}

#undef LOCTEXT_NAMESPACE

