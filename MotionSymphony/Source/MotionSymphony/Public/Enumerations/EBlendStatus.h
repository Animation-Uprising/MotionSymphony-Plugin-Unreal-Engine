// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EBlendStatus.generated.h"

UENUM(BlueprintType)
enum class EBlendStatus : uint8
{
	Inactive,
	Chosen,
	Dominant,
	Decay
};