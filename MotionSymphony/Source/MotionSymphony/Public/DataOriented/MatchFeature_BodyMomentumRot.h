#pragma once

#include "CoreMinimal.h"
#include "MatchFeatureBase.h"
#include "MatchFeature_BodyMomentumRot.generated.h"


struct FPoseMatrix;

/** Motion matching feature which stores and matches bone velocity relative to the character root */
UCLASS(BlueprintType)
class MOTIONSYMPHONY_API UMatchFeature_BodyMomentumRot : public UMatchFeatureBase
{
	GENERATED_BODY()
	
public:
	virtual int32 Size() const override;
	
	virtual void EvaluatePreProcess(float* ResultLocation, FMotionAnimSequence& InSequence,
	                                const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile) override;
	virtual void EvaluatePreProcess(float* ResultLocation, FMotionComposite& InComposite,
	                                const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile) override;
	virtual void EvaluatePreProcess(float* ResultLocation, FMotionBlendSpace& InBlendSpace,
	                                const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile, const FVector2D BlendSpacePosition) override;

	virtual void ExtractRuntime(FCSPose<FCompactPose>& CSPose, float* ResultLocation, float* FeatureCacheLocation, FAnimInstanceProxy*
	                            AnimInstanceProxy, float DeltaTime) override;
	
	virtual void DrawPoseDebugEditor(UMotionDataAsset* MotionData, UDebugSkelMeshComponent* DebugSkeletalMesh,
		const int32 PreviewIndex, const int32 FeatureOffset, const UWorld* World, FPrimitiveDrawInterface* DrawInterface) override;
	virtual void DrawDebugCurrentRuntime(FAnimInstanceProxy* AnimInstanceProxy, UMotionDataAsset* MotionData,
		TArray<float>& CurrentPoseArray, const int32 FeatureOffset) override; 
};