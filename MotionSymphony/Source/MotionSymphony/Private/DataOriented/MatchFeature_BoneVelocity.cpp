#include "DataOriented/MatchFeature_BoneVelocity.h"
#include "MirroringProfile.h"
#include "MMPreProcessUtils.h"
#include "MotionAnimAsset.h"
#include "MotionDataAsset.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/DebugSkelMeshComponent.h"

bool UMatchFeature_BoneVelocity::IsMotionSnapshotCompatible() const
{
	return true;
}

int32 UMatchFeature_BoneVelocity::Size() const
{
	return 3;
}

void UMatchFeature_BoneVelocity::EvaluatePreProcess(float* ResultLocation, FMotionAnimSequence& InSequence,
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
	
	const float StartTime = FMath::Max(Time - (PoseInterval / 2.0f), 0.0f);
	
	FTransform BeforeTransform_CS = FTransform::Identity;
	FTransform AfterTransform_CS = FTransform::Identity;
	TArray<FName> BonesToRoot;
	FName BoneName = BoneReference.BoneName;
	
	if(bMirror && MirrorProfile)
	{
		BoneName = MirrorProfile->FindBoneMirror(BoneReference.BoneName);
	}
	
	FMMPreProcessUtils::FindBonePathToRoot(InSequence.Sequence, BoneName, BonesToRoot);
	BonesToRoot.RemoveAt(BonesToRoot.Num() - 1); //Removes the root

	FMMPreProcessUtils::GetJointTransform_RootRelative(BeforeTransform_CS, InSequence.Sequence, BonesToRoot, StartTime);
	FMMPreProcessUtils::GetJointTransform_RootRelative(AfterTransform_CS, InSequence.Sequence, BonesToRoot, StartTime + PoseInterval);

	const FVector BoneVelocity = ((AfterTransform_CS.GetLocation() - BeforeTransform_CS.GetLocation()) * InSequence.PlayRate) / PoseInterval;
	
	*ResultLocation = bMirror? -BoneVelocity.X : BoneVelocity.X;
	++ResultLocation;
	*ResultLocation = BoneVelocity.Y;
	++ResultLocation;
	*ResultLocation = BoneVelocity.Z;
}

void UMatchFeature_BoneVelocity::EvaluatePreProcess(float* ResultLocation, FMotionComposite& InComposite,
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

	const float StartTime = FMath::Min(Time - (PoseInterval / 2.0f), 0.0f);

	FTransform BeforeTransform_CS = FTransform::Identity;
	FTransform AfterTransform_CS = FTransform::Identity;
	TArray<FName> BonesToRoot;
	FName BoneName = BoneReference.BoneName;
	
	if(bMirror && MirrorProfile)
	{
		BoneName = MirrorProfile->FindBoneMirror(BoneReference.BoneName);
	}

	FMMPreProcessUtils::FindBonePathToRoot(InComposite.AnimComposite, BoneName, BonesToRoot);
	BonesToRoot.RemoveAt(BonesToRoot.Num() - 1); //Removes the root

	FMMPreProcessUtils::GetJointTransform_RootRelative(BeforeTransform_CS, InComposite.AnimComposite, BonesToRoot, StartTime);
	FMMPreProcessUtils::GetJointTransform_RootRelative(AfterTransform_CS, InComposite.AnimComposite, BonesToRoot, StartTime + PoseInterval);

	const FVector BoneVelocity = ((AfterTransform_CS.GetLocation() - BeforeTransform_CS.GetLocation()) * InComposite.PlayRate) / PoseInterval;
	
	*ResultLocation = bMirror? -BoneVelocity.X : BoneVelocity.X;
	++ResultLocation;
	*ResultLocation = BoneVelocity.Y;
	++ResultLocation;
	*ResultLocation = BoneVelocity.Z;
}

void UMatchFeature_BoneVelocity::EvaluatePreProcess(float* ResultLocation, FMotionBlendSpace& InBlendSpace,
                                                    const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile, const FVector2D BlendSpacePosition)
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
	
	const float StartTime = FMath::Min(Time - (PoseInterval / 2.0f), 0.0f);

	FTransform BeforeTransform_CS = FTransform::Identity;
	FTransform AfterTransform_CS = FTransform::Identity;
	TArray<FName> BonesToRoot;
	FName BoneName = BoneReference.BoneName;

	TArray<FBlendSampleData> SampleDataList;
	int32 CachedTriangulationIndex = -1;
	InBlendSpace.BlendSpace->GetSamplesFromBlendInput(FVector(BlendSpacePosition.X, BlendSpacePosition.Y, 0.0f),
		SampleDataList, CachedTriangulationIndex, false);

	if(bMirror && MirrorProfile)
	{
		BoneName = MirrorProfile->FindBoneMirror(BoneReference.BoneName);
	}

	FMMPreProcessUtils::FindBonePathToRoot(InBlendSpace.BlendSpace->GetBlendSamples()[0].Animation.Get(), BoneName, BonesToRoot);
	BonesToRoot.RemoveAt(BonesToRoot.Num() - 1); //Removes the root
	
	FMMPreProcessUtils::GetJointTransform_RootRelative(BeforeTransform_CS, SampleDataList, BonesToRoot, StartTime);
	FMMPreProcessUtils::GetJointTransform_RootRelative(AfterTransform_CS, SampleDataList, BonesToRoot, StartTime + PoseInterval);
	const FVector BoneVelocity = ((AfterTransform_CS.GetLocation() - BeforeTransform_CS.GetLocation()) * InBlendSpace.PlayRate) / PoseInterval;
	
	*ResultLocation = bMirror? -BoneVelocity.X : BoneVelocity.X;
	++ResultLocation;
	*ResultLocation = BoneVelocity.Y;
	++ResultLocation;
	*ResultLocation = BoneVelocity.Z;
}

void UMatchFeature_BoneVelocity::CacheMotionBones(FAnimInstanceProxy* InAnimInstanceProxy)
{
	BoneReference.Initialize(InAnimInstanceProxy->GetRequiredBones());
	//BoneReference.GetCompactPoseIndex(InAnimInstanceProxy->GetRequiredBones());
}

void UMatchFeature_BoneVelocity::ExtractRuntime(FCSPose<FCompactPose>& CSPose, float* ResultLocation, float DeltaTime)
{
	const FVector BoneLocation = CSPose.GetComponentSpaceTransform(BoneReference.CachedCompactPoseIndex).GetLocation();
	const FVector LastBoneLocation(*ResultLocation , *(ResultLocation+1), *(ResultLocation + 2));
	const FVector Velocity = (BoneLocation - LastBoneLocation) / FMath::Max(0.00001f, DeltaTime);
	
	*ResultLocation = Velocity.X;
	++ResultLocation;
	*ResultLocation = Velocity.Y;
	++ResultLocation;
	*ResultLocation = Velocity.Z;
}

void UMatchFeature_BoneVelocity::DrawPoseDebugEditor(UMotionDataAsset* MotionData,
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

	DrawDebugDirectionalArrow(World, StartPoint, EndPoint, 20.0f, FColor::Blue, true, -1, 0, 1.5f);
}

void UMatchFeature_BoneVelocity::DrawDebugDesiredRuntime(FAnimInstanceProxy* AnimInstanceProxy,
	FMotionMatchingInputData& InputData, const int32 FeatureOffset, UMotionMatchConfig* MMConfig)
{
	
}

void UMatchFeature_BoneVelocity::DrawDebugCurrentRuntime(FAnimInstanceProxy* AnimInstanceProxy,
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
		FColor::Yellow, false, -1.0f, 2.0f);
}

