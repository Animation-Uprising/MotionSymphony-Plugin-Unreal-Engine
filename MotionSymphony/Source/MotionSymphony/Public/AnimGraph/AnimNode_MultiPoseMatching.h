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

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DistanceMatching", meta = (PinShownByDefault))
	// float DesiredDistance;
	//
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DistanceMatching")
	// FName DistanceCurveName;
	//
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DistanceMatching")
	// EDistanceMatchType MovementType;

public:
	FAnimNode_MultiPoseMatching();

	virtual UAnimSequenceBase* FindActiveAnim() override;

#if WITH_EDITOR
	virtual void PreProcess() override;
#endif

protected:
	virtual USkeleton* GetNodeSkeleton() override;
};