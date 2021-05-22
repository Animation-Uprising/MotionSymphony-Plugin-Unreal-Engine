// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AnimGraph/AnimNode_TimeMatching.h"
#include "Animation/AnimInstanceProxy.h"

#define LOCTEXT_NAMESPACE "MotionSymphonyNodes"

static TAutoConsoleVariable<int32> CVarTimeMatchingEnabled(
	TEXT("a.AnimNode.MoSymph.TimeMatch.Enabled"),
	1,
	TEXT("Turns Time Matching On / Off. \n")
	TEXT("<=0: Off \n")
	TEXT("  1: On"));


FAnimNode_TimeMatching::FAnimNode_TimeMatching()
	: DesiredTime(0.0f),
	MarkerTime(0.0f),
	DistanceMatching(nullptr),
	AnimInstanceProxy(nullptr)
{
}

float FAnimNode_TimeMatching::FindMatchingTime()
{
	return MarkerTime - DesiredTime;
}

void FAnimNode_TimeMatching::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_AssetPlayerBase::Initialize_AnyThread(Context);
	GetEvaluateGraphExposedInputs().Execute(Context);

	InternalTimeAccumulator = StartPosition;

	if (!Sequence)
	{
		return;
	}

	if (!bInitialized)
	{
		AnimInstanceProxy = Context.AnimInstanceProxy;
		DistanceMatching = Cast<UDistanceMatching>(AnimInstanceProxy->GetSkelMeshComponent()->GetOwner()->GetComponentByClass(UDistanceMatching::StaticClass()));

		bInitialized = true;
	}

	if (DistanceMatching)
	{
		DesiredTime = DistanceMatching->GetTimeToMarker();
	}

	const float AdjustedPlayRate = PlayRateScaleBiasClamp.ApplyTo(FMath::IsNearlyZero(PlayRateBasis) ? 0.0f : (PlayRate / PlayRateBasis), 0.0f);
	const float EffectivePlayrate = Sequence->RateScale * AdjustedPlayRate;

	DesiredTime *= EffectivePlayrate;
	if (CVarTimeMatchingEnabled.GetValueOnAnyThread() == 1)
	{
		InternalTimeAccumulator = FindMatchingTime();
	}

	if (StartPosition == 0.0f && EffectivePlayrate < 0.0f)
	{
		InternalTimeAccumulator = Sequence->SequenceLength;
	}
}



#undef LOCTEXT_NAMESPACE