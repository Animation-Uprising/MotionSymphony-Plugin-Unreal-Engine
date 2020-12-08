// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "AnimGraph/AnimNode_PoseMatching.h"

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
		return;
	
	CurrentPose.Empty(PoseConfiguration.Num());
	for (FMatchBone& MatchBone : PoseConfiguration)
	{
		MatchBone.Bone.Initialize(Sequence->GetSkeleton());
		CurrentPose.Emplace(FJointData());
	}

	PreProcessAnimation(Cast<UAnimSequence>(Sequence), 0);
}
#endif