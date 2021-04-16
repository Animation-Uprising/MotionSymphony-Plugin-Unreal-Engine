// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "Objects/Tag_Trait.h"
#include "MotionMatchingUtil/MMBlueprintFunctionLibrary.h"
#include "MotionSymphonySettings.h"
#include "Data/MotionTraitField.h"

UTag_Trait::UTag_Trait(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NotifyColor = FColor::Yellow;
}

void UTag_Trait::PreProcessTag(FMotionAnimAsset& OutMotionAnim, 
	UMotionDataAsset* OutMotionData, const float StartTime, const float EndTime) 
{
	Super::PreProcessTag(OutMotionAnim, OutMotionData, StartTime, EndTime);

	UMotionSymphonySettings* Settings = GetMutableDefault<UMotionSymphonySettings>();

	if(!Settings)
	{
		TraitHandle = FMotionTraitField();
		UE_LOG(LogTemp, Warning, TEXT("Trying to pre-process trait tag but the motion symphony settings could not be found."));
		return;
	}

	for (int32 i = 0; i < Settings->TraitNames.Num(); ++i)
	{
		if (TraitName == Settings->TraitNames[i])
		{
			TraitHandle = FMotionTraitField(i);
			return;
		}
	}
	
	TraitHandle = FMotionTraitField();
	UE_LOG(LogTemp, Warning, TEXT("Trying to pre-process trait tag but the trait name could not be found. Please verify your trait names in project settings."));
}


void UTag_Trait::PreProcessPose(FPoseMotionData& OutPose, FMotionAnimAsset& OutMotionAnim, 
	UMotionDataAsset* OutMotionData, const float StartTime, const float EndTime)
{
	Super::PreProcessPose(OutPose, OutMotionAnim, OutMotionData, StartTime, EndTime);
	
	OutPose.Traits |= TraitHandle;
}
