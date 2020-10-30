// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EJointVelocityCalculationMethod.generated.h"

UENUM(BlueprintType)
enum class EJointVelocityCalculationMethod : uint8
{
	BodyIndependent UMETA(DisplayName = "Body Independent"),
	BodyDependent UMETA(DisplayName = "Body Dependent")
};