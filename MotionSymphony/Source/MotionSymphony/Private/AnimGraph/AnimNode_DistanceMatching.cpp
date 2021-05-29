// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

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
	bNegateDistanceCurve(false),
	MovementType(EDistanceMatchType::None),
	DistanceLimit(-1.0f),
	DestinationReachedThreshold(5.0f),
	SmoothRate(-1.0f),
	SmoothTimeThreshold(0.15f)
{

}

bool FAnimNode_DistanceMatching::NeedsOnInitializeAnimInstance() const
{
	return true;
}

void FAnimNode_DistanceMatching::OnInitializeAnimInstance(const FAnimInstanceProxy* InAnimInstanceProxy, const UAnimInstance* InAnimInstance)
{
	if (!Sequence)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to initialize distance matching node. The sequence is null and has not been set"));
		return;
	}

	DistanceMatchingModule.Setup(Sequence, DistanceCurveName);

	const float AdjustedPlayRate = PlayRateScaleBiasClamp.ApplyTo(FMath::IsNearlyZero(PlayRateBasis) ? 0.0f : (PlayRate / PlayRateBasis), 0.0f);
	const float EffectivePlayrate = Sequence->RateScale * AdjustedPlayRate;
	if (StartPosition == 0.0f && EffectivePlayrate < 0.0f)
	{
		InternalTimeAccumulator = Sequence->GetPlayLength();
	}
}

void FAnimNode_DistanceMatching::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_AssetPlayerBase::Initialize_AnyThread(Context);
	GetEvaluateGraphExposedInputs().Execute(Context);

	InternalTimeAccumulator = StartPosition = 0.0f;

	DistanceMatchingModule.Initialize();
}

void FAnimNode_DistanceMatching::UpdateAssetPlayer(const FAnimationUpdateContext& Context)
{
	GetEvaluateGraphExposedInputs().Execute(Context);

	if(!Sequence)
	{
		return;
	}

	int32 Enabled = CVarDistanceMatchingEnabled.GetValueOnAnyThread();
	
	if (Enabled 
	&& (DistanceLimit < 0.0f || DesiredDistance < DistanceLimit))
	{
		//Evaluate Distance matching time
		float Time = -1.0f;
		bool bDestinationReached = (MovementType == EDistanceMatchType::Forward 
			&& DesiredDistance < DestinationReachedThreshold);

		if (!bDestinationReached)
		{
			Time = DistanceMatchingModule.FindMatchingTime(DesiredDistance, bNegateDistanceCurve);
		}

		if (Time > -0.5f)
		{
			//Clamp the time so that it cannot be beyond clip limits and so that the animation cannot run backward
			if (SmoothRate > 0.0f && FMath::Abs(Time - InternalTimeAccumulator) < SmoothTimeThreshold)
			{
				float DesiredTime = FMath::Clamp(Time, 0.0f /*InternalTimeAccumulator*/, Sequence->GetPlayLength());
				InternalTimeAccumulator = FMath::Lerp(InternalTimeAccumulator, DesiredTime, SmoothRate);
				UE_LOG(LogTemp, Log, TEXT("Smoothing"));
			}
			else
			{
				InternalTimeAccumulator = FMath::Clamp(Time, 0.0f /*InternalTimeAccumulator*/, Sequence->GetPlayLength());
				UE_LOG(LogTemp, Log, TEXT("Not Smoothing"));
			}

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