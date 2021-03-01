#pragma once

#include "CoreMinimal.h"
#include "TagSection.h"
#include "Enumerations/EDistanceMatchingEnums.h"
#include "Tag_DistanceMatch.generated.h"

UCLASS(editinlinenew, hidecategories = Object, collapsecategories)
class MOTIONSYMPHONY_API UTag_DistanceMatch : public UTagSection
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere)
	EDistanceMatchType DistanceMatchType;

	UPROPERTY(EditAnywhere)
	EDistanceMatchBasis DistancematchBasis;

public:
	virtual void PreProcessTag(FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData) override;
	virtual void PreProcessPose(FPoseMotionData& OutPose, FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData) override;
};