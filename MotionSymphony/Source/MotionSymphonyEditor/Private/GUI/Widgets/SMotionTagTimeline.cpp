// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "SMotionTagTimeline.h"
#include "Widgets/Layout/SExpandableArea.h"

#define LOCTEXT_NAMESPACE "MotionPreProcessToolkit"

void SMotionTagTimeline::Construct(const FArguments& InArgs, TSharedPtr<FUICommandList> InCommandList, 
	TSharedPtr<FMotionPreProcessToolkit> InMotionPreProcessToolkitPtr, FText HeaderText)
{
	ActiveMotionData = InArgs._ActiveMotionData;
	CurrentAnimAttr = InArgs._CurrentAnimAttr;
	CommandList = InCommandList;
	MotionPreProcessToolkitPtr = InMotionPreProcessToolkitPtr;

	const FSlateBrush* BorderBrush = FEditorStyle::GetBrush("ToolPanel.GroupBorder");

	ChildSlot
		[
			SNew(SExpandableArea)
			.BodyBorderImage(BorderBrush)
			.HeaderContent()
			[
				SNew(STextBlock)
				.Text(HeaderText)
			]
			.BodyContent()
			[
				SAssignNew(MainBoxPtr, SVerticalBox)
			]
		];

	Rebuild();
}

void SMotionTagTimeline::Rebuild()
{
	MainBoxPtr->ClearChildren();

	for (int i = 0; i < 2; ++i)
	{
		MainBoxPtr->AddSlot()
		[
			SNew(SMotionTagTrack, CommandList, MotionPreProcessToolkitPtr, i, LOCTEXT("None","None"))
			.SlateUnitsPerFrame(0)
			.ActiveMotionData(ActiveMotionData)
			.CurrentAnimAttr(CurrentAnimAttr)
		];
	}
}


#undef LOCTEXT_NAMESPACE

