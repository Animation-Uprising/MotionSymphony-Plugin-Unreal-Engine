// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimNode_PoseMatchBase.h"
#include "Animation/AnimSequence.h"
#include "AnimNode_TransitionMatching.generated.h"

UENUM(BlueprintType)
enum class ETransitionDirectionMethod : uint8
{
	Manual,
	RootMotion
};

USTRUCT(BlueprintInternalUseOnly)
struct MOTIONSYMPHONY_API FTransitionAnimData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* AnimSequence;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETransitionDirectionMethod TransitionDirectionMethod;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = -180.0f, ClampMax = 180.0f))
	float StartDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = -180.0f, ClampMax = 180.0f))
	float EndDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0.0f))
	float Favour;

	UPROPERTY()
	int32 StartPose;

	UPROPERTY()
	int32 EndPose;

public:
	FTransitionAnimData();
};

USTRUCT(BlueprintInternalUseOnly)
struct MOTIONSYMPHONY_API FAnimNode_TransitionMatching : public FAnimNode_PoseMatchBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inputs, meta = (PinShownByDefault))
	float CurrentDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inputs, meta = (PinShownByDefault))
	float DesiredDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TransitionSettings)
	float DirectionTolerance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TransitionSettings)
	float StartDirectionWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TransitionSettings)
	float EndDirectionWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TransitionSettings)
	bool bUseDistanceMatching;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	TArray<FTransitionAnimData> TransitionAnimData;

public:
	FAnimNode_TransitionMatching();

	virtual void PreProcess() override;

protected:
	virtual void FindMatchPose(const FAnimationUpdateContext& Context) override;
	virtual UAnimSequenceBase* FindActiveAnim() override;
};