// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MMOptimisationModule.h"
#include "MMOptimisation_TraitBins.generated.h"

USTRUCT()
struct MOTIONSYMPHONY_API FPoseBin
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<int32> SerializedPoseIds;

	TArray<FPoseMotionData> Poses;

public:
	void InitializeRuntime(TArray<FPoseMotionData>& FullPoseList);
};


UCLASS()
class MOTIONSYMPHONY_API UMMOptimisation_TraitBins : public UMMOptimisationModule
{
	GENERATED_BODY()

public:
	UPROPERTY();
	TMap<FMotionTraitField, FPoseBin> PoseBins;

public:
	UMMOptimisation_TraitBins(const FObjectInitializer& ObjectInitializer);

	
	virtual void BuildOptimisationStructures(UMotionDataAsset* InMotionDataAsset) override;
	virtual TArray<FPoseMotionData>* GetFilteredPoseList(const FPoseMotionData& CurrentPose, 
		const FMotionTraitField RequiredTraits, const FCalibrationData& FinalCalibration) override;

	virtual void InitializeRuntime() override;
};