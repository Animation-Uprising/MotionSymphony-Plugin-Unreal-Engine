// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "MotionPreProcessorToolkitCommands.h"

#define LOCTEXT_NAMESPACE "MotionPreProcessToolkitCommands"

void FMotionPreProcessToolkitCommands::RegisterCommands()
{
	UI_COMMAND(PickAnims, "Pick Animations", "Opens a context dialog to pick anims to include in the MotionAnimData", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ClearAnims, "Clear Animations", "Clears all the animations currently in the MotionAnimData", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(LastAnim, "Last Animation", "Selects and preview the previous animation in the list", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(NextAnim, "Next Animation", "Selects and preview the next animation in the list", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(PreProcess, "PreProcess", "Pre-Processes and bakes  all animation data into a file ready for motion matching", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(SetShowGrid, "Grid", "Displays the viewport grid.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetShowBounds, "Bounds", "Toggles display of the bounds of the static mesh.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetShowCollision, "Collision", "Toggles display of the simplified collision mesh of the static mesh, if one has been assigned.", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::C, EModifierKey::Alt));

	UI_COMMAND(SetShowPivot, "Show Pivot", "Display the pivot location of the static mesh.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetShowMatchBones, "Show Match Bones", "Displays the MotionData bones to match.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetShowTrajectory, "Show Trajectory", "Displays the Trajectory of the animation.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetShowPose, "Show Pose Data", "Displays the pose data of nearest to the current animation frame.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetShowOptimisationDebug, "Show Optimization Debug", 
		"Displays debug data relating to the current optimisation module. This may vary depending on the module being used.", EUserInterfaceActionType::ToggleButton, FInputChord());

	UI_COMMAND(DeleteAnimation, "Delete Animation", "Deletes the selected animation from the animation list.", EUserInterfaceActionType::None, FInputChord());
}

#undef LOCTEXT_NAMESPACE