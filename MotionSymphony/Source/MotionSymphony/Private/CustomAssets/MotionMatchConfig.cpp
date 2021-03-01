// Copyright 2020 Kenneth Claassen. All Rights Reserved.


#include "CustomAssets/MotionMatchConfig.h"

#define LOCTEXT_NAMESPACE "MotionMatchConfig"

UMotionMatchConfig::UMotionMatchConfig(const FObjectInitializer& ObjectInitializer)
	: SourceSkeleton(nullptr)
{
}

void UMotionMatchConfig::Initialize()
{
	if (!SourceSkeleton)
	{
		UE_LOG(LogTemp, Error, TEXT("MotionMatchConfig: Trying to initialize bone references but there is no source skeleton set. Please set a skeleton on your motion match configuration before using it"));
	}

	for (FBoneReference& BoneRef : PoseBones)
	{
		BoneRef.Initialize(SourceSkeleton);
	}
}

USkeleton* UMotionMatchConfig::GetSkeleton(bool& bInvalidSkeletonIsError)
{
	bInvalidSkeletonIsError = false;

	return SourceSkeleton;
}

USkeleton* UMotionMatchConfig::GetSkeleton()
{
	return SourceSkeleton;
}

void UMotionMatchConfig::SetSourceSkeleton(USkeleton* Skeleton)
{
	Modify();
	SourceSkeleton = Skeleton;
	MarkPackageDirty();
}


bool UMotionMatchConfig::IsSetupValid()
{
	if (!SourceSkeleton)
		return false;

	if(TrajectoryTimes.Num() == 0)
		return false;

	if(PoseBones.Num() == 0)
		return false;

	return true;
}

#undef LOCTEXT_NAMESPACE