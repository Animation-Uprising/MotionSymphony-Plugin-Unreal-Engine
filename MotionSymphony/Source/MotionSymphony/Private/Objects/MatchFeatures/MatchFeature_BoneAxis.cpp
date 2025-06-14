//Copyright 2020-2023 Kenneth Claassen. All Rights Reserved.

#include "Objects/MatchFeatures/MatchFeature_BoneAxis.h"
#include "Animation/AnimInstanceProxy.h"
#include "MMPreProcessUtils.h"
#include "MotionAnimAsset.h"
#include "MotionDataAsset.h"

#if WITH_EDITOR
#include "Animation/DebugSkelMeshComponent.h"
#include "MotionSymphonySettings.h"
#endif

bool UMatchFeature_BoneAxis::IsSetupValid() const
{
	bool bIsValid = true;
	
	if(BoneReference.BoneName.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("UMatchFeature_BoneAxis: Setup is invalid, Bone name is 'NONE'"));
		bIsValid = false;
	}

	if(Axis == EAxis::None)
	{
		UE_LOG(LogTemp, Error, TEXT("UMatchFeature_BoneAxis: Setup is invalid, Bone axis is 'NONE'"));
		bIsValid = false;
	}

	
	return bIsValid;
}

bool UMatchFeature_BoneAxis::IsMotionSnapshotCompatible() const
{
	return true;
}

int32 UMatchFeature_BoneAxis::Size() const
{
	return 1;
}

void UMatchFeature_BoneAxis::EvaluatePreProcess(float* ResultLocation, UAnimSequence* InSequence,
                                                const float Time, const float PoseInterval, const bool bMirror, UMirrorDataTable* MirrorDataTable, ::TObjectPtr<
                                                UMotionAnimObject> InMotionObject)
{
	if(!InSequence)
	{
		*ResultLocation = 0.0f;
		return;
	}

	TArray<FName> BonesToRoot;
	FTransform BoneTransform_CS = FTransform::Identity;
	FName BoneName = BoneReference.BoneName;

	if(bMirror && MirrorDataTable)
	{
		BoneName = FMMPreProcessUtils::FindMirrorBoneName(InSequence->GetSkeleton(), MirrorDataTable, BoneName);
	}

	FMMPreProcessUtils::FindBonePathToRoot(InSequence, BoneName, BonesToRoot);
	if(BonesToRoot.Num() > 0)
	{
		BonesToRoot.RemoveAt(BonesToRoot.Num() - 1); //Removes the root
	}
	
	FMMPreProcessUtils::GetJointTransform_RootRelative(BoneTransform_CS, InSequence, BonesToRoot, Time);

	switch(Axis)
	{
		case EAxis::Type::X: *ResultLocation = BoneTransform_CS.GetLocation().X; break;
		case EAxis::Type::Y: *ResultLocation = BoneTransform_CS.GetLocation().Y; break;
		case EAxis::Type::Z: *ResultLocation = BoneTransform_CS.GetLocation().Z; break;
		default: *ResultLocation = 0.0f; break;
	}
}

void UMatchFeature_BoneAxis::EvaluatePreProcess(float* ResultLocation, UAnimComposite* InComposite, const float Time,
                                                const float PoseInterval, const bool bMirror, UMirrorDataTable* MirrorDataTable, TObjectPtr<UMotionAnimObject>
                                                InAnimObject)
{
	if(!InComposite)
	{
		*ResultLocation = 0.0f;
		return;
	}

	TArray<FName> BonesToRoot;
	FTransform BoneTransform_CS = FTransform::Identity;
	FName BoneName = BoneReference.BoneName;

	if(bMirror && MirrorDataTable)
	{
		BoneName = FMMPreProcessUtils::FindMirrorBoneName(InComposite->GetSkeleton(), MirrorDataTable, BoneName);
	}

	FMMPreProcessUtils::FindBonePathToRoot(InComposite, BoneName, BonesToRoot);
	if(BonesToRoot.Num() > 0)
    {
        BonesToRoot.RemoveAt(BonesToRoot.Num() - 1); //Removes the root
    }
	
	FMMPreProcessUtils::GetJointTransform_RootRelative(BoneTransform_CS, InComposite, BonesToRoot, Time);
	
	switch(Axis)
	{
		case EAxis::Type::X: *ResultLocation = BoneTransform_CS.GetLocation().X; break;
		case EAxis::Type::Y: *ResultLocation = BoneTransform_CS.GetLocation().Y; break;
		case EAxis::Type::Z: *ResultLocation = BoneTransform_CS.GetLocation().Z; break;
		default: *ResultLocation = 0.0f; break;
	}
}

