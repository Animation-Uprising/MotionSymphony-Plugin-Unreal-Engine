// Copyright 2020 Kenneth Claassen. All Rights Reserved.


#include "MotionMatchConfig.h"

#define LOCTEXT_NAMESPACE "MotionMatchConfig"

UMotionMatchConfig::UMotionMatchConfig(const FObjectInitializer& ObjectInitializer)
	: SourceSkeleton(nullptr)
{
}

USkeleton* UMotionMatchConfig::GetSourceSkeleton()
{
	return SourceSkeleton;
}

void UMotionMatchConfig::SetSourceSkeleton(USkeleton* skeleton)
{
	Modify();
	SourceSkeleton = skeleton;
	MarkPackageDirty();
}


bool UMotionMatchConfig::IsSetupValid()
{
	if (!SourceSkeleton)
		return false;

	if(PoseJoints.Num() == 0)
		return false;

	if(TrajectoryTimes.Num() == 0)
		return false;

	return true;
}

#undef LOCTEXT_NAMESPACE