// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "MotionSymphonyStyle.h"

class FMotionPreProcessToolkitCommands : public TCommands<FMotionPreProcessToolkitCommands>
{
public:
	FMotionPreProcessToolkitCommands()
		: TCommands<FMotionPreProcessToolkitCommands>
		(
			TEXT("MotionPreProcessToolkit"),
			NSLOCTEXT("Context", "MotionPreProcessToolkit", "Motion PreProcess Editor"),
			NAME_None,
			FMotionSymphonyStyle::GetStyleSetName()
		)
	{ }

	virtual void RegisterCommands() override;

public:
	//Toolbar UI Commands
	TSharedPtr<FUICommandInfo> PickAnims;
	TSharedPtr<FUICommandInfo> ClearAnims;
	TSharedPtr<FUICommandInfo> LastAnim;
	TSharedPtr<FUICommandInfo> NextAnim;
	TSharedPtr<FUICommandInfo> PreProcess;

	//Viewport Commands
	TSharedPtr<FUICommandInfo> SetShowGrid;
	TSharedPtr<FUICommandInfo> SetShowBounds;
	TSharedPtr<FUICommandInfo> SetShowCollision;
	TSharedPtr<FUICommandInfo> SetShowPivot;
	TSharedPtr<FUICommandInfo> SetShowMatchBones;
	TSharedPtr<FUICommandInfo> SetShowTrajectory;
	TSharedPtr<FUICommandInfo> SetShowPose;
	TSharedPtr<FUICommandInfo> SetShowOptimisationDebug;

	//Animation LIst Commands
	TSharedPtr<FUICommandInfo> DeleteAnimation;
};
