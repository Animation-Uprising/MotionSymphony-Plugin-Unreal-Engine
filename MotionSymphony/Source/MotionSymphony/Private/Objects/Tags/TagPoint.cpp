// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "Tags/TagPoint.h"

UTagPoint::UTagPoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UTagPoint::PreProcessTag(const FPoseMotionData& PointPose, FMotionAnimAsset& OutMotionAnim,
	UMotionDataAsset* OutMotionData, const float Time)
{
	Received_PreProcessTag(PointPose, OutMotionAnim, OutMotionData, Time);
}

void UTagPoint::CopyTagData(UTagPoint* CopyTag)
{
}
