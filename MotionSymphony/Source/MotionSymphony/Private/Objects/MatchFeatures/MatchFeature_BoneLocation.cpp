#include "Objects/MatchFeatures/MatchFeature_BoneLocation.h"
#include "MirroringProfile.h"
#include "MMPreProcessUtils.h"
#include "MotionAnimAsset.h"
#include "MotionDataAsset.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/DebugSkelMeshComponent.h"

#if WITH_EDITOR
#include "MotionSymphonySettings.h"
#endif

bool UMatchFeature_BoneLocation::IsMotionSnapshotCompatible() const
{
	return true;
}

int32 UMatchFeature_BoneLocation::Size() const
{
	return 3;
}

void UMatchFeature_BoneLocation::EvaluatePreProcess(float* ResultLocation, FMotionAnimSequence& InSequence,
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
	const FVector BoneLocation = JointTransform_CS.GetLocation();
	
	*ResultLocation = bMirror? -BoneLocation.X : BoneLocation.X;
	++ResultLocation;
	*ResultLocation = BoneLocation.Y;
	++ResultLocation;
	*ResultLocation = BoneLocation.Z;
}

void UMatchFeature_BoneLocation::EvaluatePreProcess(float* ResultLocation, FMotionComposite& InComposite,
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
	const FVector BoneLocation = JointTransform_CS.GetLocation();
	
	*ResultLocation = bMirror? -BoneLocation.X : BoneLocation.X;
	++ResultLocation;
	*ResultLocation = BoneLocation.Y;
	++ResultLocation;
	*ResultLocation = BoneLocation.Z;
}

void UMatchFeature_BoneLocation::EvaluatePreProcess(float* ResultLocation, FMotionBlendSpace& InBlendSpace,
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
	const FVector BoneLocation = JointTransform_CS.GetLocation();
	
	*ResultLocation = bMirror? -BoneLocation.X : BoneLocation.X;
	++ResultLocation;
	*ResultLocation = BoneLocation.Y;
	++ResultLocation;
	*ResultLocation = BoneLocation.Z;
}

void UMatchFeature_BoneLocation::CacheMotionBones(FAnimInstanceProxy* InAnimInstanceProxy)
{
	BoneReference.Initialize(InAnimInstanceProxy->GetRequiredBones());
	//BoneReference.GetCompactPoseIndex(InAnimInstanceProxy->GetRequiredBones());
}

void UMatchFeature_BoneLocation::ExtractRuntime(FCSPose<FCompactPose>& CSPose, float* ResultLocation, float* FeatureCacheLocation, FAnimInstanceProxy*
                                                AnimInstanceProxy, float DeltaTime)
{
	const FVector BoneLocation = CSPose.GetComponentSpaceTransform(FCompactPoseBoneIndex(BoneReference.CachedCompactPoseIndex)).GetLocation();

	*ResultLocation = BoneLocation.X;
	++ResultLocation;
	*ResultLocation = BoneLocation.Y;
	++ResultLocation;
	*ResultLocation = BoneLocation.Z;
}

void UMatchFeature_BoneLocation::CalculateDistanceSqrToMeanArrayForStandardDeviations(TArray<float>& OutDistToMeanSqrArray,
	const TArray<float>& InMeanArray, const TArray<float>& InPoseArray, const int32 FeatureOffset, const int32 PoseStartIndex) const
{
	const FVector Location
	{
		InPoseArray[PoseStartIndex + FeatureOffset],
		InPoseArray[PoseStartIndex + FeatureOffset + 1],
		InPoseArray[PoseStartIndex + FeatureOffset + 2]
	};

	const FVector MeanLocation
	{
		InMeanArray[FeatureOffset],
		InMeanArray[FeatureOffset + 1],
		InMeanArray[FeatureOffset + 2]
	};

	const float DistanceToMean = FVector::DistSquared(Location, MeanLocation);

	OutDistToMeanSqrArray[FeatureOffset] += DistanceToMean;
	OutDistToMeanSqrArray[FeatureOffset+1] += DistanceToMean;
	OutDistToMeanSqrArray[FeatureOffset+2] += DistanceToMean;
}

#if WITH_EDITOR
void UMatchFeature_BoneLocation::DrawPoseDebugEditor(UMotionDataAsset* MotionData,
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

	const UMotionSymphonySettings* Settings = GetMutableDefault<UMotionSymphonySettings>();
	const FColor DebugColor = Settings->DebugColor_Joint;

	const FTransform PreviewTransform = DebugSkeletalMesh->GetComponentTransform();
	const FVector BonePos = PreviewTransform.TransformPosition(FVector(PoseArray[StartIndex],
		PoseArray[StartIndex+1], PoseArray[StartIndex+2]));

	DrawDebugSphere(World, BonePos, 8.0f, 8, DebugColor, true, -1.0f, -1);
}

void UMatchFeature_BoneLocation::DrawDebugDesiredRuntime(FAnimInstanceProxy* AnimInstanceProxy,
	FMotionMatchingInputData& InputData, const int32 FeatureOffset, UMotionMatchConfig* MMConfig)
{
	
}

void UMatchFeature_BoneLocation::DrawDebugCurrentRuntime(FAnimInstanceProxy* AnimInstanceProxy,
	UMotionDataAsset* MotionData, TArray<float>& CurrentPoseArray, const int32 FeatureOffset)
{
	if(!MotionData || !AnimInstanceProxy)
	{
		return;
	}

	const UMotionSymphonySettings* Settings = GetMutableDefault<UMotionSymphonySettings>();
	const FColor DebugColor = Settings->DebugColor_Joint;

	const FTransform PreviewTransform = AnimInstanceProxy->GetSkelMeshComponent()->GetComponentTransform();
	const FVector BonePos = PreviewTransform.TransformPosition(FVector(CurrentPoseArray[FeatureOffset],
		CurrentPoseArray[FeatureOffset+1], CurrentPoseArray[FeatureOffset+2]));

	AnimInstanceProxy->AnimDrawDebugSphere(BonePos, 8.0f, 8, DebugColor, false,
		-1, 0, SDPG_Foreground);
}
#endif