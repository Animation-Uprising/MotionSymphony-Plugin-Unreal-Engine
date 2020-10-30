// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "SmotionTrackTagWidget.h"
#include "SMotionTagTrack.h"
#include "MotionDataAsset.h"
#include "MotionPreProcessToolkit.h"

#include "Input/Reply.h"

#define LOCTEXT_NAMESPACE "MotionPreProcessToolkit"

void SMotionTrackTagWidget::Construct(const FArguments& InArgs, const float Time, const uint8 InTagIndex,
	TSharedPtr<FUICommandList> InCommandList, TSharedPtr<FMotionPreProcessToolkit> InMotionPreProcessToolkitPtr)
{
	KeyTime = Time;
	TagIndex = InTagIndex;

	CommandList = MakeShareable(new FUICommandList);
	CommandList->Append(InCommandList.ToSharedRef());

	ActiveMotionDataAsset = InArgs._MotionDataBeingEdited;
	MotionPreProcessToolkitPtr = InMotionPreProcessToolkitPtr;

	//Tagged = MotionPreProcessToolkitPtr.Get()->IsTimeTagged(KeyTime, TagIndex);

	ChildSlot
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.OnMouseButtonUp(this, &SMotionTrackTagWidget::OnMouseButtonUp)
				.BorderBackgroundColor(this, &SMotionTrackTagWidget::GetBorderColor)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					[
						SNew(STextBlock)
						.Text(this, &SMotionTrackTagWidget::GetStepIndexText)
						.ColorAndOpacity(this, &SMotionTrackTagWidget::GetSelectionColor)
					]
				]
			]
		];

}

FReply SMotionTrackTagWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Tagged = !Tagged;

	if (Tagged)
	{
		//MotionPreProcessToolkitPtr.Get()->AddRangeToTagHelper(KeyTime, TagIndex);
	}
	else
	{
		//MotionPreProcessToolkitPtr.Get()->RemoveRangeFromTagHelper(KeyTime, TagIndex);
	}

	//MotionPreProcessToolkitPtr.Get()->GoToTime(KeyTime);

	return FReply::Handled();
}

bool SMotionTrackTagWidget::SupportsKeyboardFocus() const
{
	return true;
}

FSlateColor SMotionTrackTagWidget::GetSelectionColor() const
{
	if (Tagged)
		return FSlateColor(FLinearColor::FromSRGBColor(FColor::Green));

	return FSlateColor(FLinearColor::FromSRGBColor(FColor::White));
}

FSlateColor SMotionTrackTagWidget::GetBorderColor() const
{
	/*if (FMath::RandBool())
		return FSlateColor(FLinearColor::Red);*/

	return FSlateColor(FLinearColor::White);
}

FText SMotionTrackTagWidget::GetStepIndexText() const
{
	return FText::AsNumber(KeyTime);
}

#undef LOCTEXT_NAMESPACE