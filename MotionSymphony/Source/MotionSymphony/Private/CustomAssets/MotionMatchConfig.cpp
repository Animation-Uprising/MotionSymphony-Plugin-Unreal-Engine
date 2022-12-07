// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "CustomAssets/MotionMatchConfig.h"

#define LOCTEXT_NAMESPACE "MotionMatchConfig"

UMotionMatchConfig::UMotionMatchConfig(const FObjectInitializer& ObjectInitializer)
	: SourceSkeleton(nullptr),
	UpAxis(EAllAxis::Z),
	ForwardAxis(EAllAxis::Y)
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

#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 1
USkeleton* UMotionMatchConfig::GetSkeleton(bool& bInvalidSkeletonIsError, const IPropertyHandle* PropertyHandle)
#else
USkeleton* UMotionMatchConfig::GetSkeleton(bool& bInvalidSkeletonIsError)
#endif
{
	bInvalidSkeletonIsError = false;

	return SourceSkeleton;
}

USkeleton* UMotionMatchConfig::GetSourceSkeleton() const
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
	bool bIsValid = true;

	//Check that a source skeleton is set
	if (!SourceSkeleton)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Match Config validity check failed. Source skeleton is not set (null)."));
		bIsValid = false;
	}

	//Check that there is a trajectory
	if(TrajectoryTimes.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Match Config validity check failed. There are no trajectory points set."));
		bIsValid = false;
	}

	
	//Check that there are no zero time trajectory points 
	bool bHasFutureTime = false;
	for (float& PointTime : TrajectoryTimes)
	{
		if (PointTime > 0.0f)
		{
			bHasFutureTime = true;
		}

		if (PointTime > -0.0001f && PointTime < 0.0001f)
		{
			UE_LOG(LogTemp, Warning, TEXT("Motion Match Config: A trajectory point with time '0' (zero) is not required and should be avoided"));
		}
	}

	//Check that there is at least 1 future trajectory point
	if (!bHasFutureTime)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Match Config validity check failed. Trajectory must have at least one point in the future (+ve value)"));
		bIsValid = false;
	}

	//Check that there is at least one bone to match
	if(PoseBones.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Match Config validity check failed. There are no match bones set."));
		bIsValid = false;
	}

	return bIsValid;
}

#undef LOCTEXT_NAMESPACE