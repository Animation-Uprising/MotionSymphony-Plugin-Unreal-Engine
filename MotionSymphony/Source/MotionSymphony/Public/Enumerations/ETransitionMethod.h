// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ETransitionMethod.generated.h"

UENUM(BlueprintType)
enum class ETransitionMethod : uint8
{
	None,
	Inertialization,
	Blend
};