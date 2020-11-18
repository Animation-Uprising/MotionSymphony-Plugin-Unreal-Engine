// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSequence.h"
#include "PoseMotionData.h"
#include "BoneContainer.h"
#include "Enumerations//ETrajectoryPreProcessMethod.h"
#include "Math/UnrealMathUtility.h"

class UMotionDataAsset;
class USkeletalMeshComponent;
struct FTrajectory;

class MOTIONSYMPHONY_API FMMPreProcessUtils
{
public:
	static void ExtractRootVelocity(FVector& o_rootVelocity, const UAnimSequence* a_anim, 
		const float a_time, const float a_poseInterval);

	static void ExtractPastTrajectoryPoint(FTrajectoryPoint& outTrajPoint, const UAnimSequence* AnimSequence, 
		const float BaseTime, const float PointTime, ETrajectoryPreProcessMethod PastMethod, 
	    UAnimSequence* PrecedingMotion);

	static void ExtractFutureTrajectoryPoint(FTrajectoryPoint& outTrajPoint, const UAnimSequence* AnimSequence,
		const float BaseTime, const float PointTime, ETrajectoryPreProcessMethod FutureMethod,
		UAnimSequence* FollowingMotion);

	static void ExtractLoopingTrajectoryPoint(FTrajectoryPoint& outTrajPoint, const UAnimSequence* AnimSequence,
		const float BaseTime, const float PointTime);

	static void ExtractJointData(FJointData& o_jointData, const UAnimSequence* a_anim,
		const int a_jointId, const float a_time, const float a_poseInterval);

	static void ExtractJointData(FJointData& o_jointData, const UAnimSequence* a_anim,
		const FBoneReference& BoneReference, const float a_time, const float a_poseInterval);

	static void GetJointTransform_RootRelative(FTransform& o_jointTransform, const UAnimSequence* a_anim,
		const int a_jointId, const float a_time);

	static void GetJointVelocity_RootRelative(FVector& o_jointVelocity, const UAnimSequence* a_anim, 
		const int a_jointId, const float a_time, const float a_poseInterval);

	static void GetJointTransform_RootRelative(FTransform& OutTransform, const UAnimSequence* AnimSequence,
		const TArray<FName>& BonesToRoot, const float Time);

	static void GetJointVelocity_RootRelative(FVector& OutVelocity, const UAnimSequence* AniMSequence,
		const TArray<FName>& BonesToRoot, const float Time, const float PoseInterval);

	static bool GetDoNotUseTag(const UAnimSequence* a_anim, const float a_time, const float a_poseInterval);
};
