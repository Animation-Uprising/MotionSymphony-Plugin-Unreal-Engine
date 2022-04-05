// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AnimChannelState.h"
#include "MotionSymphony.h"

FAnimChannelState::FAnimChannelState()
	: Weight(0.0f), 
	  HighestWeight(0.0f), 
	  AnimId(0), 
	  AnimType(EMotionAnimAssetType::None),
	  StartPoseId(0), 
	  StartTime(0.0f), 
	  BlendSpacePosition(FVector2D(0.0f)),
	  Age(0.0f),
	  DecayAge(0.0f), 
	  AnimTime(0.0f), 
	  BlendStatus(EBlendStatus::Inactive),
	  bLoop(false),
	  PlayRate(1.0f),
	  bMirrored(false),
	  AnimLength(0.0f),
	  CachedTriangulationIndex(-1)
{ 
}

FAnimChannelState::FAnimChannelState(const FPoseMotionData & InPose, 
	EBlendStatus InBlendStatus, float InWeight, float InAnimLength, bool bInLoop,
	float InPlayRate, bool bInMirrored, float InTimeOffset /*= 0.0f*/, float InPoseOffset /*= 0.0f*/)
	: Weight(InWeight), 
	HighestWeight(InWeight),
	AnimId(InPose.AnimId), 
	AnimType(InPose.AnimType),
	StartPoseId(InPose.PoseId),
	StartTime(InPose.Time),
	BlendSpacePosition(InPose.BlendSpacePosition),
	Age(InTimeOffset), 
	DecayAge(0.0f), 
	AnimTime(InPose.Time + InTimeOffset + InPoseOffset), 
	BlendStatus(InBlendStatus),
	bLoop(bInLoop),
	PlayRate(InPlayRate),
	bMirrored(bInMirrored),
	AnimLength(InAnimLength),
	CachedTriangulationIndex(-1)
{ 
	if (Weight > 0.999f)
	{
		StartTime -= FMath::Min(StartTime, 0.3f);
		Age = 0.3f + InTimeOffset;
	}

	if(AnimTime > AnimLength)
	{
		AnimTime = AnimLength;
	}
}

float FAnimChannelState::Update(const float DeltaTime, const float BlendTime,
	const bool Current, const float NodePlayRate)
{
	if (BlendStatus == EBlendStatus::Inactive)
	{
		return 0.0f;
	}

	AnimTime += DeltaTime * PlayRate * NodePlayRate;
	
	if (bLoop && AnimTime > AnimLength)
	{
		const float WrapAmount = (AnimTime / AnimLength);
		AnimTime = (WrapAmount - FMath::Floor(WrapAmount)) * AnimLength;
		
		//AnimTime -= AnimLength;
	}

	if (Current)
	{
		Age += DeltaTime * NodePlayRate;
		Weight = HighestWeight = FMath::Sin((PI / 2.0f) * FMath::Clamp(Age / BlendTime, 0.0f, 1.0f));
	}
	else
	{
		Weight = HighestWeight * (1.0f - FMath::Sin((PI / 2.0f) * FMath::Clamp(DecayAge / BlendTime, 0.0f, 1.0f)));

		if (Weight < DeltaTime * NodePlayRate)
		{
			Weight = HighestWeight = 0.0f;
			Age = 0.0f;
			DecayAge = 0.0f;
			BlendStatus = EBlendStatus::Inactive;
			return -1.0f;
		}
		else
		{
			Age += DeltaTime * NodePlayRate;
			DecayAge += DeltaTime * NodePlayRate;
		}
	}

	return Weight;
}