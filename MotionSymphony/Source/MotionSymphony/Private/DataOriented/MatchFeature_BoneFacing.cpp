#include "DataOriented/MatchFeature_BoneFacing.h"
#include "MirroringProfile.h"
#include "MMPreProcessUtils.h"
#include "MotionAnimAsset.h"
#include "MotionDataAsset.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/DebugSkelMeshComponent.h"

bool UMatchFeature_BoneFacing::IsMotionSnapshotCompatible() const
{
	return true;
}

int32 UMatchFeature_BoneFacing::Size() const
{
	return 3;
}

void UMatchFeature_BoneFacing::EvaluatePreProcess(float* ResultLocation, FMotionAnimSequence& InSequence,
                                                    const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile)
{
	if(!InSequence.Sequence)
	{
		*ResultLocation = 0.0f;
		++ResultLocation;
		*ResultLocation = 0.0f;
		++ResultLocation;
		*ResultLocation = 0.0f;
		return;
	}
	
	FTransform JointTransform_CS = FTransform::Identity;
	TArray<FName> BonesToRoot;
	FName BoneName = BoneReference.BoneName;
	
	if(bMirror && MirrorProfile)
	{
		BoneName = MirrorProfile->FindBoneMirror(BoneReference.BoneName);
	}

	FMMPreProcessUtils::FindBonePathToRoot(InSequence.Sequence, BoneName, BonesToRoot);
	BonesToRoot.RemoveAt(BonesToRoot.Num() - 1); //Removes the root
	
	FMMPreProcessUtils::GetJointTransform_RootRelative(JointTransform_CS, InSequence.Sequence, BonesToRoot, Time);
	const FVector BoneFacing = JointTransform_CS.GetUnitAxis(EAxis::X);
	
	*ResultLocation = bMirror? -BoneFacing.X : BoneFacing.X;
	++ResultLocation;
	*ResultLocation = BoneFacing.Y;
	++ResultLocation;
	*ResultLocation = BoneFacing.Z;
}

void UMatchFeature_BoneFacing::EvaluatePreProcess(float* ResultLocation, FMotionComposite& InComposite,
                                                    const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile)
{
	if(!InComposite.AnimComposite)
	{
		*ResultLocation = 0.0f;
		++ResultLocation;
		*ResultLocation = 0.0f;
		++ResultLocation;
		*ResultLocation = 0.0f;
		return;
	}
	
	FTransform JointTransform_CS = FTransform::Identity;
	TArray<FName> BonesToRoot;
	FName BoneName = BoneReference.BoneName;
	
	if(bMirror && MirrorProfile)
	{
		BoneName = MirrorProfile->FindBoneMirror(BoneReference.BoneName);
	}

	FMMPreProcessUtils::FindBonePathToRoot(InComposite.AnimComposite, BoneName, BonesToRoot);
	BonesToRoot.RemoveAt(BonesToRoot.Num() - 1); //Removes the root
	
	FMMPreProcessUtils::GetJointTransform_RootRelative(JointTransform_CS, InComposite.AnimComposite, BonesToRoot, Time);
	const FVector BoneFacing = JointTransform_CS.GetUnitAxis(EAxis::X);
	
	*ResultLocation = bMirror? -BoneFacing.X : BoneFacing.X;
	++ResultLocation;
	*ResultLocation = BoneFacing.Y;
	++ResultLocation;
	*ResultLocation = BoneFacing.Z;
}

void UMatchFeature_BoneFacing::EvaluatePreProcess(float* ResultLocation, FMotionBlendSpace& InBlendSpace,
                                                    const float Time, const float PoseInterval, const bool bMirror,
                                                    UMirroringProfile* MirrorProfile, const FVector2D BlendSpacePosition)
{
	if(!InBlendSpace.BlendSpace)
	{
		*ResultLocation = 0.0f;
		++ResultLocation;
		*ResultLocation = 0.0f;
		++ResultLocation;
		*ResultLocation = 0.0f;
		return;
	}

	TArray<FBlendSample> BlendSamples =  InBlendSpace.BlendSpace->GetBlendSamples();
	TArray<FBlendSampleData> SampleDataList;
	int32 CachedTriangulationIndex = -1;
	InBlendSpace.BlendSpace->GetSamplesFromBlendInput(FVector(BlendSpacePosition.X, BlendSpacePosition.Y, 0.0f),
		SampleDataList, CachedTriangulationIndex, false);
	
	FTransform JointTransform_CS = FTransform::Identity;
	TArray<FName> BonesToRoot;
	FName BoneName = BoneReference.BoneName;
	
	if(bMirror && MirrorProfile)
	{
		BoneName = MirrorProfile->FindBoneMirror(BoneReference.BoneName);
	}

	FMMPreProcessUtils::FindBonePathToRoot(BlendSamples[0].Animation.Get(), BoneName, BonesToRoot);
	BonesToRoot.RemoveAt(BonesToRoot.Num() - 1); //Removes the root
	
	FMMPreProcessUtils::GetJointTransform_RootRelative(JointTransform_CS, SampleDataList, BonesToRoot, Time);
	const FVector BoneFacing = JointTransform_CS.GetUnitAxis(EAxis::X);
	
	*ResultLocation = bMirror? -BoneFacing.X : BoneFacing.X;
	++ResultLocation;
	*ResultLocation = BoneFacing.Y;
	++ResultLocation;
	*ResultLocation = BoneFacing.Z;
}

