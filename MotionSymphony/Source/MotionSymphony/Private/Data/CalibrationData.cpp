// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "Data/CalibrationData.h"

FCalibrationData::FCalibrationData()
	: PoseWeight_Position(3.0f),
	PoseWeight_Velocity(1.0f),
	PoseWeight_ResVelocity(0.1f),
	BodyWeight_Velocity(5.0f),
	TrajectoryWeight_Position(12.0f),
	TrajectoryWeight_Rotation(6.0f)
{

}

void FCalibrationData::SetCalibration(float a_poseWeightPos, float a_poseWeightVel, float a_poseWeightRes,
	float a_bodyWeightVel, float a_trajectoryWeightPos, float a_trajectoryWeightRot)
{
	PoseWeight_Position = a_poseWeightPos;
	PoseWeight_Velocity = a_poseWeightVel;
	PoseWeight_ResVelocity = a_poseWeightRes;
	BodyWeight_Velocity = a_bodyWeightVel;
	TrajectoryWeight_Position = a_trajectoryWeightPos;
	TrajectoryWeight_Rotation = a_trajectoryWeightRot;
}