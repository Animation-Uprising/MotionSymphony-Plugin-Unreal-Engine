// Fill out your copyright notice in the Description page of Project Settings.

#include "AnimNotifies/AnimNotify_MSFootLockTimer.h"
#include "MSFootLockManager.h"

#if ENGINE_MAJOR_VERSION >= 5
void UAnimNotify_MSFootLockTimer::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                       const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
#else
void UAnimNotify_FootLockTimer::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
#endif
	
	if(UMSFootLockManager* FootLockManager = Cast<UMSFootLockManager>(MeshComp->GetOwner()->GetComponentByClass(UMSFootLockManager::StaticClass())))
	{
		FootLockManager->LockFoot(FootLockId, GroundingTime);
	}
}