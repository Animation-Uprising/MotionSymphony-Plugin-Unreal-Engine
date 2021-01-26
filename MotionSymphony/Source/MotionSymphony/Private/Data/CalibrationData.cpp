// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "Data/CalibrationData.h"
#include "CustomAssets/MotionDataAsset.h"
#include "CustomAssets/MotionMatchConfig.h"

FCalibrationData::FCalibrationData()
	: PoseTrajectoryBalance(0.5f),
	BodyWeight_Velocity(5.0f),
	BodyWeight_RotationalVelocity(3.0),
	PoseWeight_Position(3.0f),
	PoseWeight_Velocity(1.0f),
	PoseWeight_ResVelocity(0.1f),
	TrajectoryWeight_Position(12.0f),
	TrajectoryWeight_Rotation(6.0f)
{
	PoseJointWeights.Empty(5);
}

FCalibrationData::FCalibrationData(UMotionDataAsset* SourceMotionData)
	: PoseTrajectoryBalance(0.5f),
	BodyWeight_Velocity(5.0f),
	BodyWeight_RotationalVelocity(3.0),
	PoseWeight_Position(3.0f),
	PoseWeight_Velocity(1.0f),
	PoseWeight_ResVelocity(0.1f),
	TrajectoryWeight_Position(12.0f),
	TrajectoryWeight_Rotation(6.0f)
{
	if(SourceMotionData == nullptr)
	{
		PoseJointWeights.Empty(5);
		return;
	}

	PoseJointWeights.Empty(SourceMotionData->PoseJoints.Num() +1);

	for (int32 i = 0; i < SourceMotionData->PoseJoints.Num(); ++i)
	{
		PoseJointWeights.Emplace(FPoseWeightSet(3.0f, 1.0f));
	}
}

FCalibrationData::FCalibrationData(UMotionMatchConfig* SourceConfig)
	: PoseTrajectoryBalance(0.5f),
	BodyWeight_Velocity(5.0f),
	BodyWeight_RotationalVelocity(3.0),
	PoseWeight_Position(3.0f),
	PoseWeight_Velocity(1.0f),
	PoseWeight_ResVelocity(0.1f),
	TrajectoryWeight_Position(12.0f),
	TrajectoryWeight_Rotation(6.0f)
{
	if (SourceConfig == nullptr)
	{
		PoseJointWeights.Empty(5);
		return;
	}

	PoseJointWeights.Empty(SourceConfig->PoseJoints.Num() + 1);

	for (int32 i = 0; i < SourceConfig->PoseJoints.Num(); ++i)
	{
		PoseJointWeights.Emplace(FPoseWeightSet(3.0f, 1.0f));
	}
}

FCalibrationData::FCalibrationData(int32 PoseJointCount)
	: PoseTrajectoryBalance(0.5f),
	BodyWeight_Velocity(5.0f),
	BodyWeight_RotationalVelocity(3.0),
	PoseWeight_Position(3.0f),
	PoseWeight_Velocity(1.0f),
	PoseWeight_ResVelocity(0.1f),
	TrajectoryWeight_Position(12.0f),
	TrajectoryWeight_Rotation(6.0f)
{
	PoseJointCount = FMath::Clamp(PoseJointCount, 0, 100);

	PoseJointWeights.Empty(PoseJointCount + 1);

	for (int32 i = 0; i < PoseJointCount; ++i)
	{
		PoseJointWeights.Emplace(FPoseWeightSet(3.0f, 1.0f));
	}
}

void FCalibrationData::SetCalibration(float PoseTrajBalance, float PoseWeightPos, float PoseWeightVel, float PoseWeightRes,
	float BodyWeightVel, float BodyWeightRotVel, float TrajectoryWeightPos, float TrajectoryWeightRot)
{
	PoseTrajectoryBalance = PoseTrajBalance;
	PoseWeight_Position = PoseWeightPos;
	PoseWeight_Velocity = PoseWeightVel;
	PoseWeight_ResVelocity = PoseWeightRes;
	BodyWeight_Velocity = BodyWeightVel;
	BodyWeight_RotationalVelocity = BodyWeightRotVel;
	TrajectoryWeight_Position = TrajectoryWeightPos;
	TrajectoryWeight_Rotation = TrajectoryWeightRot;
}

FPoseWeightSet::FPoseWeightSet()
	: PositionWeight(3.0f),
	VelocityWeight(1.0f)
{
}

FPoseWeightSet::FPoseWeightSet(float PosWeight, float VelWeight)
	: PositionWeight(PosWeight),
	VelocityWeight(VelWeight)
{
}