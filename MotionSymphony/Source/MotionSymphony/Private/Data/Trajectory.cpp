// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "Trajectory.h"
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
	float rot = a_transform.GetRotation().Euler().Z;

	for (int i = 0; i < TrajectoryPoints.Num(); ++i)
	{
		FTrajectoryPoint& point = TrajectoryPoints[i];

		FVector newPos = a_transform.InverseTransformVector(point.Position);
		float newRot = point.RotationZ - rot;

		TrajectoryPoints[i] = FTrajectoryPoint(newPos, newRot);
	}
}