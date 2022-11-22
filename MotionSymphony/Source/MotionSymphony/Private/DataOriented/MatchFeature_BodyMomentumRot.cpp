#include "DataOriented/MatchFeature_BodyMomentumRot.h"
#include "MMPreProcessUtils.h"
#include "MotionAnimAsset.h"
#include "MotionDataAsset.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/DebugSkelMeshComponent.h"

int32 UMatchFeature_BodyMomentumRot::Size() const
{
	return 1;
}

void UMatchFeature_BodyMomentumRot::EvaluatePreProcess(float* ResultLocation, FMotionAnimSequence& InSequence,
                                                       const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile)
{
	if(!InSequence.Sequence)
	{
		*ResultLocation = 0.0f;
		return;
	}
	
	FVector RootVelocity;
	float RootRotVelocity;
	FMMPreProcessUtils::ExtractRootVelocity(RootVelocity, RootRotVelocity, InSequence.Sequence, Time, PoseInterval);
	
	RootRotVelocity *= InSequence.PlayRate;
	*ResultLocation = bMirror ? -RootRotVelocity : RootRotVelocity;
}

void UMatchFeature_BodyMomentumRot::EvaluatePreProcess(float* ResultLocation, FMotionComposite& InComposite,
                                                       const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile)
{
	if(!InComposite.AnimComposite)
	{
		*ResultLocation = 0.0f;
		return;
	}
	
	FVector RootVelocity;
	float RootRotVelocity;
	FMMPreProcessUtils::ExtractRootVelocity(RootVelocity, RootRotVelocity, InComposite.AnimComposite, Time, PoseInterval);
	
	RootRotVelocity *= InComposite.PlayRate;
	 *ResultLocation = bMirror ? -RootRotVelocity : RootRotVelocity;
}

void UMatchFeature_BodyMomentumRot::EvaluatePreProcess(float* ResultLocation, FMotionBlendSpace& InBlendSpace,
                                                       const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile, const FVector2D BlendSpacePosition)
{
	if(!InBlendSpace.BlendSpace)
	{
		*ResultLocation = 0.0f;
		return;
	}

	TArray<FBlendSampleData> SampleDataList;
	int32 CachedTriangulationIndex = -1;
	InBlendSpace.BlendSpace->GetSamplesFromBlendInput(FVector(BlendSpacePosition.X, BlendSpacePosition.Y, 0.0f),
		SampleDataList, CachedTriangulationIndex, false);

	
	 FVector RootVelocity;
	 float RootRotVelocity;
	 FMMPreProcessUtils::ExtractRootVelocity(RootVelocity, RootRotVelocity, SampleDataList, Time, PoseInterval);
	
	RootRotVelocity *= InBlendSpace.PlayRate;
	*ResultLocation = bMirror ? -RootRotVelocity : RootRotVelocity;
}

void UMatchFeature_BodyMomentumRot::DrawPoseDebugEditor(UMotionDataAsset* MotionData,
	UDebugSkelMeshComponent* DebugSkeletalMesh, const int32 PreviewIndex, const int32 FeatureOffset,
	const UWorld* World, FPrimitiveDrawInterface* DrawInterface)
{
	if(!MotionData || !DebugSkeletalMesh ||!World)
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
	const FVector RotatePoint = PreviewTransform.GetLocation();
	const FVector StartPoint = RotatePoint + PreviewTransform.TransformPosition(FVector(50.0f, 0.0f, 0.0f));
	const FVector EndPoint = StartPoint + PreviewTransform.TransformPosition(FVector(0.0f, (1.0f / 180.0f) * PoseArray[StartIndex], 0.0f));

	DrawDebugCircle(World, RotatePoint, 50.0f, 32, FColor::Magenta, true, -1, 0, 1.5, FVector::LeftVector, FVector::ForwardVector, false);
	DrawDebugDirectionalArrow(World, StartPoint, EndPoint, 20.0f, FColor::Magenta, true, -1, 0, 1.5f);
}

void UMatchFeature_BodyMomentumRot::DrawDebugCurrentRuntime(FAnimInstanceProxy* AnimInstanceProxy,
	UMotionDataAsset* MotionData, TArray<float>& CurrentPoseArray, const int32 FeatureOffset)
{
	if(!AnimInstanceProxy || !MotionData)
	{
		return;
	}

	const FTransform& MeshTransform = AnimInstanceProxy->GetSkelMeshComponent()->GetComponentTransform();
	const FVector MeshLocation = MeshTransform.GetLocation();
	const FVector StartPoint = MeshLocation + MeshTransform.TransformPosition(FVector(50.0f, 0.0f, 0.0f));
	const FVector EndPoint = StartPoint + MeshTransform.TransformPosition(FVector(0.0f, (1.0f / 180.0f) * CurrentPoseArray[FeatureOffset], 0.0f));
	

	AnimInstanceProxy->AnimDrawDebugSphere(StartPoint, 5.0f, 32, FColor::Magenta, false, -1.0f, 0.0f);
	AnimInstanceProxy->AnimDrawDebugDirectionalArrow(StartPoint, EndPoint, 80.0f, FColor::Magenta, false, -1.0f, 3.0f);
}
