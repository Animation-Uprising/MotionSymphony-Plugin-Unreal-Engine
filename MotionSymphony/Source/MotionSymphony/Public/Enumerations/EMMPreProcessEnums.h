// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EMMPreProcessEnums.generated.h"

UENUM(BlueprintType)
enum class ETrajectoryPreProcessMethod : uint8
{
	IgnoreEdges,
	Extrapolate,
	Animation
};