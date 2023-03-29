// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "MotionMatchingUtil/MotionMatchingUtils.h"
#include "CustomAssets/MirroringProfile.h"
#include "CustomAssets/MotionCalibration.h"
#include "Data/AnimMirroringData.h"
#include "Data/CalibrationData.h"
#include "Components/SkeletalMeshComponent.h"
#include "BonePose.h"


void FMotionMatchingUtils::LerpPose(FPoseMotionData& OutLerpPose,
	FPoseMotionData& From, FPoseMotionData& To, float Progress)
{
	if (Progress < 0.5f)
	{
		OutLerpPose.AnimId = From.AnimId;
		OutLerpPose.CandidateSetId = From.CandidateSetId;
		OutLerpPose.bDoNotUse = From.bDoNotUse;
		OutLerpPose.Favour = From.Favour;
		OutLerpPose.PoseId = From.PoseId;
		OutLerpPose.BlendSpacePosition = From.BlendSpacePosition;
	}
	else
	{
		OutLerpPose.AnimId = To.AnimId;
		OutLerpPose.CandidateSetId = To.CandidateSetId;
		OutLerpPose.bDoNotUse = To.bDoNotUse;
		OutLerpPose.Favour = To.Favour;
		OutLerpPose.PoseId = To.PoseId;
		OutLerpPose.BlendSpacePosition = To.BlendSpacePosition;
	}

	OutLerpPose.LastPoseId = From.PoseId;
	OutLerpPose.NextPoseId = To.PoseId;
	OutLerpPose.Time = FMath::Lerp(From.Time, To.Time, Progress);
	OutLerpPose.LocalVelocity = FMath::Lerp(From.LocalVelocity, To.LocalVelocity, Progress);
	OutLerpPose.RotationalVelocity = FMath::Lerp(From.RotationalVelocity, To.RotationalVelocity, Progress);
	
	for (int32 i = 0; i < From.JointData.Num(); ++i)
	{
		FJointData::Lerp(OutLerpPose.JointData[i], From.JointData[i], To.JointData[i], Progress);
	}

	for (int32 i = 0; i < From.Trajectory.Num(); ++i)
	{
		FTrajectoryPoint::Lerp(OutLerpPose.Trajectory[i], From.Trajectory[i], To.Trajectory[i], Progress);
	}
}

void FMotionMatchingUtils::LerpPoseTrajectory(FPoseMotionData & OutLerpPose, FPoseMotionData & From, FPoseMotionData & To, float Progress)
{
	if (Progress < 0.5f)
	{
		OutLerpPose.AnimId = From.AnimId;
		OutLerpPose.CandidateSetId = From.CandidateSetId;
		OutLerpPose.bDoNotUse = From.bDoNotUse;
		OutLerpPose.Favour = From.Favour;
		OutLerpPose.PoseId = From.PoseId;
	}
	else
	{
		OutLerpPose.AnimId = To.AnimId;
		OutLerpPose.CandidateSetId = To.CandidateSetId;
		OutLerpPose.bDoNotUse = To.bDoNotUse;
		OutLerpPose.Favour = To.Favour;
		OutLerpPose.PoseId = To.PoseId;
	}

	OutLerpPose.LastPoseId = From.PoseId;
	OutLerpPose.NextPoseId = To.PoseId;
	OutLerpPose.Time = FMath::Lerp(From.Time, To.Time, Progress);
	OutLerpPose.LocalVelocity = FMath::Lerp(From.LocalVelocity, To.LocalVelocity, Progress);
	OutLerpPose.RotationalVelocity = FMath::Lerp(From.RotationalVelocity, To.RotationalVelocity, Progress);

	for (int32 i = 0; i < From.Trajectory.Num(); ++i)
	{
		FTrajectoryPoint::Lerp(OutLerpPose.Trajectory[i], From.Trajectory[i], To.Trajectory[i], Progress);
	}
}

