#include "CustomAssets/MotionAnimMetaDataWrapper.h"
#include "CustomAssets/MotionDataAsset.h"
#include "CustomAssets/MotionAnimAsset.h"

UMotionAnimMetaDataWrapper::UMotionAnimMetaDataWrapper(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	bLoop(false),
	bEnableMirroring(false),
	Favour(1.0f),
	bFlattenTrajectory(true),
	PastTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	PrecedingMotion(nullptr),
	FutureTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	FollowingMotion(nullptr),
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
	bEnableMirroring = MetaData->bEnableMirroring;
	Favour = MetaData->Favour;
	bFlattenTrajectory = MetaData->bFlattenTrajectory;
	PrecedingMotion = MetaData->PrecedingMotion;
	FutureTrajectory = MetaData->FutureTrajectory;
	PastTrajectory = MetaData->PastTrajectory;
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

