// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "SMotionMetaDataPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SExpandableArea.h"

#define LOCTEXT_NAMESPACE "MotionPreProcessToolkit"

namespace FMotionPreProcessUIConstants
{
	const float MetaPanelHeight = 200.0f;
};


void SMotionMetaDataPanel::Construct(const FArguments &InArgs, 
	TSharedPtr<FUICommandList> InCommandList,
	TSharedPtr<FMotionPreProcessToolkit> InMotionPreProcessToolkitPtr)
{
	ActiveMotionData = InArgs._ActiveMotionData;
	CurrentAnimAttr = InArgs._CurrentAnimAttr;

	CommandList = InCommandList;
	MotionPreProcessToolkitPtr = InMotionPreProcessToolkitPtr;

	TSharedPtr<SMotionTagTimeline> TraitTagTimeline = SNew(SMotionTagTimeline, CommandList, 
		MotionPreProcessToolkitPtr, LOCTEXT("Trait Tags", "Trait Tags"))
		.ActiveMotionData(ActiveMotionData)
		.CurrentAnimAttr(CurrentAnimAttr);

	TSharedPtr<SMotionTagTimeline> FavourTagTimeline = SNew(SMotionTagTimeline, CommandList,
		MotionPreProcessToolkitPtr, LOCTEXT("Favour Tags", "Favour Tags"))
		.ActiveMotionData(ActiveMotionData)
		.CurrentAnimAttr(CurrentAnimAttr);

	TSharedPtr<SMotionTagTimeline> UtilityTagTimeline = SNew(SMotionTagTimeline, CommandList,
		MotionPreProcessToolkitPtr, LOCTEXT("Utility Tags", "Utility Tags"))
		.ActiveMotionData(ActiveMotionData)
		.CurrentAnimAttr(CurrentAnimAttr);

	const FSlateBrush* BorderBrush = FEditorStyle::GetBrush("ToolPanel.GroupBorder");

	ChildSlot
		[
			SNew(SBorder)
			.BorderImage(BorderBrush)
			[
				SNew(SBox)
				.HeightOverride(FMotionPreProcessUIConstants::MetaPanelHeight)
				[
					SNew(SScrollBox)
					.ScrollBarAlwaysVisible(true)
					+SScrollBox::Slot()
					[
						TraitTagTimeline.ToSharedRef()
					]
					+SScrollBox::Slot()
					[
						FavourTagTimeline.ToSharedRef()
					]
					+SScrollBox::Slot()
					[
						UtilityTagTimeline.ToSharedRef()
					]
				]
			]
		];
}

#undef LOCTEXT_NAMESPACE