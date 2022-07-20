// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "Tags/Tag_DistanceMatch.h"
#include "Data/DistanceMatchSection.h"
#include "Animation/AnimSequence.h"

UTag_DistanceMatch::UTag_DistanceMatch(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	DistanceMatchType(EDistanceMatchType::None),
	DistancematchBasis(EDistanceMatchBasis::Positional),
	Lead(1.0f),
	Tail(1.0f)
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor::Green;
#endif
}

void UTag_DistanceMatch::PreProcessTag(const FPoseMotionData& PointPose, 
                                       FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData, const float Time)
{
	Super::PreProcessTag(PointPose, OutMotionAnim, OutMotionData, Time);
	if (OutMotionAnim.AnimAsset == nullptr || !OutMotionData ||DistanceMatchType == EDistanceMatchType::None || OutMotionAnim.MotionAnimAssetType == EMotionAnimAssetType::BlendSpace)
	{
		return;
	}



	switch (DistanceMatchType)
	{
		case EDistanceMatchType::None: return;
		case EDistanceMatchType::Backward: Lead = 0.0f; break;
		case EDistanceMatchType::Forward: Tail = 0.0f; break;
	}

	Lead  = FMath::Abs(Lead);
	Tail = FMath::Abs(Tail);

	float StartTime = FMath::Clamp(Time - Lead, 0.0f, FMath::Min(Time, (float)OutMotionAnim.GetPlayLength()));
	float EndTime = FMath::Clamp(Time + Tail, FMath::Max(0.0f, Time), (float)OutMotionAnim.GetPlayLength());

	int32 StartPoseId = PointPose.PoseId - FMath::CeilToInt((Time - StartTime) / OutMotionData->PoseInterval);
	int32 EndPoseId = PointPose.PoseId + FMath::FloorToInt((EndTime - StartTime) / OutMotionData->PoseInterval);

	FDistanceMatchSection NewSection(Time, DistanceMatchType, DistancematchBasis, 
		OutMotionAnim.AnimId, OutMotionAnim.MotionAnimAssetType, StartPoseId, EndPoseId, StartTime, EndTime);

	switch (OutMotionAnim.MotionAnimAssetType)
	{
		case EMotionAnimAssetType::Sequence:
		{
			if (DistancematchBasis == EDistanceMatchBasis::Positional)
			{
				NewSection.GenerateDistanceCurve(Cast<UAnimSequence>(OutMotionAnim.AnimAsset));
			}
			else
			{
				NewSection.GenerateRotationCurve(Cast<UAnimSequence>(OutMotionAnim.AnimAsset));
			}

		} break;
		default:
		{
			return;
		} 
	}

	OutMotionData->AddDistanceMatchSection(NewSection);
}

void UTag_DistanceMatch::CopyTagData(UTagPoint* CopyTag)
{
	UTag_DistanceMatch* Tag = Cast<UTag_DistanceMatch>(CopyTag);

	if(Tag)
	{
		DistanceMatchType = Tag->DistanceMatchType;
		DistancematchBasis = Tag->DistancematchBasis;
		Lead = Tag->Lead;
		Tail = Tag->Tail;
	}
}
