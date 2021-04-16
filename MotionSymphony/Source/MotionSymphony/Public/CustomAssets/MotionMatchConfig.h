// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BoneContainer.h"
//#include "Interface_BoneReferenceSkeletonProvider.h"
#include "Interfaces/Interface_BoneReferenceSkeletonProvider.h"
#include "MotionMatchConfig.generated.h"

class USkeleton;

UCLASS(BlueprintType)
class MOTIONSYMPHONY_API UMotionMatchConfig : public UObject, public IBoneReferenceSkeletonProvider
{
	GENERATED_BODY()

public:
	UMotionMatchConfig(const FObjectInitializer& ObjectInitializer);
	
public:
	/** The source skeleton that this motion match configuration is based on and compatible with */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	USkeleton* SourceSkeleton;

	/** The trajectory point timings for past and furture trajectory prediction. Should be listed in
	ascending order with past values being negative values. Measured in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trajectory Config")
	TArray<float> TrajectoryTimes;

	/** Bone references for all character bones to use for pose matching */
	UPROPERTY(EditAnywhere, Category = "Pose Config")
	TArray<FBoneReference> PoseBones;


public:
	void Initialize();

	virtual USkeleton* GetSkeleton(bool& bInvalidSkeletonIsError) override;
	USkeleton* GetSkeleton();
	void SetSourceSkeleton(USkeleton* kkeleton);

	bool IsSetupValid();
};
