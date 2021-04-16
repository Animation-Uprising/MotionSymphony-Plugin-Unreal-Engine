// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "Data/MotionAction.h"

FMotionAction::FMotionAction()
	: ActionId(-1),
	PoseId(-1),
	Time(0.0f),
	Trait(FMotionTraitField()),
	LastActionId(-1),
	NextActionId(-1)
{
}


FMotionAction::FMotionAction(int32 InActionId, int32 InPoseId, float InTime, FMotionTraitField InTrait, int32 InLastActionId, int32 InNextActionId)
	: ActionId(InActionId),
	PoseId(InPoseId),
	Time(InTime),
	Trait(InTrait),
	LastActionId(InLastActionId),
	NextActionId(InNextActionId)
{
}

FMotionActionPayload::FMotionActionPayload()
	: ActionId(-1),
	 LeadLength(0.0f),
	 TailLength(0.0f)
{
}
