// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MMOptimisationModule.h"
#include "MMOptimisation_LayeredAABB.generated.h"

USTRUCT()
struct MOTIONSYMPHONY_API FPoseAABB
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FPoseMotionData> Poses;

	UPROPERTY()
	FPoseMotionData Center;

	UPROPERTY()
	FPoseMotionData Extents;

public:
	void AddPose(FPoseMotionData& Pose);
	void CalculateAABB();
	bool IsFull();
};

USTRUCT()
struct MOTIONSYMPHONY_API FPoseAABBCollection
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FPoseAABB> ChildAABBs;

	UPROPERTY()
	FPoseMotionData Center;

	UPROPERTY()
	FPoseMotionData Extents;

public:
	void AddPose(FPoseMotionData& Pose);
	void CalculateAABB();
	bool IsFull();
};

USTRUCT()
struct MOTIONSYMPHONY_API FLayeredAABBStructure
{
	GENERATED_BODY()

public:
	TArray<FPoseAABBCollection> ChildAABBs;

public:
	void AddPose(FPoseMotionData& Pose);
	void CalculateAABBs();
};


UCLASS()
class MOTIONSYMPHONY_API UMMOptimisation_LayeredAABB : public UMMOptimisationModule
{
	GENERATED_BODY()

public:
	UPROPERTY();
	TMap<FMotionTraitField, FLayeredAABBStructure> SearchStructure;

public:
	UMMOptimisation_LayeredAABB(const FObjectInitializer& ObjectInitializer);

	virtual void BuildOptimisationStructures(UMotionDataAsset* InMotionDataAsset) override;
	virtual TArray<FPoseMotionData>* GetFilteredPoseList(const FPoseMotionData& CurrentPose,
		const FMotionTraitField RequiredTraits, const FCalibrationData& FinalCalibration);
};