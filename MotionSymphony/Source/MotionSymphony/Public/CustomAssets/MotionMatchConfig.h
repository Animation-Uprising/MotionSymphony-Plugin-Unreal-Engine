// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BoneContainer.h"
#include "DataOriented/MatchFeatureBase.h"
#include "Enumerations/EMMPreProcessEnums.h"
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

	//Todo: Make the axis' editor only data which gets removed for a full build
	/** The up axis of the model*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	EAllAxis UpAxis;

	/** The forward axis of the model. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	EAllAxis ForwardAxis;

	/** A list of features that the end user would like to match */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Features")
	TArray<TObjectPtr<UMatchFeatureBase>> Features;

	/** Atom offsets for for all features */
	UPROPERTY()
	TArray<int32> Offsets;

	/** Number of dimensions in the response portion of features*/ 
	UPROPERTY()
	int32 ResponseDimensionCount = 0;

	/** Number of dimensions in the quality portion of features*/
	UPROPERTY()
	int32 QualityDimensionCount = 0;
	
	
	/** Total number of dimensions (Atoms) for all features.*/
	UPROPERTY()
	int32 TotalDimensionCount = 0;

public:
	void Initialize();
	void ComputeOffsets();

	virtual USkeleton* GetSkeleton(bool& bInvalidSkeletonIsError) override;
	USkeleton* GetSkeleton() const;
	void SetSourceSkeleton(USkeleton* Skeleton);
	
	bool IsSetupValid();
	
	int32 ComputeResponseArraySize();
	int32 ComputeQualityArraySize();
};
