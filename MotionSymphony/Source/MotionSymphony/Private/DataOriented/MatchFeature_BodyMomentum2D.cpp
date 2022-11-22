#include "DataOriented/MatchFeature_BodyMomentum2D.h"
#include "MMPreProcessUtils.h"
#include "MotionAnimAsset.h"
#include "MotionDataAsset.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/DebugSkelMeshComponent.h"

int32 UMatchFeature_BodyMomentum2D::Size() const
{
	return 2;
}

void UMatchFeature_BodyMomentum2D::EvaluatePreProcess(float* ResultLocation, FMotionAnimSequence& InSequence,
                                                      const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile)
{
	if(!InSequence.Sequence)
	{
		*ResultLocation = 0.0f;
		++ResultLocation;
		*ResultLocation = 0.0f;
		return;
	}
	
	FVector RootVelocity;
	float RootRotVelocity;
	FMMPreProcessUtils::ExtractRootVelocity(RootVelocity, RootRotVelocity, InSequence.Sequence, Time, PoseInterval);

	RootVelocity *= InSequence.PlayRate;
	
	*ResultLocation = bMirror ? -RootVelocity.X : RootVelocity.X;
	++ResultLocation;
	*ResultLocation = RootVelocity.Y;
}

void UMatchFeature_BodyMomentum2D::EvaluatePreProcess(float* ResultLocation, FMotionComposite& InComposite,
                                                      const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile)
{
	if(!InComposite.AnimComposite)
	{
		*ResultLocation = 0.0f;
		++ResultLocation;
		*ResultLocation = 0.0f;
		return;
	}
	
	FVector RootVelocity;
	float RootRotVelocity;
	FMMPreProcessUtils::ExtractRootVelocity(RootVelocity, RootRotVelocity, InComposite.AnimComposite, Time, PoseInterval);

	RootVelocity *= InComposite.PlayRate;
	
	*ResultLocation = bMirror ? -RootVelocity.X : RootVelocity.X;
	++ResultLocation;
	*ResultLocation = RootVelocity.Y;
}

void UMatchFeature_BodyMomentum2D::EvaluatePreProcess(float* ResultLocation, FMotionBlendSpace& InBlendSpace,
                                                      const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile, const FVector2D BlendSpacePosition)
{
	if(!InBlendSpace.BlendSpace)
	{
		*ResultLocation = 0.0f;
		++ResultLocation;
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
	
	RootVelocity *= InBlendSpace.PlayRate;
	
	*ResultLocation = bMirror ? -RootVelocity.X : RootVelocity.X;
	++ResultLocation;
	*ResultLocation = RootVelocity.Y;
}

void UMatchFeature_BodyMomentum2D::DrawPoseDebugEditor(UMotionDataAsset* MotionData,
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
	const FVector StartPoint = PreviewTransform.GetLocation();
	const FVector EndPoint = StartPoint + PreviewTransform.TransformVector(FVector(PoseArray[StartIndex], PoseArray[StartIndex+1], 0.0f));

	DrawDebugCircle(World, StartPoint, 25.0f, 32, FColor::Orange, true, -1, 0, 1.5, FVector::LeftVector, FVector::ForwardVector, false);
	DrawDebugDirectionalArrow(World, StartPoint , EndPoint, 20.0f, FColor::Orange, true, -1, 0, 1.5f);
}

void UMatchFeature_BodyMomentum2D::DrawDebugCurrentRuntime(FAnimInstanceProxy* AnimInstanceProxy,
	UMotionDataAsset* MotionData, TArray<float>& CurrentPoseArray, const int32 FeatureOffset)
{
	if(!AnimInstanceProxy || !MotionData)
	{
		return;
	}

	//const FTransform& ActorTransform = AnimInstanceProxy->GetActorTransform();
	const FTransform& MeshTransform = AnimInstanceProxy->GetSkelMeshComponent()->GetComponentTransform();
	const FVector MeshLocation = MeshTransform.GetLocation();
	
	AnimInstanceProxy->AnimDrawDebugSphere(MeshLocation, 5.0f, 32, FColor::Orange, false, -1.0f, 0.0f);
	AnimInstanceProxy->AnimDrawDebugDirectionalArrow(MeshLocation, MeshLocation + MeshTransform.TransformVector(
		FVector(CurrentPoseArray[FeatureOffset], CurrentPoseArray[FeatureOffset+1], MeshLocation.Z)),
		80.0f, FColor::Orange, false, -1.0f, 3.0f);
}
