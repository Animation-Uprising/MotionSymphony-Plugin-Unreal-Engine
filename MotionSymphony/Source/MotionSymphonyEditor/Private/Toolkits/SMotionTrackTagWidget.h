// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "Layout/Margin.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Types/SlateStructs.h"
#include "MotionDataAsset.h"
#include "MotionPreProcessToolkit.h"

class SMotionTrackTagWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMotionTrackTagWidget)
		: _SlateUnitsPerFrame(1)
		, _MotionDataBeingEdited(nullptr)
	{}

	SLATE_ATTRIBUTE(float, SlateUnitsPerFrame)
		SLATE_ATTRIBUTE(class UMotionDataAsset*, MotionDataBeingEdited)
		SLATE_END_ARGS()

protected:
	float KeyTime;
	uint8 TagIndex;

	TAttribute<class UMotionDataAsset*> ActiveMotionDataAsset;
	TSharedPtr<FUICommandList> CommandList;

private:
	bool Tagged;
	TSharedPtr<class FMotionPreProcessToolkit> MotionPreProcessToolkitPtr;

public:
	void Construct(const FArguments& InArgs, const float Time, const uint8 InTagIndex,
		TSharedPtr<FUICommandList> InCommandList, TSharedPtr<class FMotionPreProcessToolkit> InMotionPreProcessToolkitPtr);

	//SWidget interface
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual bool SupportsKeyboardFocus() const override;
	//End SWidget interface

	FSlateColor GetSelectionColor() const;
	FSlateColor GetBorderColor() const;

protected:
	FText GetStepIndexText() const;
};