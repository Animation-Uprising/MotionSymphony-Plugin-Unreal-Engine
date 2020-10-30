// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "MotionPreProcessorToolkitCommands.h"

#define LOCTEXT_NAMESPACE "MotionPreProcessToolkitCommands"

void FMotionPreProcessToolkitCommands::RegisterCommands()
{
	UI_COMMAND(PickAnims, "PickAnims", "Opens a context dialog to pick anims to include in the pre-processor", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(PickBones, "PickBones", "Opens a context dialog to pick which bones to match in the pre-processor", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(PreProcess, "PreProcess", "Pre-Processes and bakes  all animation data into a file ready for motion matching", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(SetShowGrid, "Grid", "Displays the viewport grid.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetShowBounds, "Bounds", "Toggles display of the bounds of the static mesh.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetShowCollision, "Collision", "Toggles display of the simplified collision mesh of the static mesh, if one has been assigned.", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::C, EModifierKey::Alt));

	UI_COMMAND(SetShowPivot, "Show Pivot", "Display the pivot location of the static mesh.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetShowMatchBones, "Show Match Bones", "Displays the MotionData bones to match.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetShowTrajectory, "Show Trajectory", "Displays the Trajectory of the animation.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetShowPose, "Show Pose Data", "Displays the pose data of nearest to the current animation frame.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetShowTrajectoryClustering, "Show Trajectory Clustering", "Displays a visualization of K-Means trajectory clustering for optimization.", EUserInterfaceActionType::ToggleButton, FInputChord());
}

#undef LOCTEXT_NAMESPACE