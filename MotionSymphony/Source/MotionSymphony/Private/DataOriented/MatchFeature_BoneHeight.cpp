#include "DataOriented/MatchFeature_BoneHeight.h"
#include "MirroringProfile.h"
#include "MMPreProcessUtils.h"
#include "MotionAnimAsset.h"
#include "MotionDataAsset.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/DebugSkelMeshComponent.h"

bool UMatchFeature_BoneHeight::IsMotionSnapshotCompatible() const
{
	return true;
}

int32 UMatchFeature_BoneHeight::Size() const
{
	return 1;
}

void UMatchFeature_BoneHeight::EvaluatePreProcess(float* ResultLocation, FMotionAnimSequence& InSequence,
                                                  const float Time, const float PoseInterval, const bool bMirror,
                                                  UMirroringProfile* MirrorProfile)
{
	if(!InSequence.Sequence)
	{
		*ResultLocation = 0.0f;
		return;
	}

	TArray<FName> BonesToRoot;
	FTransform BoneTransform_CS = FTransform::Identity;
	FName BoneName = BoneReference.BoneName;

	if(bMirror && MirrorProfile)
	{
		BoneName = MirrorProfile->FindBoneMirror(BoneReference.BoneName);
	}

	FMMPreProcessUtils::FindBonePathToRoot(InSequence.Sequence, BoneName, BonesToRoot);
	BonesToRoot.RemoveAt(BonesToRoot.Num() - 1); //Removes the root
	
	FMMPreProcessUtils::GetJointTransform_RootRelative(BoneTransform_CS, InSequence.Sequence, BonesToRoot, Time);
	
	*ResultLocation = BoneTransform_CS.GetLocation().Z;
}

void UMatchFeature_BoneHeight::EvaluatePreProcess(float* ResultLocation, FMotionComposite& InComposite,
                                                  const float Time, const float PoseInterval, const bool bMirror,
                                                  UMirroringProfile* MirrorProfile)
{
	if(!InComposite.AnimComposite)
	{
		*ResultLocation = 0.0f;
		return;
	}

	TArray<FName> BonesToRoot;
	FTransform BoneTransform_CS = FTransform::Identity;
	FName BoneName = BoneReference.BoneName;

	if(bMirror && MirrorProfile)
	{
		BoneName = MirrorProfile->FindBoneMirror(BoneReference.BoneName);
	}

	FMMPreProcessUtils::FindBonePathToRoot(InComposite.AnimComposite, BoneName, BonesToRoot);
	BonesToRoot.RemoveAt(BonesToRoot.Num() - 1); //Removes the root
	
	FMMPreProcessUtils::GetJointTransform_RootRelative(BoneTransform_CS, InComposite.AnimComposite, BonesToRoot, Time);
	
	*ResultLocation = BoneTransform_CS.GetLocation().Z;
}

void UMatchFeature_BoneHeight::EvaluatePreProcess(float* ResultLocation, FMotionBlendSpace& InBlendSpace,
                                                  const float Time, const float PoseInterval, const bool bMirror,
                                                  UMirroringProfile* MirrorProfile, const FVector2D BlendSpacePosition)
{
	if(!InBlendSpace.BlendSpace)
	{
		*ResultLocation = 0.0f;
		return;
	}

	TArray<FName> BonesToRoot;
	FTransform BoneTransform_CS = FTransform::Identity;
	FName BoneName = BoneReference.BoneName;
	
	TArray<FBlendSampleData> SampleDataList;
	int32 CachedTriangulationIndex = -1;
	InBlendSpace.BlendSpace->GetSamplesFromBlendInput(FVector(BlendSpacePosition.X, BlendSpacePosition.Y, 0.0f),
		SampleDataList, CachedTriangulationIndex, false);
	

	if(bMirror && MirrorProfile)
	{
		BoneName = MirrorProfile->FindBoneMirror(BoneReference.BoneName);
	}

	FMMPreProcessUtils::FindBonePathToRoot(InBlendSpace.BlendSpace->GetBlendSamples()[0].Animation, BoneName, BonesToRoot);
	BonesToRoot.RemoveAt(BonesToRoot.Num() - 1); //Removes the root
	
	FMMPreProcessUtils::GetJointTransform_RootRelative(BoneTransform_CS, SampleDataList, BonesToRoot, Time);
	
	*ResultLocation = BoneTransform_CS.GetLocation().Z;
}

void UMatchFeature_BoneHeight::CacheMotionBones(FAnimInstanceProxy* InAnimInstanceProxy)
{
	BoneReference.Initialize(InAnimInstanceProxy->GetRequiredBones());
	//BoneReference.GetCompactPoseIndex(InAnimInstanceProxy->GetRequiredBones());
}


void UMatchFeature_BoneHeight::ExtractRuntime(FCSPose<FCompactPose>& CSPose, float* ResultLocation, float* FeatureCacheLocation, FAnimInstanceProxy*
                                              AnimInstanceProxy, float DeltaTime)
{
	const FVector BoneLocation = CSPose.GetComponentSpaceTransform(FCompactPoseBoneIndex(BoneReference.CachedCompactPoseIndex)).GetLocation();
	
	*ResultLocation = BoneLocation.Z;
}

void UMatchFeature_BoneHeight::DrawPoseDebugEditor(UMotionDataAsset* MotionData,
                                                   UDebugSkelMeshComponent* DebugSkeletalMesh, const int32 PreviewIndex, const int32 FeatureOffset,
                                                   const UWorld* World, FPrimitiveDrawInterface* DrawInterface)
{
	if(!MotionData || !DebugSkeletalMesh || !World)
	{
		return;
	}

	TArray<float>& PoseArray = MotionData->LookupPoseMatrix.PoseArray;
	const int32 StartIndex = PreviewIndex * MotionData->LookupPoseMatrix.AtomCount + FeatureOffset;
	
	if(PoseArray.Num() < StartIndex + Size())
	{
		return;
	}

	const FTransform PreviewTransform = DebugSkeletalMesh->GetComponentTransform();
	const FVector BonePos = PreviewTransform.TransformPosition(FVector(0.0f, 0.0f, PoseArray[StartIndex]));
	
	DrawDebugBox(World, BonePos, FVector(15.0f, 15.0f, 15.0f), FColor::Blue, true, -1, 0);
}

void UMatchFeature_BoneHeight::DrawDebugDesiredRuntime(FAnimInstanceProxy* AnimInstanceProxy,
	FMotionMatchingInputData& InputData, const int32 FeatureOffset, UMotionMatchConfig* MMConfig)
{

}

void UMatchFeature_BoneHeight::DrawDebugCurrentRuntime(FAnimInstanceProxy* AnimInstanceProxy,
	UMotionDataAsset* MotionData, TArray<float>& CurrentPoseArray, const int32 FeatureOffset)
{
	if(!MotionData || !AnimInstanceProxy)
	{
		return;
	}

	const FTransform PreviewTransform = AnimInstanceProxy->GetSkelMeshComponent()->GetComponentTransform();
	const FVector BonePos = PreviewTransform.TransformPosition(FVector(0.0f, 0.0f,CurrentPoseArray[FeatureOffset]));
	
	AnimInstanceProxy->AnimDrawDebugSphere(BonePos, 15.0f, 8, FColor::Orange, false, -1, 0);
}
