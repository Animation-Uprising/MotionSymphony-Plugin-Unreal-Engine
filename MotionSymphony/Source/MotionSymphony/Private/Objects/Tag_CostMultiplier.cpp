// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "Objects/Tag_CostMultiplier.h"

UTag_CostMultiplier::UTag_CostMultiplier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	CostMultiplier(1.0f),
	bOverride(true)
{
	NotifyColor = FColor::Purple;
}


void UTag_CostMultiplier::PreProcessTag(FMotionAnimAsset& OutMotionAnim, 
	UMotionDataAsset* OutMotionData, const float StartTime, const float EndTime) 
{
	Super::PreProcessTag(OutMotionAnim, OutMotionData, StartTime, EndTime);
}


void UTag_CostMultiplier::PreProcessPose(FPoseMotionData& OutPose, FMotionAnimAsset& OutMotionAnim, 
	UMotionDataAsset* OutMotionData, const float StartTime, const float EndTime)
{
	Super::PreProcessPose(OutPose, OutMotionAnim, OutMotionData, StartTime, EndTime);

	if (CostMultiplier < 0.1f)
	{
		UE_LOG(LogTemp, Warning, TEXT("CostMultiplier Tag has a value less than 0.1f? Is this intended? It may negatively impact runtime motion matching"));
	}

	if (bOverride)
	{
		OutPose.Favour = FMath::Abs(CostMultiplier);
	}
	else
	{
		OutPose.Favour *= FMath::Abs(CostMultiplier);
	}
}
