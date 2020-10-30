// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BoneContainer.h"
#include "MotionMatchConfig.generated.h"

class USkeleton;

UCLASS()
class MOTIONSYMPHONY_API UMotionMatchConfig : public UObject
{
	GENERATED_BODY()

public:
	UMotionMatchConfig(const FObjectInitializer& ObjectInitializer);
	
public:
	UPROPERTY(EditAnywhere, Category = "General")
	USkeleton* SourceSkeleton;

	UPROPERTY(EditAnywhere, Category = "Trajectory Config")
	TArray<float> TrajectoryTimes;

	UPROPERTY(EditAnywhere, Category = "Pose Config")
	TArray<int32> PoseJoints;


public:
	USkeleton* GetSourceSkeleton();
	void SetSourceSkeleton(USkeleton* skeleton);

	bool IsSetupValid();
};
