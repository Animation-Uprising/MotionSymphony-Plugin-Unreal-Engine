// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "CustomAssets/MMOptimisation_TraitBins.h"
#include "CustomAssets/MotionDataAsset.h"

UMMOptimisation_TraitBins::UMMOptimisation_TraitBins(const FObjectInitializer& ObjectInitializer)
	: UMMOptimisationModule(ObjectInitializer)
{

}

void UMMOptimisation_TraitBins::BuildOptimisationStructures(UMotionDataAsset* InMotionDataAsset)
{
	Super::BuildOptimisationStructures(InMotionDataAsset);

	PoseBins.Empty();

	for (FPoseMotionData& Pose : InMotionDataAsset->Poses)
	{
		if(Pose.bDoNotUse)
			continue;

		FPoseBin& PoseBin = PoseBins.FindOrAdd(Pose.Traits);
		//PoseBin.Poses.Add(FPoseMotionData(Pose));
		PoseBin.SerializedPoseIds.Add(Pose.PoseId);
	}
}

TArray<FPoseMotionData>* UMMOptimisation_TraitBins::GetFilteredPoseList(const FPoseMotionData& CurrentPose, 
	const FMotionTraitField RequiredTraits, const FCalibrationData& FinalCalibration)
{
	if (PoseBins.Contains(RequiredTraits))
	{
		return &PoseBins[RequiredTraits].Poses;
	}

	return nullptr;
}

void UMMOptimisation_TraitBins::InitializeRuntime()
{
	if(bIsRuntimeInitialized || !ParentMotionDataAsset)
		return;

	Super::InitializeRuntime();

	for (auto& PoseBinPair : PoseBins)
	{
		FPoseBin& PoseBin = PoseBinPair.Value;
		PoseBin.InitializeRuntime(ParentMotionDataAsset->Poses);
	}
}

void FPoseBin::InitializeRuntime(TArray<FPoseMotionData>& FullPoseList)
{
	Poses.Empty(SerializedPoseIds.Num() + 1);
	for (int32 PoseId : SerializedPoseIds)
	{
		Poses.Add(FullPoseList[FMath::Clamp(PoseId, 0, FullPoseList.Num() - 1)]);
	}
}
