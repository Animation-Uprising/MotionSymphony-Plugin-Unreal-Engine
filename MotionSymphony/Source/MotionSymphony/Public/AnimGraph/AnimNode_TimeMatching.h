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
#include "DistanceMatching.h"
#include "AnimNode_TimeMatching.generated.h"

USTRUCT(BlueprintInternalUseOnly)
struct MOTIONSYMPHONY_API FAnimNode_TimeMatching : public FAnimNode_SequencePlayer
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inputs, meta = (PinShownByDefault, ClampMin = 0.0f))
	float DesiredTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	float MarkerTime;

private:
	bool bInitialized;

	UDistanceMatching* DistanceMatching;
	FAnimInstanceProxy* AnimInstanceProxy;

public:
	FAnimNode_TimeMatching();

protected:
	float FindMatchingTime();

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	// End of FAnimNode_Base interface
};