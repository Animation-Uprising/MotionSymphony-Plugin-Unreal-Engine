// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "MotionpreProcessToolkit.h"
#include "SMotionTagTrack.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "SMotionTagTrack.h"

class SMotionTagTimeline : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMotionTagTimeline)
		: _ActiveMotionData(nullptr)
		, _CurrentAnimAttr(nullptr)
	{}

	SLATE_ATTRIBUTE(class UMotionDataAsset*, ActiveMotionData)
	SLATE_ATTRIBUTE(class UAnimSequence*, CurrentAnimAttr)
	SLATE_END_ARGS()

private:
	TAttribute<class UMotionDataAsset*> ActiveMotionData;
	TAttribute<class UAnimSequence*> CurrentAnimAttr;

	TSharedPtr<FMotionPreProcessToolkit> MotionPreProcessToolkitPtr;

	TSharedPtr<SVerticalBox> MainBoxPtr;
	TSharedPtr<FUICommandList> CommandList;

public:
	void Construct(const FArguments& InArgs, TSharedPtr<FUICommandList> InCommandList,
		TSharedPtr<FMotionPreProcessToolkit> InMotionPreProcessToolkitPtr, FText HeaderText);

	void Rebuild();
};
