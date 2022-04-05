// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "Data/DistanceMatchSection.h"
#include "Animation/AnimSequence.h"
#include "Runtime/Launch/Resources/Version.h"

FDistanceMatchSection::FDistanceMatchSection()
	: TargetTime(0.0f),
	  StartTime(0.0f),
	  EndTime(1.0f),
	  MatchType(EDistanceMatchType::None),
	  MatchBasis(EDistanceMatchBasis::Positional),
	  AnimId(0),
	  AnimType(EMotionAnimAssetType::None),
	  StartPoseId(0),
	  EndPoseId(0),
	  MaxDistance(500.0f)
{
}

FDistanceMatchSection::FDistanceMatchSection(const float InTargetTime, const EDistanceMatchType InMatchType, 
	const EDistanceMatchBasis InMatchBasis, const int32 InAnimId, const EMotionAnimAssetType InAnimType,
	const int32 InStartPoseId, const int32 InEndPoseId, const float InStartTime, const float InEndTime)
	:	TargetTime(InTargetTime),
		StartTime(InStartTime),
		EndTime(InEndTime),
		MatchType(InMatchType),
		MatchBasis(InMatchBasis),
		AnimId(InAnimId),
		AnimType(InAnimType),
		StartPoseId(InStartPoseId),
		EndPoseId(InEndPoseId),
		MaxDistance(500.0f)
{
}

FDistanceMatchSection::~FDistanceMatchSection()
{
}

void FDistanceMatchSection::GenerateDistanceCurve(const UAnimSequence* Sequence)
{
	if (!Sequence)
	{
		return;
	}

	const float FrameRate = 1.0f / Sequence->GetSamplingFrameRate().AsDecimal();

	TArray<float> RootDistance;
	TArray<float> FrameTimes;

	//Pass 1: Record distance from point backwards
	float CumulativeDistance = 0.0f;
	DistanceCurve.UpdateOrAddKey(CumulativeDistance, TargetTime);
	for (float Time = TargetTime - FrameRate; Time >= StartTime; Time -= FrameRate)
	{
		FTransform RootMotion = Sequence->ExtractRootMotion(Time + FrameRate, FrameRate, false);

		if (MatchType == EDistanceMatchType::Both || MatchType == EDistanceMatchType::Forward)
		{
			CumulativeDistance += RootMotion.GetLocation().Size();

			DistanceCurve.UpdateOrAddKey(CumulativeDistance, Time);
		}
	}

	//Pass 2: Record distance from point forwards
	CumulativeDistance = 0.0f;
	for (float Time = TargetTime + FrameRate; Time <= EndTime; Time += FrameRate)
	{
		FTransform RootMotion = Sequence->ExtractRootMotion(Time - FrameRate, FrameRate, false);

		if (MatchType == EDistanceMatchType::Both || MatchType == EDistanceMatchType::Backward)
		{
			CumulativeDistance -= RootMotion.GetLocation().Size();
			DistanceCurve.UpdateOrAddKey(CumulativeDistance, Time);
		}
	}
}

void FDistanceMatchSection::GenerateRotationCurve(const UAnimSequence* Sequence)
{
	if (!Sequence)
	{
		return;
	}
	
	const float FrameRate = Sequence->GetSamplingFrameRate().AsDecimal();
	
	TArray<float> RootDistance;
	TArray<float> FrameTimes;

	//Pass 1: Record distance from point backwards
	float CumulativeDistance = 0.0f;
	DistanceCurve.UpdateOrAddKey(CumulativeDistance, TargetTime);
	for (int32 Time = TargetTime - FrameRate; Time >= StartTime; Time -= FrameRate)
	{
		FTransform RootMotion = Sequence->ExtractRootMotion(Time + FrameRate, FrameRate, false);

		if (MatchType == EDistanceMatchType::Both || MatchType == EDistanceMatchType::Forward)
		{
			CumulativeDistance += RootMotion.Rotator().Yaw;
			DistanceCurve.UpdateOrAddKey(CumulativeDistance, Time);
		}
	}

	//Pass 2: Record distance from point forwards
	CumulativeDistance = 0.0f;
	for (int32 Time = TargetTime + FrameRate; Time <= EndTime; Time += FrameRate)
	{
		FTransform RootMotion = Sequence->ExtractRootMotion(Time - FrameRate, FrameRate, false);


		if (MatchType == EDistanceMatchType::Both || MatchType == EDistanceMatchType::Forward)
		{
			CumulativeDistance -= RootMotion.Rotator().Yaw;
			DistanceCurve.UpdateOrAddKey(CumulativeDistance, Time);
		}
	}
}


float FDistanceMatchSection::FindMatchingTime(const float DesiredDistance, int32& LastKeyChecked)
{
	int32 KeyCount = DistanceCurve.FloatCurve.Keys.Num();
	int32 StartKey = LastKeyChecked;

	if(StartKey < 0 || StartKey >= KeyCount)
		return -1.0f;

	FRichCurveKey* PKey = &DistanceCurve.FloatCurve.Keys[StartKey];
	FRichCurveKey* SKey = nullptr;

	for (int32 i = StartKey; i < KeyCount; ++i)
	{
		FRichCurveKey& Key = DistanceCurve.FloatCurve.Keys[i];

		if (Key.Value > DesiredDistance)
		{
			PKey = &Key;
		}
		else
		{
			SKey = &Key;
			LastKeyChecked = i;
			break;
		}
	}

	if (!SKey)
	{
		return PKey->Time;
	}

	float dV = SKey->Value - PKey->Value;

	if (dV < 0.000001f)
	{
		return PKey->Time;
	}

	float dT = SKey->Time - PKey->Time;

	return ((dT / dV) * (DesiredDistance - PKey->Value)) + PKey->Time; //Interpolate the exact time
}

FDistanceMatchPayload::FDistanceMatchPayload()
	: 
	bTrigger(false),
	MatchType(EDistanceMatchType::None),
	MatchBasis(EDistanceMatchBasis::Positional),
	MarkerDistance(0.0f)
{

}

FDistanceMatchPayload::FDistanceMatchPayload(const bool bInTrigger, const EDistanceMatchType InMatchType, 
	const EDistanceMatchBasis InMatchBasis, const float InMarkerDistance)
	: bTrigger(bInTrigger),
	MatchType(InMatchType),
	MatchBasis(InMatchBasis),
	MarkerDistance(InMarkerDistance)
{
}
