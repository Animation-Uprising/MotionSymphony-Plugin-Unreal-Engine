// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MMOptimisationModule.h"
#include "MotionMatchingUtil/KMeansClustering.h"
#include "MotionMatchingUtil/PoseLookupTable.h"
#include "MMOptimisation_MultiClustering.generated.h"


UCLASS()
class MOTIONSYMPHONY_API UMMOptimisation_MultiClustering : public UMMOptimisationModule
{
	GENERATED_BODY()

public:
	/** The number of clusters to create in the first step of trajectory clustering during pre-process*/
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (ClampMin = 1))
	int32 KMeansClusterCount;

	/** The maximum number of K-means clustering iterations for each attempt.*/
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (ClampMin = 1))
	int32 KMeansMaxIterations;

	/** The desired size of the lookup table (i.e. number of columns? */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (ClampMin = 1))
	int32 DesiredLookupTableSize;

	/** The desired maximum poses in each lookup column (note the value is not exact, some columns
	may have more than the max but no more than double. */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (ClampMin = 1))
	int32 MaxLookupColumnSize;

	/** A lookup table for pose searches. Each pose in the data set points to a single column
	of this table. At any pose search, only one of these columns will ever be searched. Each
	column holds an Id of a potential successor pose. */
	UPROPERTY()
	TMap<FMotionTraitField, FPoseLookupTable> PoseLookupSets;

public:
	UMMOptimisation_MultiClustering(const FObjectInitializer& ObjectInitializer);

	virtual void BuildOptimisationStructures(UMotionDataAsset* InMotionDataAsset) override;
	virtual TArray<FPoseMotionData>* GetFilteredPoseList(const FPoseMotionData& CurrentPose, 
		const FMotionTraitField RequiredTraits, const FCalibrationData& FinalCalibration);

	virtual void InitializeRuntime() override;
};