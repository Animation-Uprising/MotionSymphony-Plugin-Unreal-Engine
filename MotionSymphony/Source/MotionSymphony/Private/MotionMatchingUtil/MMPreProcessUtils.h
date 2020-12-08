// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSequence.h"
#include "PoseMotionData.h"
#include "BoneContainer.h"
#include "Enumerations/EMotionMatchingEnums.h"
#include "Enumerations/EMMPreProcessEnums.h"
#include "Math/UnrealMathUtility.h"

class UMotionDataAsset;
class USkeletalMeshComponent;
struct FTrajectory;

class MOTIONSYMPHONY_API FMMPreProcessUtils
{
public:
	static void ExtractRootVelocity(FVector& OutRootVelocity, float& OutRootRotVelocity, const UAnimSequence* Anim, 
		const float Time, const float PoseInterval);

	static void ExtractPastTrajectoryPoint(FTrajectoryPoint& OutTrajPoint, const UAnimSequence* AnimSequence, 
		const float BaseTime, const float PointTime, ETrajectoryPreProcessMethod PastMethod, 
	    UAnimSequence* PrecedingMotion);

	static void ExtractFutureTrajectoryPoint(FTrajectoryPoint& OutTrajPoint, const UAnimSequence* AnimSequence,
		const float BaseTime, const float PointTime, ETrajectoryPreProcessMethod FutureMethod,
		UAnimSequence* FollowingMotion);

	static void ExtractLoopingTrajectoryPoint(FTrajectoryPoint& OutTrajPoint, const UAnimSequence* AnimSequence,
		const float BaseTime, const float PointTime);

#if WITH_EDITOR
	static void ExtractJointData(FJointData& OutjointData, const UAnimSequence* Anim,
		const int JointId, const float Time, const float PoseInterval);

	static void ExtractJointData(FJointData& OutjointData, const UAnimSequence* Anim,
		const FBoneReference& BoneReference, const float Time, const float PoseInterval);

	static void GetJointTransform_RootRelative(FTransform& OutjointTransform, const UAnimSequence* Anim,
		const int JointId, const float Time);

	static void GetJointTransform_RootRelative(FTransform& OutTransform, const UAnimSequence* AnimSequence,
		const TArray<FName>& BonesToRoot, const float Time);

	static void GetJointVelocity_RootRelative(FVector& OutjointVelocity, const UAnimSequence* Anim, 
		const int JointId, const float Time, const float PoseInterval);

	static void GetJointVelocity_RootRelative(FVector& OutVelocity, const UAnimSequence* AnimSequence,
		const TArray<FName>& BonesToRoot, const float Time, const float PoseInterval);
#endif

	static bool GetDoNotUseTag(const UAnimSequence* Anim, const float Time, const float PoseInterval);
};
