// Copyright 2020 Kenneth Claassen. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "EPoseMatchMethod.generated.h"

UENUM(BlueprintType)
enum class EPoseMatchMethod : uint8
{
	LowQuality,
	HighQuality,

	LowQuality_Linear,
	HighQuality_Linear
};