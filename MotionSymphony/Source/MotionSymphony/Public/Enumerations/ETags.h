// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ETags.generated.h"

UENUM(BlueprintType, meta = (BitFlags))
enum class ETags : uint8
{
	None = 0,
	Tag1 = 0x1,
	Tag2 = 0x2,
	Tag3 = 0x4,
	Tag4 = 0x8,
	Tag5 = 0x10,
	Tag6 = 0x20,
	Tag7 = 0x40,
	Tag8 = 0x80,
};