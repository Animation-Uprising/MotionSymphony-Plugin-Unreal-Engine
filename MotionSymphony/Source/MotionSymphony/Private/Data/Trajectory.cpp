// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "Data/Trajectory.h"
#include "MotionSymphony.h"

FTrajectory::FTrajectory()
{
}

FTrajectory::~FTrajectory()
{
	TrajectoryPoints.Empty();
}

void FTrajectory::Initialize(int a_trajCount)
{
	TrajectoryPoints.Empty(a_trajCount);
	
	for (int i = 0; i < a_trajCount; ++i)
	{
		TrajectoryPoints.Emplace(FTrajectoryPoint());
	}
}

void FTrajectory::Clear()
{
	TrajectoryPoints.Empty();
}

void FTrajectory::MakeRelativeTo(FTransform a_transform)
{
	float rot = a_transform.GetRotation().Euler().Z + 90.0f; //-90.0f is to make up for the 90 degree offset of characters in UE4

	for (int i = 0; i < TrajectoryPoints.Num(); ++i)
	{
		FTrajectoryPoint& point = TrajectoryPoints[i];

		FVector newPos = a_transform.InverseTransformVector(point.Position);
		float newRot = point.RotationZ - rot;

		//Wrap rotation within range -180 to 180
		float rotRemain = (newRot + 180.0f) / 360.0f;
		rotRemain -= FMath::FloorToFloat(rotRemain);
		newRot = rotRemain * 360.0f - 180.0f;

		TrajectoryPoints[i] = FTrajectoryPoint(newPos, newRot);
	}
}