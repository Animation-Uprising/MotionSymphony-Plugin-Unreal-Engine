// Copyright 2020-2023 Kenneth Claassen. All Rights Reserved.

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

	//Non mirrored animation
	PreProcessAnimation(Cast<UAnimSequence>(LocalSequence), 0);

	if (bEnableMirroring && MirrorDataTable)
	{
		//Mirrored animation
		PreProcessAnimation(Cast<UAnimSequence>(LocalSequence), 0, true);
	}
}