// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/TrajectoryPoint.h"
#include "JointData.h"
#include "PoseMotionData.generated.h"

/** A data structure representing a single pose within an animation set. These poses are recorded at specific 
time intervals (usually 0.05s - 0.1s) and stored in a linear list. The list is searched continuously, either linearly or 
via an optimised method, for the best matching pose for the next frame. */
USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FPoseMotionData
{
	GENERATED_USTRUCT_BODY()

public:
	/** The Id of this pose in the pose database */
	UPROPERTY()
	int32 PoseId;

	/** The Id of the pose candidate set to search when searching from this pose*/
	UPROPERTY()
	int32 CandidateSetId = -1;

	/** The Id of the animation this pose relates to */
	UPROPERTY()
	int32 AnimId = 0;
	
	/** The time within the referenced animation that this pose relates to*/
	UPROPERTY()
	float Time = 0.0f;

	/** Id of the next pose in the animation database*/
	UPROPERTY()
	int32 NextPoseId;

	/** Id of the previous pose in the animation database*/
	UPROPERTY()
	int32 LastPoseId;

	/** A cost multiplier for this pose for the motion matching cost function */
	UPROPERTY()
	float Favour = 1.0f;

	/** Is this pose for a mirrored version of the aniamtion */
	UPROPERTY()
	bool bMirrored = false;

	/** If true this pose will not be searched or jumped to in the pre-process phase */
	UPROPERTY()
	bool bDoNotUse = false;

	/** Body velocity of the character at this pose*/
	UPROPERTY()
	FVector LocalVelocity;

	/** Rotational velocity (in degrees) around the Z axis (character turning) */
	UPROPERTY()
	float RotationalVelocity;

	/** A list of trajectory points (past and future) for the animation from this point as
	time 0 of the trajectory*/
	UPROPERTY()
	TArray<FTrajectoryPoint> Trajectory;

	/** Relative position and velocity of select joints in this pose*/
	UPROPERTY()
	TArray<FJointData> JointData;

	TBitArray<FDefaultBitArrayAllocator> Tags;

public:
	FPoseMotionData();
	FPoseMotionData(int32 InNumTrajPoints, int32 InNumJoints);
	FPoseMotionData(int32 InPoseId, int32 InAnimId, float InTime,
		float InFavour, bool bInDoNotUse, bool bInMirrored,
		float InRotationalVelocity, FVector InLocalVelocity);

	void Clear();

	FPoseMotionData& operator += (const FPoseMotionData& rhs);
	FPoseMotionData& operator /= (const float rhs);
	FPoseMotionData& operator *= (const float rhs);
};