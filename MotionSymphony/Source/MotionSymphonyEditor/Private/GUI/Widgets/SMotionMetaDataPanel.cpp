// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "SMotionMetaDataPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Input/SSpinBox.h"
#include "Layout/Margin.h"
#include "Misc/Attribute.h"

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

	TSharedPtr<SMotionTagTimeline> TagTimeline = SNew(SMotionTagTimeline, CommandList, 
		MotionPreProcessToolkitPtr, LOCTEXT("Trait Tags", "Tags"))
		.ActiveMotionData(ActiveMotionData)
		.CurrentAnimAttr(CurrentAnimAttr);

	const FSlateBrush* BorderBrush = FEditorStyle::GetBrush("ToolPanel.GroupBorder");

	const int32 Column0 = 0, Column1 = 1;
	const int32 Row0 = 0, Row1 = 1, Row2 = 2, Row3 = 3, Row4 = 4;
	const FMargin ResizeBarPadding(4.0f, 0.0f, 0.0f, 0.0f);

	ColumnFillCoefficients[0] = 0.2f;
	ColumnFillCoefficients[1] = 0.8f;
	
	TAttribute<float> FillCoefficient_0, FillCoefficient_1;
	{
		FillCoefficient_0.Bind(TAttribute<float>::FGetter::CreateSP(this, &SMotionMetaDataPanel::GetColumnFillCoefficient, 0));
		FillCoefficient_1.Bind(TAttribute<float>::FGetter::CreateSP(this, &SMotionMetaDataPanel::GetColumnFillCoefficient, 1));
	}

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
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							SNew(SGridPanel)
							.FillRow(1, 1.0f)
							.FillColumn(Column0, FillCoefficient_0)
							.FillColumn(Column1, FillCoefficient_1)

							//Top Left Bar
							+SGridPanel::Slot(Column0, Row0, SGridPanel::Layer(10))
							[
								SNew(SHorizontalBox)
								+SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.VAlign(VAlign_Center)
								//[
									//Search Bar Here
								//]

								+SHorizontalBox::Slot()
								.VAlign(VAlign_Center)
								.HAlign(HAlign_Center)
								.AutoWidth()
								.Padding(2.0f, 0.0f, 2.0f, 0.0f)
								[
									SNew(SBox)
									.MinDesiredWidth(30.0f)
									.VAlign(VAlign_Center)
									.HAlign(HAlign_Center)
									[
										SNew(SSpinBox<double>)
										.Style(&FEditorStyle::GetWidgetStyle<FSpinBoxStyle>("Sequencer.PlayTimeSpinBox"))
										.Value_Lambda([this]() -> double
										{
											return 0.0; //Return the scrub position
										})
										//.OnValueChanged(this, &SAnimTimeline::SetPlayTime)
										.OnValueCommitted_Lambda([this](double InFrame, ETextCommit::Type)
										{
											//SetPlayTime(InFrame)
										})
										.MinValue(TOptional<double>())
										.MaxValue(TOptional<double>())
										//.TypeInterface(NumericTypeInterface)
										//.Delta(this, &SMotionMetaPanel::GetSpinboxDelta)
										.LinearDeltaSensitivity(25)
									]
								]

							]

							//Main Timeline Area
							+SGridPanel::Slot(Column0, Row1, SGridPanel::Layer(10))
							.ColumnSpan(2)
							[
							
								TagTimeline.ToSharedRef()
							]
						
							//Timeline Scrubber
							+SGridPanel::Slot(Column0, Row3, SGridPanel::Layer(10))
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Center)

							//Second Column
							+SGridPanel::Slot(Column1, Row0)
							.Padding(ResizeBarPadding)
							.RowSpan(2)
							[
								SNew(SBorder)
								.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
								[
									SNew(SSpacer)
								]
							]

							+SGridPanel::Slot(Column1, Row0, SGridPanel::Layer(10))
							.Padding(ResizeBarPadding)
							[
								SNew(SBorder)
								.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
								.BorderBackgroundColor(FLinearColor(0.50f, 0.50f, 1.0f))
								.Padding(0)
								.Clipping(EWidgetClipping::ClipToBounds)
								/*[
									TopTimeSlider.ToSharedRef()
								]*/
							]

							//Overlay that draws the tick lines
							+SGridPanel::Slot(Column1, Row1, SGridPanel::Layer(10))
							.Padding(ResizeBarPadding)
							/*[
								SNew(SAnimTimelineOverlay, TimeSliderControllerRef)
							]*/

							//Overlay that draws the scrub position
							+SGridPanel::Slot(Column1, Row1, SGridPanel::Layer(20))
							.Padding(ResizeBarPadding)
							/*[
								SNew(SAnimTimelineOverlay, TimeSliderControllerRef)
							]*/

							+SGridPanel::Slot(Column1, Row3, SGridPanel::Layer(10))
							.Padding(ResizeBarPadding)
							[
								SNew(SBorder)
								.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
								.BorderBackgroundColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f))
								.Clipping(EWidgetClipping::ClipToBounds)
								.Padding(0)
								/*[
									BottomTimeRange
								]*/
							]
						]
						+SOverlay::Slot()
						[
							SNew(SSplitter/*SAnimTimelineSplitterOverlay*/)
							.Style(FEditorStyle::Get(), "AnimTimeline.Outliner.Splitter")
							.Visibility(EVisibility::SelfHitTestInvisible)

							+ SSplitter::Slot()
							.Value(FillCoefficient_0)
							.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &SMotionMetaDataPanel::OnColumnFillCoefficientChanged, 0))
							[
								SNew(SSpacer)
							]

							+ SSplitter::Slot()
							.Value(FillCoefficient_1)
							.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &SMotionMetaDataPanel::OnColumnFillCoefficientChanged, 1))
							[
								SNew(SSpacer)
							]
						]
					]
				]
			]
		];
}

void SMotionMetaDataPanel::OnColumnFillCoefficientChanged(float FillCoefficient, int32 ColumnIndex)
{
	ColumnFillCoefficients[ColumnIndex] = FillCoefficient;
}

#undef LOCTEXT_NAMESPACE