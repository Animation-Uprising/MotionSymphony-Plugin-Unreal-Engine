// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "PoseMotionData.h"
#include "MotionSymphony.h"

FPoseMotionData::FPoseMotionData()
	: PoseId(0),
	AnimType(EMotionAnimAssetType::None),
	AnimId(0),
	CandidateSetId(-1),
	Time(0.0f),
	BlendSpacePosition(FVector2D(0.0f)),
	NextPoseId(0),
	LastPoseId(0),
	Favour(1.0f),
	bMirrored(false),
	bDoNotUse(false),
	LocalVelocity(FVector(0.0f)),
	RotationalVelocity(0.0f),
	Tags(0)
{ 	
}

FPoseMotionData::FPoseMotionData(int32 InNumTrajPoints, int32 InNumJoints)
	: PoseId(0), 
	  AnimType(EMotionAnimAssetType::None),
	  AnimId(0),
	  CandidateSetId(-1),
	  Time(0.0f),
	  BlendSpacePosition(FVector2D(0.0f)),
	  NextPoseId(0), 
	  LastPoseId(0),
	  Favour(1.0f), 
	  bMirrored(false),
	  bDoNotUse(false), 
	  LocalVelocity(FVector(0.0f)),
	  RotationalVelocity(0.0f),
	  Tags(0)
{
	Trajectory.Empty(InNumTrajPoints);
	JointData.Empty(InNumJoints);

	for (int32 i = 0; i < InNumTrajPoints; ++i)
	{
		Trajectory.Add(FTrajectoryPoint());
	}

	for (int32 i = 0; i < InNumJoints; ++i)
	{
		JointData.Add(FJointData());
	}
}

FPoseMotionData::FPoseMotionData(int32 InPoseId, EMotionAnimAssetType InAnimType, int32 InAnimId, 
	float InTime, float InCostMultiplier, bool bInDoNotUse, bool bInMirrored,
	float InRotationalVelocity, FVector InLocalVelocity, uint64 InTags)
	: PoseId(InPoseId), 
	  AnimType(InAnimType),
	  AnimId(InAnimId),
	  CandidateSetId(-1),
	  Time(InTime), 
	  BlendSpacePosition(FVector2D(0.0f)),
	  NextPoseId(InPoseId + 1),
	  LastPoseId(FMath::Clamp(InPoseId - 1, 0, InPoseId)), 
	  Favour(InCostMultiplier),
	  bMirrored(bInMirrored),
	  bDoNotUse(bInDoNotUse), 
	  LocalVelocity(InLocalVelocity),
	  RotationalVelocity(InRotationalVelocity),
	  Tags(InTags)
{
}

void FPoseMotionData::Clear()
{
	PoseId = -1;
	AnimType = EMotionAnimAssetType::None;
	CandidateSetId = -1;
	AnimId = -1;
	Time = 0;
	BlendSpacePosition = FVector2D(0.0f);
	NextPoseId = -1;
	LastPoseId = -1;
	Favour = 1.0f;
	bMirrored = false;
	bDoNotUse = false;
	LocalVelocity = FVector::ZeroVector;
	RotationalVelocity = 0.0f;
	Trajectory.Empty(6);
	JointData.Empty(6);
	Tags = 0;
}

FPoseMotionData& FPoseMotionData::operator+=(const FPoseMotionData& rhs)
{
	LocalVelocity += rhs.LocalVelocity;
	RotationalVelocity += rhs.RotationalVelocity;

	for (int32 i = 0; i < Trajectory.Num() && i < rhs.Trajectory.Num(); ++i)
	{
		Trajectory[i] += rhs.Trajectory[i];
	}

	for (int32 i = 0; i < JointData.Num() && i < rhs.JointData.Num(); ++i)
	{
		JointData[i] += rhs.JointData[i];
	}

	return *this;
}

FPoseMotionData& FPoseMotionData::operator/=(const float rhs)
{
	LocalVelocity /= rhs;
	RotationalVelocity /= rhs;
	
	for (int32 i = 0; i < Trajectory.Num(); ++i)
	{
		Trajectory[i] /= rhs;
	}

	for (int32 i = 0; i < JointData.Num(); ++i)
	{
		JointData[i] /= rhs;
	}

	return *this;
}

FPoseMotionData& FPoseMotionData::operator*=(const float rhs)
{
	LocalVelocity *= rhs;
	RotationalVelocity *= rhs;

	for (int32 i = 0; i < Trajectory.Num(); ++i)
	{
		Trajectory[i] *= rhs;
	}

	for (int32 i = 0; i < JointData.Num(); ++i)
	{
		JointData[i] *= rhs;
	}

	return *this;
}