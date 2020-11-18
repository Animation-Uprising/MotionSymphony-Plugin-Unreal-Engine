// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/TrajectoryPoint.h"
#include "JointData.h"
//#include "Containers/BitArray.h"
#include "PoseMotionData.generated.h"

USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FPoseMotionData
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY()
	int32 PoseId;

	UPROPERTY()
	int32 CandidateSetId = -1;

	UPROPERTY()
	int32 AnimId = 0;
	
	UPROPERTY()
	float Time = 0.0f;

	UPROPERTY()
	int32 NextPoseId;

	UPROPERTY()
	int32 LastPoseId;

	UPROPERTY()
	float Favour = 1.0f;

	UPROPERTY()
	bool DoNotUse = false;

	UPROPERTY()
	FVector LocalVelocity;

	UPROPERTY()
	TArray<FTrajectoryPoint> Trajectory;

	UPROPERTY()
	TArray<FJointData> JointData;

	TBitArray<FDefaultBitArrayAllocator> Tags;

public:
	FPoseMotionData();
	FPoseMotionData(int32 InNumTrajPoints, int32 InNumJoints);
	FPoseMotionData(int32 InPoseId, int32 InAnimId, float InTime, 
		float InFavour, bool InDoNotUse, FVector InLocalVelocity);
	~FPoseMotionData();

	void Clear();

	FPoseMotionData& operator += (const FPoseMotionData& rhs);
	FPoseMotionData& operator /= (const float rhs);
	FPoseMotionData& operator *= (const float rhs);
};