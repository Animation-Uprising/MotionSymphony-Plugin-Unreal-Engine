// Copyright 2020 Kenneth Claassen. All Rights Reserved.


#include "SAnimDetails.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"

#define LOCTEXT_NAMESPACE "AnimDetails"

void SAnimDetailsRow::Construct(const FArguments& InArgs, 
	TSharedPtr<SAnimDetailsCategory> InParentCategory,
	TSharedPtr<SAnimDetails> InAnimDetails)
{
	ParentCategory = InParentCategory;
	AnimDetails = InAnimDetails;

	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(SBorder)
			.BorderImage(this, &SAnimDetailsRow::GetBorderImage)
			[
				SAssignNew(ColumnSplitter, SSplitter)
				.Style(FEditorStyle::Get(), "DetailsView.Splitter")
				.PhysicalSplitterHandleSize(1.0f)
				.HitDetectionSplitterHandleSize(5.0f)
				+ SSplitter::Slot()
				.Value(0.3f)
				.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &SAnimDetailsRow::OnSplitterResize))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SSpacer)
						.Size(FVector2D(15.0f, 15.0f))
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 4.0f, 4.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Global Tag", "Global Tag"))
					]
				]
				+ SSplitter::Slot()
				.Value(0.7f)
				[
					SNew(SBox)
					.Padding(4.0f)
					.MinDesiredWidth(500.0f)
					.HAlign(HAlign_Left)
					[
						SAssignNew(RowBox, SHorizontalBox)

						/*SNew(SEditableTextBox)
						.Text(LOCTEXT("Test", "Test"))
						.MinDesiredWidth(125.0f)*/
						//.OnTextCommitted(this, &SAnimDetails::OnGlobalTagTextCommitted)
					]
				]
			]
		]
	];
}

void SAnimDetailsRow::OnSplitterResize(float InNewWidth)
{
	//Leave blank because we want splitter size to be managed globally
}

void SAnimDetailsRow::SetSplitterPosition(float SplitterPosition)
{
	if (!ColumnSplitter.IsValid())
		return;

	ColumnSplitter->SlotAt(0)
		.Value(SplitterPosition);
}

const FSlateBrush* SAnimDetailsRow::GetBorderImage() const
{
	if (IsHighlighted())
	{
		return FEditorStyle::GetBrush("DetailsView.CategoryMiddle_Highlighted");
	}
	else if (false/*bIsDragDropObject*/)
	{
		return FEditorStyle::GetBrush("DetailsView.CategoryMiddle_Active");
	}
	else if (IsHovered()/* && !bIsHoveredDragTarget*/)
	{
		return FEditorStyle::GetBrush("DetailsView.CategoryMiddle_Hovered");
	}
	else if (false/*bIsHoveredDragTarget*/)
	{
		return FEditorStyle::GetBrush("DetailsView.CategoryMiddle_Highlighted");
	}
	else
	{
		return FEditorStyle::GetBrush("DetailsView.CategoryMiddle");
	}
}

bool SAnimDetailsRow::IsHighlighted() const
{
	return false/*OwnerTreeNode.Pin()->IsHighlighted()*/;
}

void SAnimDetailsCategory::Construct(const FArguments& InArgs, 
	TSharedPtr<SAnimDetails> InParentDetails)
{
	ParentDetails = InParentDetails;

	DetailRows.Empty(InArgs._RowCount.Get());

	ChildSlot
	[
		SNew(SExpandableArea)
		.HeaderContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(8.0f, 0.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBorder)
				]
				+ SOverlay::Slot()
				.Padding(8.0f)
				[
					SNew(STextBlock)
					.Text(InArgs._TitleText)
				]
			]
		]
		.BodyContent()
		[
			SAssignNew(CategoryBox, SVerticalBox)
		]
	];

	/*for (int32 i = 0; i < DetailRows.Num(); ++i)
	{
		CategoryBox->AddSlot()
		.AutoHeight()
		[
			SNew(SAnimDetailsRow, SharedThis(this), InParentDetails)
		];
	}*/
}



void SAnimDetailsCategory::AddRow(FText Name, TSharedPtr<SHorizontalBox> ValueBox)
{
	if (!CategoryBox.IsValid())
		return;

	TSharedPtr<SAnimDetailsRow> newRow = SNew(SAnimDetailsRow, SharedThis(this), ParentDetails)
		.RowName(Name);

	if (ValueBox.IsValid())
	{
		//newRow->SetContent(ValueBox);
	}

	CategoryBox->AddSlot()
	.AutoHeight()
	[
		newRow.ToSharedRef()
	];

}

