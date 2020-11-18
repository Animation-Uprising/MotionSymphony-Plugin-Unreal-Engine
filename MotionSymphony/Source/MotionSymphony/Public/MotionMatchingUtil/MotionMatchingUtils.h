// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "PoseMotionData.h"
#include "Math/UnrealMathUtility.h"

class MOTIONSYMPHONY_API FMotionMatchingUtils
{
public:
	static void LerpPose(FPoseMotionData& OutLerpPose, FPoseMotionData& From, FPoseMotionData& To, float Progress);

	static void LerpPoseTrajectory(FPoseMotionData& LerpPose, FPoseMotionData& From, FPoseMotionData& To, float Progress);

	static float WrapAnimationTime(float Time, float Length);

	static float ComputeTrajectoryCost(const TArray<FTrajectoryPoint>& Current, 
		const TArray<FTrajectoryPoint>& Candidate, const float PosWeight, const float RotWeight);

	static float ComputePoseCost_SD(const TArray<FJointData>& Current,
		const TArray<FJointData>& Candidate, const float PosWeight,
		const float VelWeight);

	static float ComputePoseCost_HD(const TArray<FJointData>& Current, 
		const TArray<FJointData>& Candidate, const float PosWeight, 
		const float VelWeight, const float ResultVelWeight, const float PoseInterval);

	static inline float LerpAngle(float AngleA, float AngleB, float Progress)
	{
		float Max = PI * 2.0f;
		float DeltaAngle = fmod((AngleB - AngleA), Max);

		return AngleA + (fmod(2.0f * DeltaAngle, Max) - DeltaAngle) * Progress;
	}
};