// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EMotionMatchingEnums.generated.h"

UENUM(BlueprintType)
enum class EPoseMatchMethod : uint8
{
	LowQuality,
	HighQuality,

	LowQuality_Linear,
	HighQuality_Linear
};

UENUM(BlueprintType)
enum class EBlendStatus : uint8
{
	Inactive,
	Chosen,
	Dominant,
	Decay
};

UENUM(BlueprintType)
enum class ETransitionMethod : uint8
{
	None,
	Inertialization,
	Blend
};

UENUM(BlueprintType)
enum class EPastTrajectoryMode : uint8
{
	ActualHistory,
	CopyFromCurrentPose
};

UENUM(BlueprintType)
enum class EMotionAnimAssetType : uint8
{
	None,
	Sequence,
	BlendSpace
};