
#include "CustomAssets/MotionAnimAsset.h"


FMotionAnimAsset::FMotionAnimAsset()
	: MotionAnimAssetType(EMotionAnimAssetType::None),
	bLoop(false),
	bEnableMirroring(false),
	Favour(1.0f),
	bFlattenTrajectory(true),
	PastTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	FutureTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	AnimAsset(nullptr),
	PrecedingMotion(nullptr),
	FollowingMotion(nullptr)
{
}

FMotionAnimAsset::FMotionAnimAsset(UAnimationAsset* InAnimAsset)
	: MotionAnimAssetType(EMotionAnimAssetType::None),
	bLoop(false),
	bEnableMirroring(false),
	Favour(1.0f),
	bFlattenTrajectory(true),
	PastTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	FutureTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	AnimAsset(InAnimAsset),
	PrecedingMotion(nullptr),
	FollowingMotion(nullptr)
{
}

float FMotionAnimAsset::GetAnimLength() const
{
	return 0.0f;
}

FMotionAnimSequence::FMotionAnimSequence()
	: FMotionAnimAsset(),
	Sequence(nullptr)
{
	MotionAnimAssetType = EMotionAnimAssetType::Sequence;
}

FMotionAnimSequence::FMotionAnimSequence(UAnimSequence* InSequence)
	: FMotionAnimAsset(InSequence),
	Sequence(InSequence)
{
	MotionAnimAssetType = EMotionAnimAssetType::Sequence;
}

float FMotionAnimSequence::GetAnimLength() const
{
	return Sequence == nullptr ? 0.0f : Sequence->SequenceLength;
}

FMotionBlendSpace::FMotionBlendSpace()
	: FMotionAnimAsset(),
	BlendSpace(nullptr),
	SampleSpacing(0.1f, 0.1f)
{
	MotionAnimAssetType = EMotionAnimAssetType::BlendSpace;
}

FMotionBlendSpace::FMotionBlendSpace(UBlendSpaceBase* InBlendSpace)
	: FMotionAnimAsset(InBlendSpace),
	BlendSpace(InBlendSpace),
	SampleSpacing(0.1f, 0.1f)
{
	MotionAnimAssetType = EMotionAnimAssetType::BlendSpace;
}

float FMotionBlendSpace::GetAnimLength() const
{
	return BlendSpace == nullptr ? 0.0f : BlendSpace->AnimLength;
}