void SAnimDetailsCategory::SetSplitterPosition(float SplitterPosition)
{
	ColumnSplitter->SlotAt(0)
		.Value(SplitterPosition);

	for (auto& row : DetailRows)
	{
		row->SetSplitterPosition(SplitterPosition);
	}
}

void SAnimDetails::Construct(const FArguments& InArgs, 
	TWeakPtr<class FMotionPreProcessToolkit> InMotionPreProcessToolkitPtr)
{
	ActiveMotionData = InArgs._ActiveMotionData;
	CurrentAnimAttr = InArgs._CurrentAnimAttr;
	SplitterPosition = 0.3f;

	MotionPreProcessToolkitPtr = InMotionPreProcessToolkitPtr;

	MainBox = SNew(SVerticalBox);

	Categories.Empty(2);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SScrollBox)
			.Orientation(Orient_Vertical)
			.ScrollBarAlwaysVisible(false)
			+ SScrollBox::Slot()
			[
				SAssignNew(MainBox, SVerticalBox)
			]
		]
	];

	Rebuild();
}

//The AnimDetails panel should be rebuilt whenever the animation is changed
void SAnimDetails::Rebuild()
{
	MainBox->ClearChildren();

	const FSlateBrush* BorderBrush = FEditorStyle::GetBrush("ToolPanel.GroupBorder");

	TSharedPtr<SAnimDetailsCategory> PreProcessCategory = SNew(SAnimDetailsCategory, SharedThis(this))
		.TitleText(LOCTEXT("Pre Process", "Pre Process"))
		.RowCount(2);

	PreProcessCategory->AddRow(LOCTEXT("Loop", "Loop"), nullptr);
	PreProcessCategory->AddRow(LOCTEXT("Flatten Trajectory", "Flatten Trajectory"), nullptr);

	TSharedPtr<SAnimDetailsCategory> RuntimeCategory = SNew(SAnimDetailsCategory, SharedThis(this))
		.TitleText(LOCTEXT("Runtime", "Runtime"))
		.RowCount(1);

	RuntimeCategory->AddRow(LOCTEXT("Global Tag", "Global Tag"), nullptr);

	Categories.Add(PreProcessCategory);
	Categories.Add(RuntimeCategory);

	MainBox->AddSlot()
	.AutoHeight()
	[
		PreProcessCategory.ToSharedRef()
	];

	MainBox->AddSlot()
	.AutoHeight()
	[
		RuntimeCategory.ToSharedRef()
	];



	/*MainBox->AddSlot()
	.AutoHeight()
	[
		SNew(SExpandableArea)
		.HeaderContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(8.0f, 0.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBorder)
				]
				+ SOverlay::Slot()
				.Padding(8.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Pre Process", "Pre Process"))
				]
			]
		]
		.BodyContent()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SCheckBox)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Loop", "Loop"))
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SMultiBoxWidget)
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SCheckBox)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Flatten Trajectory", "Flatten Trajectory"))
				]
			]
		]
	];
	
	MainBox->AddSlot()
	.AutoHeight()
	[
		SNew(SExpandableArea)
		.HeaderContent()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.Padding(8.0f, 0.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBorder)
					
				]
				+ SOverlay::Slot()
				.Padding(8.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Runtime", "Runtime"))
				]
			]
		]
		.BodyContent()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					SNew(SSplitter)
					.Style(FEditorStyle::Get(), "DetailsView.Splitter")
					.PhysicalSplitterHandleSize(1.0f)
					.HitDetectionSplitterHandleSize(5.0f)
					+ SSplitter::Slot()
					.Value(0.3f)
					.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &SAnimDetails::OnDetailsSplitterResize))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SSpacer)
							.Size(FVector2D(15.0f, 15.0f))
						]
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.Padding(0.0f, 4.0f, 4.0f, 4.0f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Global Tag", "Global Tag"))
						]
					]
					+ SSplitter::Slot()
					.Value(0.7f)
					[
						SNew(SBox)
						.Padding(4.0f)
						.MinDesiredWidth(500.0f)
						.HAlign(HAlign_Left)
						[
							SNew(SEditableTextBox)
							.Text(LOCTEXT("Test", "Test"))
							.MinDesiredWidth(125.0f)
							.OnTextCommitted(this, &SAnimDetails::OnGlobalTagTextCommitted)
						]
					]
				]
			]
		]
	];*/
}

void SAnimDetails::OnGlobalTagTextCommitted(const FText& InText, ETextCommit::Type)
{

}

void SAnimDetails::OnDetailsSplitterResize(float InNewWidth)
{
	//Change the size of all other splitter slots in the details panel
	for (auto& category : Categories)
	{
		category->SetSplitterPosition(InNewWidth);
	}
}

#undef LOCTEXT_NAMESPACE