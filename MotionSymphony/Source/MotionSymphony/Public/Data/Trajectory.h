// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrajectoryPoint.h"
#include "Trajectory.generated.h"

USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FTrajectory
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
	TArray<FTrajectoryPoint>  TrajectoryPoints;

public:
	FTrajectory();
	~FTrajectory();

public: 
	void Initialize(int a_trajCount);
	void Clear();

	void MakeRelativeTo(FTransform a_transform);

	void SetTrajectoryPoint(const int32 Index, const FVector InPosition, const float InRotationZ);
	void AddTrajectoryPoint(const FVector InPosition, const float InRotationZ);
	int32 TrajectoryPointCount() const;
};