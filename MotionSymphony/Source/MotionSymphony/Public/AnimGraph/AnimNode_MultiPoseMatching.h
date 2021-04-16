// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimNode_PoseMatchBase.h"
#include "Animation/AnimSequence.h"
#include "AnimNode_MultiPoseMatching.generated.h"

USTRUCT(BlueprintInternalUseOnly)
struct MOTIONSYMPHONY_API FAnimNode_MultiPoseMatching : public FAnimNode_PoseMatchBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TArray<UAnimSequence*> Animations;

public:
	FAnimNode_MultiPoseMatching();

	virtual UAnimSequenceBase* FindActiveAnim() override;

#if WITH_EDITOR
	virtual void PreProcess() override;
#endif
};