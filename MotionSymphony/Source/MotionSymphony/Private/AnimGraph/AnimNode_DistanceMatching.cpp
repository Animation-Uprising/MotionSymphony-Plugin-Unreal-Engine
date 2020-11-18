// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "AnimGraph/AnimNode_DistanceMatching.h"
#include "Animation/AnimInstanceProxy.h"

#define LOCTEXT_NAMESPACE "MotionSymphonyNodes"

static TAutoConsoleVariable<int32> CVarDistanceMatchingEnabled(
	TEXT("a.AnimNode.MoSymph.DistanceMatch.Enabled"),
	1,
	TEXT("Turns Distance Matching On / Off. \n")
	TEXT("<=0: Off \n")
	TEXT("  1: On"));


FAnimNode_DistanceMatching::FAnimNode_DistanceMatching()
	: DesiredDistance(0.0f),
	DistanceCurveName(FName(TEXT("MoSymph_Distance"))),
	MovementType(EDistanceMatchType::None),
	bInitialized(false),
	DistanceMatchInstanceId(-1),
	ActiveDistanceMatchInstanceId(-1),
	DistanceMatchType(EDistanceMatchType::None),
	DistanceMatching(nullptr),
	AnimInstanceProxy(nullptr)
{

}

void FAnimNode_DistanceMatching::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_AssetPlayerBase::Initialize_AnyThread(Context);
	GetEvaluateGraphExposedInputs().Execute(Context);

	InternalTimeAccumulator = StartPosition = 0.0f;
	DistanceMatchInstanceId = -1;
	//LastKeyChecked = 0;

	DistanceMatchingModule.Initialize();
	
	if(bInitialized)
		return;

	if (!Sequence)
		return;

	AnimInstanceProxy = Context.AnimInstanceProxy;
	DistanceMatching = Cast<UDistanceMatching>(AnimInstanceProxy->GetSkelMeshComponent()->GetOwner()->GetComponentByClass(UDistanceMatching::StaticClass()));
	
	DistanceMatchingModule.Setup(Sequence, DistanceCurveName);

	const float AdjustedPlayRate = PlayRateScaleBiasClamp.ApplyTo(FMath::IsNearlyZero(PlayRateBasis) ? 0.0f : (PlayRate / PlayRateBasis), 0.0f);
	const float EffectivePlayrate = Sequence->RateScale * AdjustedPlayRate;
	if (StartPosition == 0.0f && EffectivePlayrate < 0.0f)
	{
		InternalTimeAccumulator = Sequence->SequenceLength;
	}

	bInitialized = true;
}

void FAnimNode_DistanceMatching::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DistanceMatchInstanceId = DistanceMatching->GetCurrentInstanceId();
}

void FAnimNode_DistanceMatching::UpdateAssetPlayer(const FAnimationUpdateContext& Context)
{
	GetEvaluateGraphExposedInputs().Execute(Context);

	if(!Sequence)
		return;

	int32 Enabled = CVarDistanceMatchingEnabled.GetValueOnAnyThread();
	if (Enabled == 1 && DistanceMatching != nullptr)
	{
		ActiveDistanceMatchInstanceId = DistanceMatching->GetCurrentInstanceId();

		if(DistanceMatchInstanceId == -1)
		{
			DistanceMatchInstanceId = ActiveDistanceMatchInstanceId;
		}
		else if (ActiveDistanceMatchInstanceId != DistanceMatchInstanceId)
		{
			FAnimNode_SequencePlayer::UpdateAssetPlayer(Context);
			return;
		}

		DesiredDistance = DistanceMatching->GetMarkerDistance();
		DistanceMatchType = DistanceMatching->GetDistanceMatchType();
	
		float Time =  -1.0f; 
		if (!(DistanceMatchType == EDistanceMatchType::Forward && DesiredDistance < 5.0f))
		{
			Time = DistanceMatchingModule.FindMatchingTime(DesiredDistance);
		}

		if (Time > -0.5f)
		{
			InternalTimeAccumulator = FMath::Clamp(Time, InternalTimeAccumulator, Sequence->SequenceLength);
		}
		else
		{
			FAnimNode_SequencePlayer::UpdateAssetPlayer(Context);
		}

	}
	else
	{
		FAnimNode_SequencePlayer::UpdateAssetPlayer(Context);
	}
}



#undef LOCTEXT_NAMESPACE