// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/TrajectoryPoint.h"
#include "JointData.h"
#include "Enumerations/EMotionMatchingEnums.h"
#include "Containers/BitArray.h"
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
	UPROPERTY(BlueprintReadOnly)
	int32 PoseId;

	/** The type of animation this pose uses */
	UPROPERTY(BlueprintReadOnly)
	EMotionAnimAssetType AnimType;

	/** The Id of the animation this pose relates to */
	UPROPERTY(BlueprintReadOnly)
	int32 AnimId = 0;

	/** The Id of the pose candidate set to search when searching from this pose*/
	UPROPERTY(BlueprintReadOnly)
	int32 CandidateSetId = -1;
	
	/** The time within the referenced animation that this pose relates to*/
	UPROPERTY(BlueprintReadOnly)
	float Time = 0.0f;

	/** The position within a blend space that the pose exists*/
	UPROPERTY(BlueprintReadOnly)
	FVector2D BlendSpacePosition = FVector2D::ZeroVector;

	/** Id of the next pose in the animation database*/
	UPROPERTY(BlueprintReadOnly)
	int32 NextPoseId;

	/** Id of the previous pose in the animation database*/
	UPROPERTY(BlueprintReadOnly)
	int32 LastPoseId;

	/** A cost multiplier for this pose for the motion matching cost function */
	UPROPERTY(BlueprintReadWrite)
	float Favour = 1.0f;

	/** Is this pose for a mirrored version of the aniamtion */
	UPROPERTY(BlueprintReadOnly)
	bool bMirrored = false;

	/** If true this pose will not be searched or jumped to in the pre-process phase */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PoseMotionData")
	bool bDoNotUse = false;

	/** Body velocity of the character at this pose*/
	UPROPERTY(BlueprintReadWrite)
	FVector LocalVelocity;

	/** Rotational velocity (in degrees) around the Z axis (character turning) */
	UPROPERTY(BlueprintReadWrite)
	float RotationalVelocity;

	/** A list of trajectory points (past and future) for the animation from this point as
	time 0 of the trajectory*/
	UPROPERTY(BlueprintReadWrite)
	TArray<FTrajectoryPoint> Trajectory;

	/** Relative position and velocity of select joints in this pose*/
	UPROPERTY(BlueprintReadWrite)
	TArray<FJointData> JointData;

	UPROPERTY()
	uint64 Tags;

public:
	FPoseMotionData();
	FPoseMotionData(int32 InNumTrajPoints, int32 InNumJoints);
	FPoseMotionData(int32 InPoseId, EMotionAnimAssetType InAnimType,
		int32 InAnimId, float InTime, float InCostMultiplier, bool bInDoNotUse, 
		bool bInMirrored, float InRotationalVelocity, FVector InLocalVelocity,
		uint64 InTags);

	void Clear();

	FPoseMotionData& operator += (const FPoseMotionData& rhs);
	FPoseMotionData& operator /= (const float rhs);
	FPoseMotionData& operator *= (const float rhs);
};