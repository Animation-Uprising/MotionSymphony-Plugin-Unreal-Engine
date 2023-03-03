// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifies/AnimNotify_MSFootLockSingle.h"
#include "MSFootLockManager.h"

#if ENGINE_MAJOR_VERSION >= 5
void UAnimNotify_MSFootLockSingle::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
									   const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
#else
void UAnimNotify_FootLockSingle::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
#endif

	if(UMSFootLockManager* FootLockManager = Cast<UMSFootLockManager>(MeshComp->GetOwner()->GetComponentByClass(UMSFootLockManager::StaticClass())))
	{
		if(bSetLocked)
		{
			FootLockManager->LockFoot(FootLockId, -1.0f);
		}
		else
		{
			FootLockManager->UnlockFoot(FootLockId); 
		}
	}
}
