// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "MotionPreProcessToolkit.h"
#include "CustomAssets/MotionDataAsset.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Animation/AnimSequence.h"
#include "Misc/Attribute.h"
#include "SMotionTagTimeline.h"


class SMotionMetaDataPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMotionMetaDataPanel)
		: _ActiveMotionData((UMotionDataAsset*)nullptr)
		, _CurrentAnimAttr((UAnimSequence*)nullptr)
	{}

	SLATE_ATTRIBUTE(UMotionDataAsset*, ActiveMotionData)
	SLATE_ATTRIBUTE(UAnimSequence*, CurrentAnimAttr)

	SLATE_END_ARGS()

private:
	TAttribute<UMotionDataAsset*> ActiveMotionData;
	TAttribute<UAnimSequence*> CurrentAnimAttr;

	TSharedPtr<FMotionPreProcessToolkit> MotionPreProcessToolkitPtr;
	TSharedPtr<FUICommandList> CommandList;

	/** The fill coefficients of each column in the grid. */
	float ColumnFillCoefficients[2];

public:
	void Construct(const FArguments &InArgs, TSharedPtr<FUICommandList> InCommandList,
		TSharedPtr<FMotionPreProcessToolkit> InMotionPreProcessToolkitPtr);

private:
	float GetColumnFillCoefficient(int32 ColumnIndex) const
	{
		return ColumnFillCoefficients[ColumnIndex];
	}

	/** Called when a column fill percentage is changed by a splitter slot. */
	void OnColumnFillCoefficientChanged(float FillCoefficient, int32 ColumnIndex);
};