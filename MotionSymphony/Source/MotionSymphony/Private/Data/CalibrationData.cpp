// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "Data/CalibrationData.h"

FCalibrationData::FCalibrationData()
	: PoseWeight_Position(3.0f),
	PoseWeight_Velocity(1.0f),
	PoseWeight_ResVelocity(0.1f),
	BodyWeight_Velocity(5.0f),
	BodyWeight_RotationalVelocity(3.0),
	TrajectoryWeight_Position(12.0f),
	TrajectoryWeight_Rotation(6.0f)
{
}

void FCalibrationData::SetCalibration(float PoseWeightPos, float PoseWeightVel, float PoseWeightRes,
	float BodyWeightVel, float BodyWeightRotVel, float TrajectoryWeightPos, float TrajectoryWeightRot)
{
	PoseWeight_Position = PoseWeightPos;
	PoseWeight_Velocity = PoseWeightVel;
	PoseWeight_ResVelocity = PoseWeightRes;
	BodyWeight_Velocity = BodyWeightVel;
	BodyWeight_RotationalVelocity = BodyWeightRotVel;
	TrajectoryWeight_Position = TrajectoryWeightPos;
	TrajectoryWeight_Rotation = TrajectoryWeightRot;
}