// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "CustomAssets/MMOptimisationModule.h"

UMMOptimisationModule::UMMOptimisationModule(const FObjectInitializer& ObjectInitializer)
	: UObject(ObjectInitializer),
	ParentMotionDataAsset(nullptr),
	bIsProcessed(false),
	bIsRuntimeInitialized(false)
{

}

void UMMOptimisationModule::BuildOptimisationStructures(UMotionDataAsset* InMotionDataAsset)
{
	Modify();

	ParentMotionDataAsset = InMotionDataAsset;

	bIsProcessed = true;
	bIsRuntimeInitialized = false;
}

TArray<FPoseMotionData>* UMMOptimisationModule::GetFilteredPoseList(const FPoseMotionData& CurrentPose, const FMotionTraitField RequiredTraits, 
	const FCalibrationData& FinalCalibration)
{
	return nullptr;
}

void UMMOptimisationModule::InitializeRuntime()
{
	bIsRuntimeInitialized = true;
}

bool UMMOptimisationModule::IsProcessedAndValid(const UMotionDataAsset* CheckMotionData) const
{
	return bIsProcessed && ParentMotionDataAsset && ParentMotionDataAsset == CheckMotionData;
}
