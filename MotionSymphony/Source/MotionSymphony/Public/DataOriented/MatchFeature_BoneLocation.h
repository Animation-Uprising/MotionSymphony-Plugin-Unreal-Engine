#pragma once

#include "CoreMinimal.h"
#include "MatchFeatureBase.h"
#include "MatchFeature_BoneLocation.generated.h"


struct FPoseMatrix;

/** Motion matching feature which stores and matches bone location relative to the character root */
UCLASS(BlueprintType)
class MOTIONSYMPHONY_API UMatchFeature_BoneLocation : public UMatchFeatureBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "Match Feature|Bone Location")
	FBoneReference BoneReference;

public:
	virtual int32 Size() const override;
	virtual void EvaluatePreProcess(float* ResultLocation, FMotionAnimSequence& InSequence,
	                                const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile) override;
	virtual void EvaluatePreProcess(float* ResultLocation, FMotionComposite& InComposite,
	                                const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile) override;
	virtual void EvaluatePreProcess(float* ResultLocation, FMotionBlendSpace& InBlendSpace,
	                                const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile, const FVector2D BlendSpacePosition) override;
	//virtual void EvaluateRuntime(float* RestulLocation) override;

	virtual void DrawPoseDebugEditor(UMotionDataAsset* MotionData, UDebugSkelMeshComponent* DebugSkeletalMesh,
		const int32 PreviewIndex, const int32 FeatureOffset, const UWorld* World, FPrimitiveDrawInterface* DrawInterface) override;
	virtual void DrawDebugDesiredRuntime(FAnimInstanceProxy* AnimInstanceProxy, FMotionMatchingInputData& InputData,
		const int32 FeatureOffset, UMotionMatchConfig* MMConfig) override;
	virtual void DrawDebugCurrentRuntime(FAnimInstanceProxy* AnimInstanceProxy, UMotionDataAsset* MotionData,
		TArray<float>& CurrentPoseArray, const int32 FeatureOffset) override; 
};
