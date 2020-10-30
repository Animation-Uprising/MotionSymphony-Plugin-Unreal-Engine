// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EPastTrajectoryMode.generated.h"

UENUM(BlueprintType)
enum class EPastTrajectoryMode : uint8
{
	ActualHistory,
	CopyFromCurrentPose
};