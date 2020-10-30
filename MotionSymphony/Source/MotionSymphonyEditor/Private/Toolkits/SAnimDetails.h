// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "MotionDataAsset.h"
#include "MotionPreProcessToolkit.h"

//FDeclare
class SAnimDetailsRow;
class SAnimDetailsCategory;
class SAnimDetails;

//Slate compound widget SAnimDetailsRow
class SAnimDetailsRow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimDetailsRow)
		: _RowName()
	{}
	SLATE_ATTRIBUTE(FText, RowName)
	SLATE_END_ARGS()

private:
	TAttribute<FText> RowName;

	TSharedPtr<SAnimDetailsCategory> ParentCategory;
	TSharedPtr<SAnimDetails> AnimDetails;
	TSharedPtr<SHorizontalBox> RowBox;

	TSharedPtr<SSplitter> ColumnSplitter;

public:
	void Construct(const FArguments& InArgs, TSharedPtr<SAnimDetailsCategory> InParentCategory,
		TSharedPtr<SAnimDetails> InParentDetails);

	void SetSplitterPosition(float SplitterPosition);

	const FSlateBrush* GetBorderImage() const;
	bool IsHighlighted() const;

private:
	void OnSplitterResize(float InNewWidth);
	
};

//Slate compound widget SAnimDetailsCategory
class SAnimDetailsCategory : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimDetailsCategory)
		: _TitleText()
		, _RowCount()
	{}
	SLATE_ATTRIBUTE(FText, TitleText)
	SLATE_ATTRIBUTE(int32, RowCount)
	SLATE_END_ARGS()

private:
	TArray<TSharedPtr<SAnimDetailsRow>> DetailRows;
	
	TSharedPtr<SAnimDetails> ParentDetails;
	TSharedPtr<SVerticalBox> CategoryBox;

	TAttribute<FText> TitleText;
	TAttribute<int32> RowCount;

	TSharedPtr<SSplitter> ColumnSplitter;

public:
	void Construct(const FArguments& InArgs, TSharedPtr<SAnimDetails> InParentDetails);

	void AddRow(FText Name, TSharedPtr<SHorizontalBox> ValueBox);
	void SetSplitterPosition(float SplitterPosition);
};

//Slate compound widget SAnimDetails
class SAnimDetails : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimDetails)
		: _ActiveMotionData(nullptr)
		, _CurrentAnimAttr(nullptr)
	{}

	SLATE_ATTRIBUTE(class UMotionDataAsset*, ActiveMotionData)
	SLATE_ATTRIBUTE(class UAnimSequence*, CurrentAnimAttr)
	SLATE_END_ARGS()

private:
	TAttribute<class UMotionDataAsset*> ActiveMotionData;
	TAttribute<class UAnimSequence*> CurrentAnimAttr;

	TWeakPtr<FMotionPreProcessToolkit> MotionPreProcessToolkitPtr;

	TSharedPtr<SVerticalBox> MainBox;

	TArray<TSharedPtr<SAnimDetailsCategory>> Categories;

	float SplitterPosition;

public:
	void Construct(const FArguments& InArgs, TWeakPtr<class FMotionPreProcessToolkit> InMotionPreProcessToolkitPtr);
	void Rebuild();

	void OnDetailsSplitterResize(float InNewWidth);
private:
	void OnGlobalTagTextCommitted(const FText& InText, ETextCommit::Type);
};