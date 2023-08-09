#include "Objects/MatchFeatures/MatchFeature_BoneAxis.h"
#include "MirroringProfile.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "MMPreProcessUtils.h"
#include "MotionAnimAsset.h"
#include "MotionDataAsset.h"

#if WITH_EDITOR
#include "MotionSymphonySettings.h"
#endif

bool UMatchFeature_BoneAxis::IsMotionSnapshotCompatible() const
{
	return true;
}

int32 UMatchFeature_BoneAxis::Size() const
{
	return 1;
}

void UMatchFeature_BoneAxis::EvaluatePreProcess(float* ResultLocation, FMotionAnimSequence& InSequence,
                                                const float Time, const float PoseInterval, const bool bMirror, UMirrorDataTable* MirrorDataTable)
{
	if(!InSequence.Sequence)
	{
		*ResultLocation = 0.0f;
		return;
	}

	TArray<FName> BonesToRoot;
	FTransform BoneTransform_CS = FTransform::Identity;
	FName BoneName = BoneReference.BoneName;

	if(bMirror && MirrorDataTable)
	{
		BoneName = FMMPreProcessUtils::FindMirrorBoneName(InSequence.Sequence->GetSkeleton(), MirrorDataTable, BoneName);
	}

	FMMPreProcessUtils::FindBonePathToRoot(InSequence.Sequence, BoneName, BonesToRoot);
	BonesToRoot.RemoveAt(BonesToRoot.Num() - 1); //Removes the root
	
	FMMPreProcessUtils::GetJointTransform_RootRelative(BoneTransform_CS, InSequence.Sequence, BonesToRoot, Time);

	switch(Axis)
	{
		case EAxis::Type::X: *ResultLocation = BoneTransform_CS.GetLocation().X; break;
		case EAxis::Type::Y: *ResultLocation = BoneTransform_CS.GetLocation().Y; break;
		case EAxis::Type::Z: *ResultLocation = BoneTransform_CS.GetLocation().Z; break;
		default: *ResultLocation = 0.0f; break;
	}
}

void UMatchFeature_BoneAxis::EvaluatePreProcess(float* ResultLocation, FMotionComposite& InComposite, const float Time,
                                                const float PoseInterval, const bool bMirror, UMirrorDataTable* MirrorDataTable)
{
	if(!InComposite.AnimComposite)
	{
		*ResultLocation = 0.0f;
		return;
	}

	TArray<FName> BonesToRoot;
	FTransform BoneTransform_CS = FTransform::Identity;
	FName BoneName = BoneReference.BoneName;

	if(bMirror && MirrorDataTable)
	{
		BoneName = FMMPreProcessUtils::FindMirrorBoneName(InComposite.AnimComposite->GetSkeleton(), MirrorDataTable, BoneName);
	}

	FMMPreProcessUtils::FindBonePathToRoot(InComposite.AnimComposite, BoneName, BonesToRoot);
	BonesToRoot.RemoveAt(BonesToRoot.Num() - 1); //Removes the root
	
	FMMPreProcessUtils::GetJointTransform_RootRelative(BoneTransform_CS, InComposite.AnimComposite, BonesToRoot, Time);
	
	switch(Axis)
	{
		case EAxis::Type::X: *ResultLocation = BoneTransform_CS.GetLocation().X; break;
		case EAxis::Type::Y: *ResultLocation = BoneTransform_CS.GetLocation().Y; break;
		case EAxis::Type::Z: *ResultLocation = BoneTransform_CS.GetLocation().Z; break;
		default: *ResultLocation = 0.0f; break;
	}
}

void UMatchFeature_BoneAxis::EvaluatePreProcess(float* ResultLocation, FMotionBlendSpace& InBlendSpace,
                                                const float Time, const float PoseInterval, const bool bMirror, UMirrorDataTable* MirrorDataTable,
                                                const FVector2D BlendSpacePosition)
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
	

	if(bMirror && MirrorDataTable)
	{
		BoneName = FMMPreProcessUtils::FindMirrorBoneName(InBlendSpace.BlendSpace->GetSkeleton(), MirrorDataTable, BoneName);
	}

	FMMPreProcessUtils::FindBonePathToRoot(InBlendSpace.BlendSpace->GetBlendSamples()[0].Animation, BoneName, BonesToRoot);
	BonesToRoot.RemoveAt(BonesToRoot.Num() - 1); //Removes the root
	
	FMMPreProcessUtils::GetJointTransform_RootRelative(BoneTransform_CS, SampleDataList, BonesToRoot, Time);
	
	switch(Axis)
	{
		case EAxis::Type::X: *ResultLocation = BoneTransform_CS.GetLocation().X; break;
		case EAxis::Type::Y: *ResultLocation = BoneTransform_CS.GetLocation().Y; break;
		case EAxis::Type::Z: *ResultLocation = BoneTransform_CS.GetLocation().Z; break;
		default: *ResultLocation = 0.0f; break;
	}
}

void UMatchFeature_BoneAxis::CacheMotionBones(FAnimInstanceProxy* InAnimInstanceProxy)
{
	BoneReference.Initialize(InAnimInstanceProxy->GetRequiredBones());
}

void UMatchFeature_BoneAxis::ExtractRuntime(FCSPose<FCompactPose>& CSPose, float* ResultLocation,
	float* FeatureCacheLocation, FAnimInstanceProxy* AnimInstanceProxy, float DeltaTime)
{
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
	UMotionDataAsset* MotionData, TArray<float>& CurrentPoseArray, const int32 FeatureOffset)
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