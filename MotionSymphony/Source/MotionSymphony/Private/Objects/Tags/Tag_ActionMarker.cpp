// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "Tags/Tag_ActionMarker.h"
#include "MotionSymphonySettings.h"

UTag_ActionMarker::UTag_ActionMarker(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor::Blue;
#endif
}

void UTag_ActionMarker::PreProcessTag(const FPoseMotionData& PointPose, FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData, const float Time)
{
	UMotionSymphonySettings* Settings = GetMutableDefault<UMotionSymphonySettings>();

	if (!Settings)
	{
		UE_LOG(LogTemp, Warning, TEXT("Trying to pre-process action tag but the motion symphony settings could not be found."));
		return;
	}

	int32 ActionId = -1;
	for (int32 i = 0; i < Settings->ActionNames.Num(); ++i)
	{
		if (ActionName == Settings->ActionNames[i])
		{
			ActionId = i;
		}
	}

	if (ActionId > -1)
	{
		//We have a valid action
		OutMotionData->AddAction(PointPose, OutMotionAnim, ActionId, Time);
	}
}

void UTag_ActionMarker::CopyTagData(UTagPoint* CopyTag)
{
	UTag_ActionMarker* Tag = Cast<UTag_ActionMarker>(CopyTag);

	if(Tag)
	{
		ActionName = Tag->ActionName;
	}
}
