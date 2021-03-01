#pragma once

#include "CoreMinimal.h"
#include "TagSection.h"
#include "Tag_Trait.generated.h"

UCLASS(editinlinenew, hidecategories = Object, collapsecategories)
class MOTIONSYMPHONY_API UTag_Trait : public UTagSection
{
	GENERATED_UCLASS_BODY()
public:
	UPROPERTY(EditAnywhere)
	FString TraitName;

public:
	virtual void PreProcessTag(FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData) override;
	virtual void PreProcessPose(FPoseMotionData& OutPose, FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData) override;
};