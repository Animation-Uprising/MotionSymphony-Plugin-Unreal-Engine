// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "SMotionTagTrack.h"
#include "CustomAssets/MotionDataAsset.h"
#include "MotionPreProcessToolkit.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Input/SButton.h"


#define LOCTEXT_NAMESPACE "MotionPreProcessToolkit"

void SMotionTagTrack::Construct(const FArguments& InArgs, TSharedPtr<FUICommandList> InCommandList,
	TSharedPtr<FMotionPreProcessToolkit> InMotionPreProcessToolkitPtr,
	const uint8 InTagIndex, const FText InTagName)
{
	TagIndex = InTagIndex;
	TagName = InTagName;
	CommandList = InCommandList;
	SlateUnitsPerFrame = InArgs._SlateUnitsPerFrame;
	ActiveMotionData = InArgs._ActiveMotionData;
	CurrentAnimAttr = InArgs._CurrentAnimAttr;
	MotionPreProcessToolkitPtr = InMotionPreProcessToolkitPtr;

	ChildSlot
		[
			SNew(SBorder)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					SNew(SBorder)
					[
						SAssignNew(TrackBoxPtr, SOverlay)
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(200.0f)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						[
							SNew(SInlineEditableTextBlock)
						]
						+ SHorizontalBox::Slot()
							.AutoWidth()
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
							.Content()
							[
								SNew(STextBlock)
								.Text(LOCTEXT("+", "+"))
							]
						]
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
							.Content()
							[
								SNew(STextBlock)
								.Text(LOCTEXT("-", "-"))
							]
						]
						
					]
				]
				
				/*+ SHorizontalBox::Slot()
				.FillWidth(0.6f)
				[
					SAssignNew(TrackBoxPtr, SHorizontalBox)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.3f)
				[
					SNew(SEditableTextBox)
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SBox)
					.WidthOverride(50.0f)	
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.FillWidth(0.5f)
						[
							SNew(SButton)
							.Content()
							[
								SNew(STextBlock)
								.Text(LOCTEXT("-", "-"))
							]
						]
						+SHorizontalBox::Slot()
						.FillWidth(0.5f)
						[
							SNew(SButton)
							.Content()
							[
								SNew(STextBlock)
								.Text(LOCTEXT("+", "+"))
							]
						]
					]
				]*/
			]
		];

	Rebuild();
}


void SMotionTagTrack::Rebuild()
{
	TrackBoxPtr->ClearChildren();

	if (UAnimSequence* SourceAnim = CurrentAnimAttr.Get())
	{
		float animLength = SourceAnim->SequenceLength;
		float AccumulatedTime = 0.0f;
		//float PoseInterval = MotionPreProcessToolkitPtr.Get()->GetActiveMotionDataAsset()->GetPoseInterval();

		//Cycle through all the tags and add them here.
		//Needs a background

		/*while (AccumulatedTime <= animLength)
		{
			MainBoxPtr->AddSlot()
				.FillWidth(0.5f)
				[
					SNew(SMotionTrackTagWidget, AccumulatedTime, TagIndex, CommandList, MotionPreProcessToolkitPtr)
					.SlateUnitsPerFrame(this->SlateUnitsPerFrame)
				.MotionDataBeingEdited(this->ActiveMotionData)
				];

			AccumulatedTime += PoseInterval;
		}*/
	}
}

#undef LOCTEXT_NAMESPACE

