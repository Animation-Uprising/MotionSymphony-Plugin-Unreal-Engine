// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrajectoryPoint.generated.h"

USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FTrajectoryPoint
{
	GENERATED_USTRUCT_BODY()
	
public:
	UPROPERTY()
	FVector Position;

	UPROPERTY()
	float RotationZ;

public:
	FTrajectoryPoint();
	FTrajectoryPoint(FVector a_position, float a_rotationX);

	static void Lerp(FTrajectoryPoint& o_result, FTrajectoryPoint& a_from, 
		FTrajectoryPoint& a_to, float a_progress);

	FTrajectoryPoint& operator += (const FTrajectoryPoint& rhs);
	FTrajectoryPoint& operator /= (const float rhs);
	FTrajectoryPoint& operator *= (const float rhs);
};