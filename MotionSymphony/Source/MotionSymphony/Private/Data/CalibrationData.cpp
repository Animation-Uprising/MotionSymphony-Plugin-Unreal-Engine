// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "CalibrationData.h"

FCalibrationData::FCalibrationData()
	: PoseWeight_Position(3.0f),
	PoseWeight_Velocity(1.0f),
	PoseWeight_ResVelocity(0.15f),
	BodyWeight_Velocity(3.0f),
	TrajectoryWeight_Position(5.0f),
	TrajectoryWeight_Rotation(0.05f)
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