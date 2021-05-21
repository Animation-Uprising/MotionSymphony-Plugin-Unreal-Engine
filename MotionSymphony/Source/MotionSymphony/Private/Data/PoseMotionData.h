// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/TrajectoryPoint.h"
#include "JointData.h"
#include "Data/MotionTraitField.h"
#include "Enumerations/EMotionMatchingEnums.h"
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
	UPROPERTY(BlueprintReadOnly, Category = "MotionSymphony|Pose")
	int32 PoseId;

	/** The type of animation this pose uses */
	UPROPERTY(BlueprintReadOnly, Category = "MotionSymphony|Pose")
	EMotionAnimAssetType AnimType;

	/** The Id of the animation this pose relates to */
	UPROPERTY(BlueprintReadOnly, Category = "MotionSymphony|Pose")
	int32 AnimId = 0;

	/** The Id of the pose candidate set to search when searching from this pose*/
	UPROPERTY(BlueprintReadOnly, Category = "MotionSymphony|Pose")
	int32 CandidateSetId = -1;
	
	/** The time within the referenced animation that this pose relates to*/
	UPROPERTY(BlueprintReadOnly, Category = "MotionSymphony|Pose")
	float Time = 0.0f;

	/** The position within a blend space that the pose exists*/
	UPROPERTY(BlueprintReadOnly, Category = "MotionSymphony|Pose")
	FVector2D BlendSpacePosition = FVector2D::ZeroVector;

	/** Id of the next pose in the animation database*/
	UPROPERTY(BlueprintReadOnly, Category = "MotionSymphony|Pose")
	int32 NextPoseId;

	/** Id of the previous pose in the animation database*/
	UPROPERTY(BlueprintReadOnly, Category = "MotionSymphony|Pose")
	int32 LastPoseId;

	/** Is this pose for a mirrored version of the aniamtion */
	UPROPERTY(BlueprintReadOnly, Category = "MotionSymphony|Pose")
	bool bMirrored = false;

	/** If true this pose will not be searched or jumped to in the pre-process phase */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MotionSymphony|Pose")
	bool bDoNotUse = false;

	/** Body velocity of the character at this pose*/
	UPROPERTY(BlueprintReadWrite, Category = "MotionSymphony|Pose")
	FVector LocalVelocity;

	/** Rotational velocity (in degrees) around the Z axis (character turning) */
	UPROPERTY(BlueprintReadWrite, Category = "MotionSymphony|Pose")
	float RotationalVelocity;

	/** A list of trajectory points (past and future) for the animation from this point as
	time 0 of the trajectory*/
	UPROPERTY(BlueprintReadWrite, Category = "MotionSymphony|Pose")
	TArray<FTrajectoryPoint> Trajectory;

	/** Relative position and velocity of select joints in this pose*/
	UPROPERTY(BlueprintReadWrite, Category = "MotionSymphony|Pose")
	TArray<FJointData> JointData;

	/** A cost multiplier for this pose for the motion matching cost function */
	UPROPERTY(BlueprintReadWrite, Category = "MotionSymphony|Pose")
	float Favour = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "MotionSymphony|Pose")
	FMotionTraitField Traits;

public:
	FPoseMotionData();
	FPoseMotionData(int32 InNumTrajPoints, int32 InNumJoints);
	FPoseMotionData(int32 InPoseId, EMotionAnimAssetType InAnimType,
		int32 InAnimId, float InTime, float InCostMultiplier, bool bInDoNotUse, 
		bool bInMirrored, float InRotationalVelocity, FVector InLocalVelocity,
		const FMotionTraitField& InTraits);

	void Clear();

	FPoseMotionData& operator += (const FPoseMotionData& rhs);
	FPoseMotionData& operator /= (const float rhs);
	FPoseMotionData& operator *= (const float rhs);
};