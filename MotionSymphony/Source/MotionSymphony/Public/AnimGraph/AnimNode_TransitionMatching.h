// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimNode_PoseMatchBase.h"
#include "AnimNode_TransitionMatching.generated.h"

UENUM(BlueprintType)
enum class ETransitionDirectionMethod : uint8
{
	Manual,
	RootMotion
};

UENUM(BlueprintType)
enum class ETransitionMatchingOrder : uint8
{
	TransitionPriority,
	PoseAndTransitionCombined
};

USTRUCT(BlueprintInternalUseOnly)
struct MOTIONSYMPHONY_API FTransitionAnimData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Animation)
	UAnimSequence* AnimSequence;

	UPROPERTY(EditAnywhere, Category = Animation)
	FVector CurrentMove;

	UPROPERTY(EditAnywhere, Category = Animation)
	FVector DesiredMove;

	UPROPERTY(EditAnywhere, Category = Animation)
	ETransitionDirectionMethod TransitionDirectionMethod;

	UPROPERTY(EditAnywhere, Category = Animation, meta = (ClampMin = 0.0f))
	float CostMultiplier;

	UPROPERTY(EditAnywhere, Category = Animation)
	bool bMirror;

	UPROPERTY()
	int32 StartPose;

	UPROPERTY()
	int32 EndPose;

public:
	FTransitionAnimData();
	FTransitionAnimData(const FTransitionAnimData& CopyTransition, bool bInMirror = false);
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

	UPROPERTY(EditAnywhere, Category = TransitionSettings)
	ETransitionMatchingOrder TransitionMatchingOrder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TransitionSettings)
	float StartDirectionWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TransitionSettings)
	float EndDirectionWeight;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TransitionSettings)
	//bool bUseDistanceMatching;

	UPROPERTY(EditAnywhere, Category = Animation)
	TArray<FTransitionAnimData> TransitionAnimData;

protected:
	UPROPERTY()
	TArray<FTransitionAnimData> MirroredTransitionAnimData;

public:
	FAnimNode_TransitionMatching();

#if WITH_EDITOR
	virtual void PreProcess() override;
#endif

protected:
	virtual void FindMatchPose(const FAnimationUpdateContext& Context) override;
	virtual UAnimSequenceBase* FindActiveAnim() override;

	int32 GetMinimaCostPoseId_TransitionPriority();
	int32 GetMinimaCostPoseId_PoseTransitionWeighted();

	int32 GetAnimationIndex(UAnimSequence* AnimSequence);
};