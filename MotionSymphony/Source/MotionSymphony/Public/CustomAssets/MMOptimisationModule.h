// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/PoseMotionData.h"
#include "Data/CalibrationData.h"
#include "MMOptimisationModule.generated.h"

class UMotionDataAsset;
class UWorld;
class FPrimitiveDrawInterface;

UCLASS()
class MOTIONSYMPHONY_API UMMOptimisationModule : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	class UMotionDataAsset* ParentMotionDataAsset;

	UPROPERTY()
	bool bIsProcessed;

	bool bIsRuntimeInitialized;

public:
	UMMOptimisationModule(const FObjectInitializer& ObjectInitializer);

	
	virtual void BuildOptimisationStructures(UMotionDataAsset* InMotionDataAsset);
	virtual TArray<FPoseMotionData>* GetFilteredPoseList(const FPoseMotionData& CurrentPose, 
	const FMotionTraitField RequiredTraits, const FCalibrationData& FinalCalibration);

	virtual void InitializeRuntime();
	virtual bool IsProcessedAndValid(const UMotionDataAsset* CheckMotionData) const;

	virtual void DrawDebug(FPrimitiveDrawInterface* DrawInterface, const UWorld* World, const UMotionDataAsset* MotionData) const;
};