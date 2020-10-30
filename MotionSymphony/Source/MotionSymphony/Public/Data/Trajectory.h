// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrajectoryPoint.h"
#include "Trajectory.generated.h"

USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FTrajectory
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	TArray<FTrajectoryPoint>  TrajectoryPoints;

public:
	FTrajectory();
	~FTrajectory();

public: 
	void Initialize(int a_trajCount);
	void Clear();

	void MakeRelativeTo(FTransform a_transform);
};