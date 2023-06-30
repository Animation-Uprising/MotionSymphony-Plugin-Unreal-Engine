// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MotionSymphony.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_MSFootLockSingle.generated.h"

/**
 * 
 */
UCLASS()
class MOTIONSYMPHONY_API UAnimNotify_MSFootLockSingle : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	/** Which foot should be locked by this notify*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foot Locker")
	EMSFootLockId FootLockId;

	/** How long should the foot be locked for in seconds*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foot Locker", meta = (ClampMin = 0.0f))
	bool bSetLocked;

public:
#if ENGINE_MAJOR_VERSION >= 5
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
			const FAnimNotifyEventReference& EventReference) override;
#else
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
#endif
};