void UMatchFeature_BoneAxis::EvaluatePreProcess(float* ResultLocation, UBlendSpace* InBlendSpace,
                                                const float Time, const float PoseInterval, const bool bMirror, UMirrorDataTable* MirrorDataTable,
                                                const FVector2D BlendSpacePosition, TObjectPtr<UMotionAnimObject> InAnimObject)
{
	if(!InBlendSpace)
	{
		*ResultLocation = 0.0f;
		return;
	}

	TArray<FName> BonesToRoot;
	FTransform BoneTransform_CS = FTransform::Identity;
	FName BoneName = BoneReference.BoneName;
	
	TArray<FBlendSampleData> SampleDataList;
	int32 CachedTriangulationIndex = -1;
	InBlendSpace->GetSamplesFromBlendInput(FVector(BlendSpacePosition.X, BlendSpacePosition.Y, 0.0f),
		SampleDataList, CachedTriangulationIndex, false);
	

	if(bMirror && MirrorDataTable)
	{
		BoneName = FMMPreProcessUtils::FindMirrorBoneName(InBlendSpace->GetSkeleton(), MirrorDataTable, BoneName);
	}

	FMMPreProcessUtils::FindBonePathToRoot(InBlendSpace->GetBlendSamples()[0].Animation, BoneName, BonesToRoot);
	if(BonesToRoot.Num() > 0)
    {
        BonesToRoot.RemoveAt(BonesToRoot.Num() - 1); //Removes the root
    }
	
	FMMPreProcessUtils::GetJointTransform_RootRelative(BoneTransform_CS, SampleDataList, BonesToRoot, Time);
	
	switch(Axis)
	{
		case EAxis::Type::X: *ResultLocation = BoneTransform_CS.GetLocation().X; break;
		case EAxis::Type::Y: *ResultLocation = BoneTransform_CS.GetLocation().Y; break;
		case EAxis::Type::Z: *ResultLocation = BoneTransform_CS.GetLocation().Z; break;
		default: *ResultLocation = 0.0f; break;
	}
}

void UMatchFeature_BoneAxis::CacheMotionBones(const FAnimInstanceProxy* InAnimInstanceProxy)
{
	BoneReference.Initialize(InAnimInstanceProxy->GetRequiredBones());
}

void UMatchFeature_BoneAxis::ExtractRuntime(FCSPose<FCompactPose>& CSPose, float* ResultLocation,
	float* FeatureCacheLocation, FAnimInstanceProxy* AnimInstanceProxy, float DeltaTime)
{
	if(BoneReference.CachedCompactPoseIndex == -1)
	{
		return;
	}
	
	const FVector BoneLocation = CSPose.GetComponentSpaceTransform(FCompactPoseBoneIndex(
		BoneReference.CachedCompactPoseIndex)).GetLocation();

	switch(Axis)
	{
		case EAxis::Type::X: *ResultLocation = BoneLocation.X; break;
		case EAxis::Type::Y: *ResultLocation = BoneLocation.Y; break;
		case EAxis::Type::Z: *ResultLocation = BoneLocation.Z; break;
		default: *ResultLocation = 0.0f; break;
	}
}

bool UMatchFeature_BoneAxis::CanBeQualityFeature() const
{
	return true;
}

#if WITH_EDITOR
void UMatchFeature_BoneAxis::DrawPoseDebugEditor(UMotionDataAsset* MotionData,
	UDebugSkelMeshComponent* DebugSkeletalMesh, const int32 PreviewIndex, const int32 FeatureOffset,
	const UWorld* World, FPrimitiveDrawInterface* DrawInterface)
{
	if(!MotionData || !DebugSkeletalMesh || !World)
	{
		return;
	}

	const TArray<float>& PoseArray = MotionData->LookupPoseMatrix.PoseArray;
	const int32 StartIndex = PreviewIndex * MotionData->LookupPoseMatrix.AtomCount + FeatureOffset;
	
	if(PoseArray.Num() < StartIndex + Size())
	{
		return;
	}

	const UMotionSymphonySettings* Settings = GetMutableDefault<UMotionSymphonySettings>();
	const FColor DebugColor = Settings->DebugColor_JointHeight;

	const FTransform PreviewTransform = DebugSkeletalMesh->GetComponentTransform();
	const FVector BonePos = DebugSkeletalMesh->GetBoneLocation(BoneReference.BoneName, EBoneSpaces::WorldSpace);
	
	DrawDebugBox(World, BonePos, FVector(15.0f, 15.0f, 15.0f), DebugColor, true, -1, -1);
}

void UMatchFeature_BoneAxis::DrawDebugDesiredRuntime(FAnimInstanceProxy* AnimInstanceProxy,
	FMotionMatchingInputData& InputData, const int32 FeatureOffset, UMotionMatchConfig* MMConfig)
{
	
}

void UMatchFeature_BoneAxis::DrawDebugCurrentRuntime(FAnimInstanceProxy* AnimInstanceProxy,
                                                     TObjectPtr<const UMotionDataAsset> MotionData, const TArray<float>& CurrentPoseArray, const int32 FeatureOffset)
{
	if(!MotionData || !AnimInstanceProxy)
	{
		return;
	}

	const UMotionSymphonySettings* Settings = GetMutableDefault<UMotionSymphonySettings>();
	const FColor DebugColor = Settings->DebugColor_JointHeight;

	const FVector BonePos = AnimInstanceProxy->GetSkelMeshComponent()->GetBoneLocation(BoneReference.BoneName, EBoneSpaces::WorldSpace);
	AnimInstanceProxy->AnimDrawDebugSphere(BonePos, 15.0f, 8, DebugColor, false, -1, 0, SDPG_Foreground);
}
#endif