// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

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
	UPROPERTY(EditAnywhere)
	UAnimSequence* AnimSequence;

	UPROPERTY(EditAnywhere, meta = (ClampMin = -180.0f, ClampMax = 180.0f))
	FVector CurrentMoveVector;

	UPROPERTY(EditAnywhere, meta = (ClampMin = -180.0f, ClampMax = 180.0f))
	FVector DesiredMoveVector;

	UPROPERTY(EditAnywhere)
	ETransitionDirectionMethod TransitionDirectionMethod;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0.0f))
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
	/** Node Input: The current character movement vector relative to the character's current facing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inputs, meta = (PinShownByDefault))
	FVector CurrentMoveVector;

	/** Node Input: The desired character movement vector relative to the character's current facing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inputs, meta = (PinShownByDefault))
	FVector DesiredMoveVector;

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

#if WITH_EDITOR
	virtual void PreProcess() override;
#endif

protected:
	virtual void FindMatchPose(const FAnimationUpdateContext& Context) override;
	virtual UAnimSequenceBase* FindActiveAnim() override;
};