float FMotionMatchingUtils::ComputeTrajectoryCost(const TArray<FTrajectoryPoint>& Current,
	const TArray<FTrajectoryPoint>& Candidate, const float PosWeight, const float rotWeight)
{
	float Cost = 0.0f;

	for (int32 i = 0; i < Current.Num(); ++i)
	{
		const FTrajectoryPoint& CurrentPoint = Current[i];
		const FTrajectoryPoint& CandidatePoint = Candidate[i];

		//Cost of distance between trajectory points
		Cost += FVector::DistSquared(CandidatePoint.Position, CurrentPoint.Position) * PosWeight;

		//Cost of angle between trajectory point facings
		Cost += FMath::Abs(FMath::FindDeltaAngleDegrees(CandidatePoint.RotationZ, CurrentPoint.RotationZ)) * rotWeight;
	}

	return Cost;
}

float FMotionMatchingUtils::ComputeTrajectoryCost(const TArray<FTrajectoryPoint>& Current, const TArray<FTrajectoryPoint>& Candidate, const FCalibrationData& Calibration)
{
	float Cost = 0.0f;

	const int32 TrajectoryIterations = FMath::Min(Current.Num(), Calibration.TrajectoryWeights.Num());
	for (int32 i = 0; i < TrajectoryIterations; ++i)
	{
		const FTrajectoryWeightSet& WeightSet = Calibration.TrajectoryWeights[i];

		const FTrajectoryPoint& CurrentPoint = Current[i];
		const FTrajectoryPoint& CandidatePoint = Candidate[i];

		//Cost of distance between trajectory points
		Cost += FVector::DistSquared(CandidatePoint.Position, CurrentPoint.Position) * WeightSet.Weight_Pos;

		//Cost of angle between trajectory point facings
		Cost += FMath::Abs(FMath::FindDeltaAngleDegrees(CandidatePoint.RotationZ, CurrentPoint.RotationZ)) * WeightSet.Weight_Facing;
	}

	return Cost;
}

float FMotionMatchingUtils::ComputePoseCost(const TArray<FJointData>& Current, const TArray<FJointData>& Candidate,
	const float PosWeight, const float VelWeight)
{
	float Cost = 0.0f;

	for (int32 i = 0; i < Current.Num(); ++i)
	{
		const FJointData& CurrentJoint = Current[i];
		const FJointData& CandidateJoint = Candidate[i];

		//Cost of velocity comparison
		Cost += FVector::DistSquared(CurrentJoint.Velocity, CandidateJoint.Velocity) * VelWeight;

		//Cost of distance between joints
		Cost += FVector::DistSquared(CurrentJoint.Position, CandidateJoint.Position) * PosWeight;
	}

	return Cost;
}

float FMotionMatchingUtils::ComputePoseCost(const TArray<FJointData>& Current, const TArray<FJointData>& Candidate, const FCalibrationData& Calibration)
{
	float Cost = 0.0f;

	for (int32 i = 0; i < Current.Num(); ++i)
	{
		const FJointWeightSet& WeightSet = Calibration.PoseJointWeights[i];
		const FJointData& CurrentJoint = Current[i];
		const FJointData& CandidateJoint = Candidate[i];

		//Cost of velocity comparison
		Cost += FVector::DistSquared(CurrentJoint.Velocity, CandidateJoint.Velocity) * WeightSet.Weight_Vel;

		//Cost of distance between joints
		Cost += FVector::DistSquared(CurrentJoint.Position, CandidateJoint.Position) * WeightSet.Weight_Pos;
	}

	return Cost;
}

