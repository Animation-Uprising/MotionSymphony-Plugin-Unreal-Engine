//Copyright 2020-2023 Kenneth Claassen. All Rights Reserved.

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

	if(!MeshComp
	   || !Animation)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if(!Owner)
	{
		return;
	}
	
	if(UMSFootLockManager* FootLockManager = Cast<UMSFootLockManager>(Owner->GetComponentByClass(UMSFootLockManager::StaticClass())))
	{
		FootLockManager->LockFoot(FootLockId, GroundingTime);
	}
}