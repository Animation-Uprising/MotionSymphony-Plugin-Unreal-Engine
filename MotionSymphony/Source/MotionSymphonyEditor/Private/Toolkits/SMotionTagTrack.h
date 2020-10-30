// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "MotionDataAsset.h"
#include "MotionPreProcessToolkit.h"
#include "SMotionTrackTagWidget.h"

class SMotionTagTrack : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMotionTagTrack)
		: _SlateUnitsPerFrame(1)
		, _ActiveMotionData(nullptr)
		, _CurrentAnimAttr(nullptr)
	{}

	SLATE_ATTRIBUTE(float, SlateUnitsPerFrame)
	SLATE_ATTRIBUTE(class UMotionDataAsset*, ActiveMotionData)
	SLATE_ATTRIBUTE(class UAnimSequence*, CurrentAnimAttr)
	SLATE_END_ARGS()

private:
	uint8 TagIndex;
	FText TagName;

	TAttribute<float> SlateUnitsPerFrame;
	TAttribute<class UMotionDataAsset*> ActiveMotionData;

	TSharedPtr<SOverlay> TrackBoxPtr;

	float HandleWidth;

	TSharedPtr<FUICommandList> CommandList;
	TAttribute<UAnimSequence*> CurrentAnimAttr;
	TSharedPtr<class FMotionPreProcessToolkit> MotionPreProcessToolkitPtr;

public:
	void Construct(const FArguments& InArgs, TSharedPtr<FUICommandList> InCommandList, 
		TSharedPtr<FMotionPreProcessToolkit> InMotionPreProcessToolkitPtr, 
		const uint8 InTagIndex, const FText InTagName);

	void Rebuild();
};