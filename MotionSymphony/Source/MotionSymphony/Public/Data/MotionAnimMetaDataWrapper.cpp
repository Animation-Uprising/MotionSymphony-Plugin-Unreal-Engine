#include "Data/MotionAnimMetaDataWrapper.h"
// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "CustomAssets/MotionDataAsset.h"
#include "Data/MotionAnimAsset.h"

UMotionAnimMetaDataWrapper::UMotionAnimMetaDataWrapper(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	bLoop(false),
	PlayRate(1.0f),
	bEnableMirroring(false),
	bFlattenTrajectory(true),
	PastTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	PrecedingMotion(nullptr),
	FutureTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	FollowingMotion(nullptr),
	CostMultiplier(1.0f),
	ParentAsset(nullptr)
{
}

void UMotionAnimMetaDataWrapper::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (ParentAsset == nullptr)
		return;

	ParentAsset->MotionAnimMetaDataModified();
}

void UMotionAnimMetaDataWrapper::SetProperties(FMotionAnimAsset* MetaData)
{
	if(!MetaData)
		return;

	Modify();
	bLoop = MetaData->bLoop;
	PlayRate = MetaData->PlayRate;
	bEnableMirroring = MetaData->bEnableMirroring;
	bFlattenTrajectory = MetaData->bFlattenTrajectory;
	PrecedingMotion = MetaData->PrecedingMotion;
	FutureTrajectory = MetaData->FutureTrajectory;
	FollowingMotion = MetaData->FollowingMotion;
	PastTrajectory = MetaData->PastTrajectory;
	CostMultiplier = MetaData->CostMultiplier;

	for (const FString& TraitName : MetaData->TraitNames)
	{
		TraitNames.Add(TraitName);
	}

	MarkPackageDirty();
}

UMotionBlendSpaceMetaDataWrapper::UMotionBlendSpaceMetaDataWrapper(const FObjectInitializer& ObjectInitializer)
	: UMotionAnimMetaDataWrapper(ObjectInitializer),
	SampleSpacing(0.1f)
{
}

void UMotionBlendSpaceMetaDataWrapper::SetProperties(FMotionAnimAsset* MetaData)
{
	Super::SetProperties(MetaData);

	FMotionBlendSpace* BlendSpaceData = static_cast<FMotionBlendSpace*>(MetaData);

	if (!BlendSpaceData)
		return;
	
	Modify();
	SampleSpacing = BlendSpaceData->SampleSpacing;
	MarkPackageDirty();
}