void UMatchFeature_BoneFacing::CacheMotionBones(FAnimInstanceProxy* InAnimInstanceProxy)
{
	BoneReference.Initialize(InAnimInstanceProxy->GetRequiredBones());
	//BoneReference.GetCompactPoseIndex(InAnimInstanceProxy->GetRequiredBones());
}

void UMatchFeature_BoneFacing::ExtractRuntime(FCSPose<FCompactPose>& CSPose, float* ResultLocation, float* FeatureCacheLocation, FAnimInstanceProxy*
                                              AnimInstanceProxy, float DeltaTime)
{
	const FVector BoneFacing = CSPose.GetComponentSpaceTransform(FCompactPoseBoneIndex(BoneReference.CachedCompactPoseIndex)).GetUnitAxis(EAxis::X);

	*ResultLocation = BoneFacing.X;
	++ResultLocation;
	*ResultLocation = BoneFacing.Y;
	++ResultLocation;
	*ResultLocation = BoneFacing.Z;
}

void UMatchFeature_BoneFacing::CalculateDistanceSqrToMeanArrayForStandardDeviations(TArray<float>& OutDistToMeanSqrArray,
	const TArray<float>& InMeanArray, const TArray<float>& InPoseArray, const int32 FeatureOffset, const int32 PoseStartIndex) const
{
	const FVector Facing
	{
		InPoseArray[PoseStartIndex + FeatureOffset],
		InPoseArray[PoseStartIndex + FeatureOffset + 1],
		InPoseArray[PoseStartIndex + FeatureOffset + 2]
	};

	const FVector MeanFacing
	{
		InMeanArray[FeatureOffset],
		InMeanArray[FeatureOffset + 1],
		InMeanArray[FeatureOffset + 2]
	};

	const float DistanceToMean = FVector::DistSquared(Facing, MeanFacing);

	OutDistToMeanSqrArray[FeatureOffset] += DistanceToMean;
	OutDistToMeanSqrArray[FeatureOffset+1] += DistanceToMean;
	OutDistToMeanSqrArray[FeatureOffset+2] += DistanceToMean;
}

#if WITH_EDITOR
void UMatchFeature_BoneFacing::DrawPoseDebugEditor(UMotionDataAsset* MotionData,
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

	const FVector StartPoint = DebugSkeletalMesh->GetBoneLocation(BoneReference.BoneName, EBoneSpaces::WorldSpace);
	const FVector EndPoint = StartPoint + PreviewTransform.TransformVector(FVector(PoseArray[StartIndex], PoseArray[StartIndex+1], PoseArray[StartIndex+2]) * 0.333f);

	DrawDebugDirectionalArrow(World, StartPoint, EndPoint, 20.0f, FColor::Orange, true, -1, 0, 1.5f);
}

void UMatchFeature_BoneFacing::DrawDebugDesiredRuntime(FAnimInstanceProxy* AnimInstanceProxy,
	FMotionMatchingInputData& InputData, const int32 FeatureOffset, UMotionMatchConfig* MMConfig)
{
	
}

void UMatchFeature_BoneFacing::DrawDebugCurrentRuntime(FAnimInstanceProxy* AnimInstanceProxy,
	UMotionDataAsset* MotionData, TArray<float>& CurrentPoseArray, const int32 FeatureOffset)
{
	if(!AnimInstanceProxy || !MotionData)
	{
		return;
	}

	const USkeletalMeshComponent* SkelMeshComponent = AnimInstanceProxy->GetSkelMeshComponent();
	
	const FTransform PreviewTransform = SkelMeshComponent->GetComponentTransform();
	
	const FVector StartPoint = SkelMeshComponent->GetBoneLocation(BoneReference.BoneName, EBoneSpaces::WorldSpace);
	const FVector EndPoint = StartPoint + PreviewTransform.TransformVector(FVector(CurrentPoseArray[FeatureOffset],
		CurrentPoseArray[FeatureOffset+1], CurrentPoseArray[FeatureOffset+2]) * 0.333f);

	AnimInstanceProxy->AnimDrawDebugDirectionalArrow(StartPoint, EndPoint, 40.0f,
		FColor::Orange, false, -1.0f, 2.0f);
}
#endif