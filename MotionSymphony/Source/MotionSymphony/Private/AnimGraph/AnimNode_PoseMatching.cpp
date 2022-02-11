// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AnimGraph/AnimNode_PoseMatching.h"
#include "Animation/AnimSequence.h"

FAnimNode_PoseMatching::FAnimNode_PoseMatching()
{
}

UAnimSequenceBase* FAnimNode_PoseMatching::FindActiveAnim()
{
	return Sequence;
}

#if WITH_EDITOR
void FAnimNode_PoseMatching::PreProcess()
{
	FAnimNode_PoseMatchBase::PreProcess();

	if (!Sequence)
	{ 
		return;
	}
	
	CurrentPose.Empty(PoseConfig.Num() + 1);
	for (FMatchBone& MatchBone : PoseConfig)
	{
		MatchBone.Bone.Initialize(Sequence->GetSkeleton());
		CurrentPose.Emplace(FJointData());
	}

	//Non mirrored animation
	PreProcessAnimation(Cast<UAnimSequence>(Sequence), 0);

	if (bEnableMirroring && MirroringProfile)
	{
		//Mirrored animation
		PreProcessAnimation(Cast<UAnimSequence>(Sequence), 0, true);
	}
}
#endif
