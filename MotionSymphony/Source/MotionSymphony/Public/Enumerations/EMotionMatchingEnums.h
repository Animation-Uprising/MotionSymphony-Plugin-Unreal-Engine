// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EMotionMatchingEnums.generated.h"

/** An enumeration for all modes or 'states' that the Motion Matching NOde can be in.*/
UENUM(BlueprintType)
enum class EMotionMatchingMode : uint8
{
	MotionMatching,
	DistanceMatching,
	Action
};

/** An enumeration describing the pose search methods.*/
UENUM(BlueprintType)
enum class EPoseMatchMethod : uint8
{
	Optimized,
	Linear,
};

/** An enumeration for the blend status of any given motion matching animation channel */
UENUM(BlueprintType)
enum class EBlendStatus : uint8
{
	Inactive,
	Chosen,
	Dominant,
	Decay
};

/** An enumeration for the different methods of transitioning between animations in motion matching */
UENUM(BlueprintType)
enum class ETransitionMethod : uint8
{
	None,
	Inertialization,
	Blend
};

/** An enumeration for the different methods of determining past trajectory for motion matching */
UENUM(BlueprintType)
enum class EPastTrajectoryMode : uint8
{
	ActualHistory,
	CopyFromCurrentPose
};

/** An enumeration defining the different types of animation assets that can be used as motion matching source animations */
UENUM(BlueprintType)
enum class EMotionAnimAssetType : uint8
{
	None,
	Sequence,
	BlendSpace,
	Composite
};

/** An enumeration defining the different behaviour modes for trajectory generators */
UENUM(BlueprintType)
enum class ETrajectoryMoveMode : uint8
{
	Standard,
	Strafe
};

/** An enumeration defining the control modes for the trajectory generator. i.e. whether it is player controlled or AI controlled*/
UENUM(BlueprintType)
enum class ETrajectoryControlMode : uint8
{
	PlayerControlled,
	AIControlled
};