// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AnimGraph/AnimNode_PoseMatching.h"

FAnimNode_PoseMatching::FAnimNode_PoseMatching()
{
}

UAnimSequenceBase* FAnimNode_PoseMatching::FindActiveAnim()
{
	return GetSequence();
}

void FAnimNode_PoseMatching::PreProcess()
{
	UAnimSequenceBase* LocalSequence = GetSequence();
	if (!LocalSequence)
	{ 
		return;
	}
	
	FAnimNode_PoseMatchBase::PreProcess();
	
	CurrentPose.Empty(PoseConfig.Num() + 1);
	for (FMatchBone& MatchBone : PoseConfig)
	{
		MatchBone.Bone.Initialize(LocalSequence->GetSkeleton());
		CurrentPose.Emplace(FJointData());
	}

	//Non mirrored animation
	PreProcessAnimation(Cast<UAnimSequence>(LocalSequence), 0);

	if (bEnableMirroring && MirroringProfile)
	{
		//Mirrored animation
		PreProcessAnimation(Cast<UAnimSequence>(LocalSequence), 0, true);
	}
}