void FMotionMatchingUtils::MirrorPose(FCompactPose& OutPose, UMirroringProfile* InMirroringProfile, USkeletalMeshComponent* SkelMesh)
{
	if(!SkelMesh || !InMirroringProfile)
	{
		return;
	}

	TArray<FBoneMirrorPair>& MirrorPairs = InMirroringProfile->MirrorPairs;

	const int32 MirrorCount = MirrorPairs.Num();

	if (MirrorCount == 0)
	{
		return;
	}

	for (FBoneMirrorPair& MirrorPair : MirrorPairs)
	{
		const int32 BoneIndex = SkelMesh->GetBoneIndex(FName(MirrorPair.BoneName)); //Todo: Change it so that its an FName natively
		FCompactPoseBoneIndex CompactBoneIndex(BoneIndex);

		if (!OutPose.IsValidIndex(CompactBoneIndex))
		{
			continue;
		}

		FTransform BoneTransform = OutPose[CompactBoneIndex];

		BoneTransform.Mirror(MirrorPair.MirrorAxis, MirrorPair.FlipAxis);

		if (MirrorPair.RotationOffset != FRotator::ZeroRotator)
		{
			FRotator BoneRotation = BoneTransform.Rotator();

			BoneRotation.Yaw += MirrorPair.RotationOffset.Yaw;
			BoneRotation.Roll += MirrorPair.RotationOffset.Roll;
			BoneRotation.Pitch += MirrorPair.RotationOffset.Pitch;

			BoneTransform.SetRotation(FQuat(BoneRotation));
		}

		if (MirrorPair.bHasMirrorBone)
		{
			const int32 MirrorBoneIndex = SkelMesh->GetBoneIndex(FName(MirrorPair.MirrorBoneName));
			FCompactPoseBoneIndex CompactMirrorBoneIndex(MirrorBoneIndex);

			if (!OutPose.IsValidIndex(CompactMirrorBoneIndex))
			{
				continue;
			}

			FTransform MirrorBoneTransform = OutPose[CompactMirrorBoneIndex];

			MirrorBoneTransform.Mirror(MirrorPair.MirrorAxis, MirrorPair.FlipAxis);

			if (MirrorPair.RotationOffset != FRotator::ZeroRotator)
			{
				FRotator MirrorBoneRotation = MirrorBoneTransform.Rotator();

				MirrorBoneRotation.Yaw += MirrorPair.RotationOffset.Yaw;
				MirrorBoneRotation.Roll += MirrorPair.RotationOffset.Roll;
				MirrorBoneRotation.Pitch += MirrorPair.RotationOffset.Pitch;

				MirrorBoneTransform.SetRotation(FQuat(MirrorBoneRotation));
			}

			BoneTransform.SetScale3D(BoneTransform.GetScale3D().GetAbs());
			MirrorBoneTransform.SetScale3D(MirrorBoneTransform.GetScale3D().GetAbs());

			if (MirrorPair.bMirrorPosition)
			{
				OutPose[CompactBoneIndex] = MirrorBoneTransform;
				OutPose[CompactBoneIndex].NormalizeRotation();

				OutPose[CompactMirrorBoneIndex] = BoneTransform;
				OutPose[CompactMirrorBoneIndex].NormalizeRotation();
			}
			else
			{
				FVector BonePosition = BoneTransform.GetLocation();
				FVector BoneMirrorPosition = MirrorBoneTransform.GetLocation();

				BoneTransform.SetLocation(BoneMirrorPosition);
				MirrorBoneTransform.SetLocation(BonePosition);

				OutPose[CompactBoneIndex] = MirrorBoneTransform;
				OutPose[CompactBoneIndex].NormalizeRotation();

				OutPose[CompactMirrorBoneIndex] = BoneTransform;
				OutPose[CompactMirrorBoneIndex].NormalizeRotation();
			}
		}
		else
		{
			BoneTransform.SetScale3D(BoneTransform.GetScale3D().GetAbs());

			OutPose[CompactBoneIndex] = BoneTransform;
		}
	}
}

