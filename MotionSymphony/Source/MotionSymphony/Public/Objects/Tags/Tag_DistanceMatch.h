// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TagPoint.h"
#include "Enumerations/EDistanceMatchingEnums.h"
#include "Tag_DistanceMatch.generated.h"

UCLASS(editinlinenew, hidecategories = Object, collapsecategories)
class MOTIONSYMPHONY_API UTag_DistanceMatch : public UTagPoint
{
	GENERATED_UCLASS_BODY()

public:
	/** The type of distance matching that should be used with this tag. Forward - distance match toward a point, backward - distance match away from a point*/
	UPROPERTY(EditAnywhere, Category = "MotionSymphony|DistanceMatching")
	EDistanceMatchType DistanceMatchType;

	UPROPERTY(EditAnywhere, Category = "MotionSymphony|DistanceMatching")
	EDistanceMatchBasis DistancematchBasis;

	UPROPERTY(EditAnywhere, Category = "MotionSymphony|DistanceMatching", meta = (ClampMin=0.0f))
	float Lead;

	UPROPERTY(EditAnywhere, Category = "MotionSymphony|DistanceMatching", meta = (ClampMin=0.0f))
	float Tail;

public:
	UTag_DistanceMatch(UTag_DistanceMatch& CopyTag);
	
	virtual void PreProcessTag(const FPoseMotionData& PointPose, FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData, const float Time) override;
	//virtual void PreProcessPose(FPoseMotionData& OutPose, FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData) override;

	virtual void CopyTagData(UTagPoint* CopyTag) override;
};