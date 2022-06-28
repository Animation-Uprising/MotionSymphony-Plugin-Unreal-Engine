#include "DataOriented/MatchFeature_BodyMomentumRot.h"
#include "MMPreProcessUtils.h"
#include "MotionAnimAsset.h"

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
	Super::DrawPoseDebugEditor(MotionData, DebugSkeletalMesh, PreviewIndex, FeatureOffset, World, DrawInterface);
}

void UMatchFeature_BodyMomentumRot::DrawDebugDesiredRuntime(FAnimInstanceProxy* AnimInstanceProxy,
	FMotionMatchingInputData& InputData, const int32 FeatureOffset, UMotionMatchConfig* MMConfig)
{
	Super::DrawDebugDesiredRuntime(AnimInstanceProxy, InputData, FeatureOffset, MMConfig);
}
