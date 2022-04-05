// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "EMMPreProcessEnums.h"
#include "PoseMotionData.h"
#include "Math/UnrealMathUtility.h"

struct FCompactPose;
class UMirroringProfile;
class USkeletalMeshComponent;
struct FAnimMirroringData;
struct FCalibrationData;

class MOTIONSYMPHONY_API FMotionMatchingUtils
{
public:
	static void LerpPose(FPoseMotionData& OutLerpPose, FPoseMotionData& From, FPoseMotionData& To, float Progress);

	static void LerpPoseTrajectory(FPoseMotionData& LerpPose, FPoseMotionData& From, FPoseMotionData& To, float Progress);

	static float WrapAnimationTime(float Time, float Length);

	static float ComputeTrajectoryCost(const TArray<FTrajectoryPoint>& Current, 
		const TArray<FTrajectoryPoint>& Candidate, const float PosWeight, const float RotWeight);

	static float ComputeTrajectoryCost(const TArray<FTrajectoryPoint>& Current,
									   const TArray<FTrajectoryPoint>& Candidate, const FCalibrationData& Calibration);

	static float ComputePoseCost(const TArray<FJointData>& Current,
		const TArray<FJointData>& Candidate, const float PosWeight,
		const float VelWeight);

	static float ComputePoseCost(const TArray<FJointData>& Current,
		const TArray<FJointData>& Candidate, const FCalibrationData& Calibration);

	static inline float LerpAngle(float AngleA, float AngleB, float Progress)
	{
		const float Max = PI * 2.0f;
		const float DeltaAngle = fmod((AngleB - AngleA), Max);

		return AngleA + (fmod(2.0f * DeltaAngle, Max) - DeltaAngle) * Progress;
	}

	static void MirrorPose(FCompactPose& OutPose, UMirroringProfile* InMirroringProfile, USkeletalMeshComponent* SkelMesh);
	static void MirrorPose(FCompactPose& OutPose, UMirroringProfile* InMirroringProfile, 
		FAnimMirroringData& MirrorData, USkeletalMeshComponent* SkelMesh);

	static float SignedAngle(FVector From, FVector To, FVector Axis);

	static float GetFacingAngleOffset(EAllAxis CharacterForward);
};