void FMotionMatchingUtils::MirrorPose(FCompactPose& OutPose, UMirroringProfile* InMirroringProfile, 
	FAnimMirroringData& MirrorData, USkeletalMeshComponent* SkelMesh)
{
	if (!SkelMesh || !InMirroringProfile)
	{
		return;
	}

	TArray<FBoneMirrorPair>& MirrorPairs = InMirroringProfile->MirrorPairs;

	const int32 MirrorCount = MirrorPairs.Num();

	if (MirrorCount == 0)
	{
		return;
	}

	for (int32 i = 0; i < MirrorCount; ++i)
	{
		FBoneMirrorPair& MirrorPair = MirrorPairs[i];
		const FIndexedMirrorPair& IndexPair = MirrorData.IndexedMirrorPairs[i];

		FCompactPoseBoneIndex CompactBoneIndex(IndexPair.BoneIndex);

		if (!OutPose.IsValidIndex(CompactBoneIndex))
		{
			continue;
		}

		FTransform BoneTransform = OutPose[CompactBoneIndex];

		BoneTransform.Mirror(MirrorPair.MirrorAxis, MirrorPair.FlipAxis);

		if (MirrorPair.RotationOffset != FRotator::ZeroRotator)
		{
			FRotator BoneRotation = BoneTransform.Rotator();

			BoneRotation.Yaw += MirrorPair.RotationOffset.Yaw;
			BoneRotation.Roll += MirrorPair.RotationOffset.Roll;
			BoneRotation.Pitch += MirrorPair.RotationOffset.Pitch;

			BoneTransform.SetRotation(FQuat(BoneRotation));
		}

		if (MirrorPair.bHasMirrorBone)
		{
			FCompactPoseBoneIndex CompactMirrorBoneIndex(IndexPair.MirrorBoneIndex);

			if (!OutPose.IsValidIndex(CompactMirrorBoneIndex))
			{
				continue;
			}

			FTransform MirrorBoneTransform = OutPose[CompactMirrorBoneIndex];

			MirrorBoneTransform.Mirror(MirrorPair.MirrorAxis, MirrorPair.FlipAxis);

			if (MirrorPair.RotationOffset != FRotator::ZeroRotator)
			{
				FRotator MirrorBoneRotation = MirrorBoneTransform.Rotator();

				MirrorBoneRotation.Yaw += MirrorPair.RotationOffset.Yaw;
				MirrorBoneRotation.Roll += MirrorPair.RotationOffset.Roll;
				MirrorBoneRotation.Pitch += MirrorPair.RotationOffset.Pitch;

				MirrorBoneTransform.SetRotation(FQuat(MirrorBoneRotation));
			}

			BoneTransform.SetScale3D(BoneTransform.GetScale3D().GetAbs());
			MirrorBoneTransform.SetScale3D(MirrorBoneTransform.GetScale3D().GetAbs());

			if (MirrorPair.bMirrorPosition)
			{
				OutPose[CompactBoneIndex] = MirrorBoneTransform;
				OutPose[CompactBoneIndex].NormalizeRotation();

				OutPose[CompactMirrorBoneIndex] = BoneTransform;
				OutPose[CompactMirrorBoneIndex].NormalizeRotation();
			}
			else
			{
				FVector BonePosition = BoneTransform.GetLocation();
				FVector BoneMirrorPosition = MirrorBoneTransform.GetLocation();

				BoneTransform.SetLocation(BoneMirrorPosition);
				MirrorBoneTransform.SetLocation(BonePosition);

				OutPose[CompactBoneIndex] = MirrorBoneTransform;
				OutPose[CompactBoneIndex].NormalizeRotation();

				OutPose[CompactMirrorBoneIndex] = BoneTransform;
				OutPose[CompactMirrorBoneIndex].NormalizeRotation();
			}
		}
		else
		{
			BoneTransform.SetScale3D(BoneTransform.GetScale3D().GetAbs());

			OutPose[CompactBoneIndex] = BoneTransform;
		}
	}
}

float FMotionMatchingUtils::SignedAngle(FVector From, FVector To, FVector Axis)
{
	const float UnsignedAngle = FMath::Acos(FVector::DotProduct(From, To));

	const float CrossX = From.Y * To.Z - From.Z * To.Y;
	const float CrossY = From.Z * To.X - From.X * To.Z;
	const float CrossZ = From.X * To.Y - From.Y * To.X;

	const float Sign = FMath::Sign(Axis.X * CrossX + Axis.Y * CrossY + Axis.Z * CrossZ);

	return UnsignedAngle * Sign;
}

float FMotionMatchingUtils::GetFacingAngleOffset(EAllAxis CharacterForward)
{
	switch(CharacterForward)
	{
		case EAllAxis::X: return 0.0f;
		case EAllAxis::Y: return 90.0f;
		case EAllAxis::NegX: return 180.0f;
		case EAllAxis::NegY: return -90.0f;
		default: return 0.0f;
	}
}


float FMotionMatchingUtils::WrapAnimationTime(float time, float length)
{
	if (time < 0.0f)
	{
		const int32 wholeNumbers = FMath::FloorToInt(FMath::Abs(time) / length);
		time = length + (time + (wholeNumbers * length));
	}
	else if (time > length)
	{
		const int32 wholeNumbers = FMath::FloorToInt(time / length);
		time = time - (wholeNumbers * length);
	}

	return time;
}
