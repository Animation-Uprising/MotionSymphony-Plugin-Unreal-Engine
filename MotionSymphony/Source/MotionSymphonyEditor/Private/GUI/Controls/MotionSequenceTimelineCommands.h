// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "EditorStyleSet.h"
#include "Framework/Commands/Commands.h"

/**
 * Defines commands for the anim sequence timeline editor
 */
class FMotionSequenceTimelineCommands : public TCommands<FMotionSequenceTimelineCommands>
{
public:
	FMotionSequenceTimelineCommands()
		: TCommands<FMotionSequenceTimelineCommands>
		(
			TEXT("MotionSequenceCurveEditor"),
			NSLOCTEXT("Contexts", "MotionSequenceTimelineEditor", "Motion Sequence Timeline Editor"),
			NAME_None,
#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 1
			FAppStyle::GetAppStyleSetName()
#else
			FEditorStyle::GetStyleSetName()
#endif
			)
	{
	}

	//TSharedPtr<FUICommandInfo> EditSelectedCurves;

	//TSharedPtr<FUICommandInfo> RemoveSelectedCurves;

	TSharedPtr<FUICommandInfo> AddNotifyTrack;

	TSharedPtr<FUICommandInfo> InsertNotifyTrack;

	TSharedPtr<FUICommandInfo> RemoveNotifyTrack;

	//TSharedPtr<FUICommandInfo> AddCurve;

	//TSharedPtr<FUICommandInfo> EditCurve;

	//TSharedPtr<FUICommandInfo> ShowCurveKeys;

	//TSharedPtr<FUICommandInfo> AddMetadata;

	//TSharedPtr<FUICommandInfo> ConvertCurveToMetaData;

	//TSharedPtr<FUICommandInfo> ConvertMetaDataToCurve;

	//TSharedPtr<FUICommandInfo> RemoveCurve;

	//TSharedPtr<FUICommandInfo> RemoveAllCurves;

	TSharedPtr<FUICommandInfo> DisplaySeconds;

	TSharedPtr<FUICommandInfo> DisplayFrames;

	TSharedPtr<FUICommandInfo> DisplayPercentage;

	TSharedPtr<FUICommandInfo> DisplaySecondaryFormat;

	TSharedPtr<FUICommandInfo> SnapToFrames;

	TSharedPtr<FUICommandInfo> SnapToNotifies;

	TSharedPtr<FUICommandInfo> SnapToMontageSections;

	TSharedPtr<FUICommandInfo> SnapToCompositeSegments;

public:
	virtual void RegisterCommands() override;
};