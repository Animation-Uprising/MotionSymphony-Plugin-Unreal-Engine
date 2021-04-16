// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "MotionSequenceTimelineCommands.h"

#define LOCTEXT_NAMESPACE "MotionSequenceTimelineCommands"

void FMotionSequenceTimelineCommands::RegisterCommands()
{
	//UI_COMMAND(EditSelectedCurves, "Edit Selected Curves", "Edit the selected curves in the curve editor tab", EUserInterfaceActionType::Button, FInputChord());
	//UI_COMMAND(RemoveSelectedCurves, "Remove Selected Curves", "Remove the selected curves", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(AddNotifyTrack, "Add Tag Track", "Add a new tag track", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(InsertNotifyTrack, "Insert Tag Track", "Insert a new tag track above here", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(RemoveNotifyTrack, "Remove Tag Track", "Remove this tag track", EUserInterfaceActionType::Button, FInputChord());
	//UI_COMMAND(AddCurve, "Add Curve...", "Add a new variable float curve", EUserInterfaceActionType::Button, FInputChord());
	//UI_COMMAND(EditCurve, "Edit Curve", "Edit this curve in the curve editor tab", EUserInterfaceActionType::Button, FInputChord());
	//UI_COMMAND(ShowCurveKeys, "Show Curve Keys", "Show keys for all curves in the timeline", EUserInterfaceActionType::ToggleButton, FInputChord());
	//UI_COMMAND(AddMetadata, "Add Metadata...", "Add a new constant (metadata) float curve", EUserInterfaceActionType::Button, FInputChord());
	//UI_COMMAND(ConvertCurveToMetaData, "Convert To Metadata", "Convert this curve to a constant (metadata) curve.\nThis is a destructive operation and will remove all keys from this curve.", EUserInterfaceActionType::Button, FInputChord());
	//UI_COMMAND(ConvertMetaDataToCurve, "Convert To Curve", "Convert this metadata curve to a full curve", EUserInterfaceActionType::Button, FInputChord());
	//UI_COMMAND(RemoveCurve, "Remove Curve", "Remove this curve", EUserInterfaceActionType::Button, FInputChord());
	//UI_COMMAND(RemoveAllCurves, "Remove All Curves", "Remove all the curves in this animation", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DisplaySeconds, "Seconds", "Display the time in seconds", EUserInterfaceActionType::RadioButton, FInputChord());
	UI_COMMAND(DisplayFrames, "Frames", "Display the time in frames", EUserInterfaceActionType::RadioButton, FInputChord());
	UI_COMMAND(DisplayPercentage, "Percentage", "Display the percentage along with the time with the scrubber", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(DisplaySecondaryFormat, "Secondary", "Display the time or frame count as a secondary format along with the scrubber", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SnapToFrames, "Frames", "Snap to frame boundaries", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SnapToNotifies, "Notifies", "Snap to notifies and notify states", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SnapToMontageSections, "Montage Sections", "Snap to montage sections", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SnapToCompositeSegments, "Composite Segments", "Snap to composite segments", EUserInterfaceActionType::ToggleButton, FInputChord());
}

#undef LOCTEXT_NAMESPACE