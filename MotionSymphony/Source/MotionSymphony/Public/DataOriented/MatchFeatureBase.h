#pragma once

#include "CoreMinimal.h"
#include "Interfaces/Interface_BoneReferenceSkeletonProvider.h"
#include "MatchFeatureBase.generated.h"

class UMotionDataAsset;
class UMotionMatchConfig;
struct FMotionMatchingInputData;
class UMirroringProfile;
struct FMotionAnimSequence;
struct FMotionComposite;
struct FMotionBlendSpace;
struct FPoseMotionData;

UENUM()
enum class EPoseCategory : uint8
{
	Quality,
	Responsiveness
};

struct FPoseMatrix;

/** Base class for all motion matching features */
UCLASS(abstract, EditInlineNew, config=Game)
class MOTIONSYMPHONY_API UMatchFeatureBase : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Match Feature");
	float DefaultWeight;

	UPROPERTY(EditAnywhere, Category = "Match Feature");
	EPoseCategory PoseCategory;
	

public:
	virtual void Initialize();
	virtual bool IsSetupValid() const;
	virtual bool IsMotionSnapshotCompatible() const;
	
	virtual int32 Size() const;
	virtual void EvaluatePreProcess(float* ResultLocation, FMotionAnimSequence& InSequence,
	                                const float Time, const float PoseInterval, const bool bMirror,
	                                UMirroringProfile* MirrorProfile);
	virtual void EvaluatePreProcess(float* ResultLocation, FMotionComposite& InComposite,
	                                const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile);
	virtual void EvaluatePreProcess(float* ResultLocation, FMotionBlendSpace& InBlendSpace,
	                                const float Time, const float PoseInterval, const bool bMirror,
	                                UMirroringProfile* MirrorProfile, const FVector2D BlendSpacePosition);

	virtual void CacheMotionBones(FAnimInstanceProxy* InAnimInstanceProxy);
	virtual void ExtractRuntime(FCSPose<FCompactPose>& CSPose, float* ResultLocation, float* FeatureCacheLocation, FAnimInstanceProxy*
	                            AnimInstanceProxy, float DeltaTime);

	//This function is only to be used for responsiveness type features. Usually used to apply trajectory blending
	virtual void ApplyInputBlending(TArray<float>& DesiredInputArray, const TArray<float>& CurrentPoseArray, const int32 FeatureOffset, const float Weight);

	//This function is only to be used for responsiveness type features. Usually used to check if the next pose trajectory is close enough to the current trajectory.
	virtual bool NextPoseToleranceTest(const TArray<float>& DesiredInputArray, const TArray<float>& PoseMatrix,
	                                   const int32 MatrixStartIndex, const int32 FeatureOffset, const float PositionTolerance, const float RotationTolerance);

	virtual float GetDefaultWeight(int32 AtomId) const;

	virtual void CalculateDistanceSqrToMeanArrayForStandardDeviations(TArray<float>& OutDistToMeanSqrArray,
	                                                                  const TArray<float>& InMeanArray, const TArray<float>& InPoseArray, const int32 FeatureOffset, const int32 PoseStartIndex) const;
	
#if WITH_EDITOR
	//Draws debugging for this feature in the MoSymph editor for the current pose . (E.g. draw baked trajectory data)
	virtual void DrawPoseDebugEditor(UMotionDataAsset* MotionData, UDebugSkelMeshComponent* DebugSkeletalMesh,
		const int32 PreviewIndex, const int32 FeatureOffset, const UWorld* World, FPrimitiveDrawInterface* DrawInterface); 

	//Draws debugging for this feature in the MoSymph editor for the entire animation . (E.g. draw entire trajectory for whole animation)
	virtual void DrawAnimDebugEditor(); 

	//Draws debugging for this feature at runtime for desired input array (used mostly for responsive category. (E.g.Draw desired trajectory at runtime)
	virtual void DrawDebugDesiredRuntime(FAnimInstanceProxy* AnimInstanceProxy, FMotionMatchingInputData& InputData,
		const int32 FeatureOffset, UMotionMatchConfig* MMConfig); 

	//Draws debugging for this feature at runtime for the chosen pose. (E.g. Draw chosen trajectory at runtime)
	virtual void DrawDebugCurrentRuntime(FAnimInstanceProxy* AnimInstanceProxy, UMotionDataAsset* MotionData,
		TArray<float>& CurrentPoseArray, const int32 FeatureOffset);
#endif
	
	//virtual void EvaluateRuntime(float* ResultLocation);
};