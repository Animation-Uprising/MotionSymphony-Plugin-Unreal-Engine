// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "Animation/AnimSequence.h"
#include "Curves/RichCurve.h"
#include "UObject/NameTypes.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/InputScaleBias.h"
#include "Components/DistanceMatching.h"
#include "AnimNode_DistanceMatching.generated.h"

USTRUCT(BlueprintInternalUseOnly)
struct MOTIONSYMPHONY_API FAnimNode_DistanceMatching : public FAnimNode_SequencePlayer
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inputs, meta = (PinShownByDefault, ClampMin = 0.0f))
	float DesiredDistance; //+ve if the marker is ahead, -ve if the marker is behind

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	FName DistanceCurveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	EDistanceMatchType MovementType;

private:
	bool bInitialized;

	FDistanceMatchingModule DistanceMatchingModule;

	uint32 DistanceMatchInstanceId;
	uint32 ActiveDistanceMatchInstanceId;
	EDistanceMatchType DistanceMatchType;

	UDistanceMatching* DistanceMatching;
	FAnimInstanceProxy* AnimInstanceProxy;

public:
	FAnimNode_DistanceMatching();

protected:
	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
	// End of FAnimNode_Base interface
};