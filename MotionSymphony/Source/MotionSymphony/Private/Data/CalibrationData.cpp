// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "Data/CalibrationData.h"
#include "Data/MotionTraitField.h"
#include "Objects/Assets/MotionDataAsset.h"
#include "Objects/Assets/MotionMatchConfig.h"

FCalibrationData::FCalibrationData()
{
	Weights.Empty(30);
}

FCalibrationData::FCalibrationData(UMotionDataAsset* SourceMotionData)
{
	if(!SourceMotionData)
	{
		Weights.Empty(30);
		return;
	}

	const UMotionMatchConfig* MMConfig = SourceMotionData->MotionMatchConfig;

	if(!MMConfig)
	{
		return;
	}
	
	Weights.SetNum(MMConfig->TotalDimensionCount);
}

FCalibrationData::FCalibrationData(UMotionMatchConfig* SourceConfig)
{
	Initialize(SourceConfig);
}

FCalibrationData::FCalibrationData(int32 AtomCount)
{
	Weights.SetNum(FMath::Max(0, AtomCount));
}

void FCalibrationData::Initialize(UMotionMatchConfig* SourceConfig)
{
	if (SourceConfig == nullptr)
	{
		Weights.Empty(30);
		return;
	}

	Weights.SetNum(SourceConfig->TotalDimensionCount);
}

bool FCalibrationData::IsValidWithConfig(const UMotionMatchConfig* MotionConfig)
{
	if (!MotionConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("FCalibrationData: Trying to call IsValidWithConfig(UMotionMatchConfig*) but the motion config passed is null"));
		return false;
	}
	
	if(Weights.Num() != MotionConfig->TotalDimensionCount)
	{
		UE_LOG(LogTemp, Error, TEXT("FCalibrationData: Calibration Data is invalid as the weight array does not match the motion config pose array"));
		return false;
	}

	return true;
}

void FCalibrationData::GenerateStandardDeviationWeights(const UMotionDataAsset* SourceMotionData, const FMotionTraitField& MotionTrait)
{
	if (!SourceMotionData || !SourceMotionData->MotionMatchConfig)
	{
		return;
	}

	UMotionMatchConfig* MMConfig = SourceMotionData->MotionMatchConfig;
	Initialize(MMConfig);
	
	int32 SdPoseCount = 0;		//Sd means 'standard deviation'
	int32 LookupMatrixPoseId = 0;

	//Determine the total for each atom
	const int32 AtomCount = SourceMotionData->LookupPoseMatrix.AtomCount; //Actual number of matching atoms
	const int32 MeasuredAtomCount = AtomCount - 1; //Atom count excluding cost multiplier
	TArray<float> TotalsArray;
	TotalsArray.SetNumZeroed(MeasuredAtomCount);
	const TArray<float>& PoseArray = SourceMotionData->LookupPoseMatrix.PoseArray;
	for(const FPoseMotionData& Pose : SourceMotionData->Poses)
	{
		++LookupMatrixPoseId;
		
		if(Pose.SearchFlag == EPoseSearchFlag::Searchable
			|| Pose.Traits != MotionTrait)
		{
			continue;
		}

		++SdPoseCount;
		
		const int32 PoseStartIndex = (LookupMatrixPoseId - 1) * AtomCount + 1; //+1 to make room for Pose Cost Multiplier
		for(int32 i = 0; i < MeasuredAtomCount; ++i)
		{
			TotalsArray[i] += PoseArray[PoseStartIndex + i];
		}
	}
	
	SdPoseCount = FMath::Max(SdPoseCount, 1);

	//Determine the Mean for each atom
	TArray<float> MeanArray;
	MeanArray.SetNumZeroed(MeasuredAtomCount);
	for(int32 i = 0; i < MeasuredAtomCount; ++i)
	{
		MeanArray[i] = TotalsArray[i] / SdPoseCount;
	}

	LookupMatrixPoseId = 0;

	//Determine the distance to the mean squared for each atom
	TArray<float> TotalDistanceSqrArray;
	TotalDistanceSqrArray.SetNumZeroed(MeasuredAtomCount);
	for (const FPoseMotionData& Pose : SourceMotionData->Poses)
	{
		++LookupMatrixPoseId;
		
		if(Pose.SearchFlag == EPoseSearchFlag::DoNotUse
			|| Pose.Traits != MotionTrait)
		{
			continue;
		}
		
		const int32 PoseStartIndex = (LookupMatrixPoseId - 1) * AtomCount + 1; //+1 to make room for Pose Cost Multiplier
		int32 FeatureOffset = 0; //Since the standard deviation array does not include a pose cost multiplier the feature offset does not have a +1
		for(const TObjectPtr<UMatchFeatureBase> FeaturePtr : MMConfig->Features)
		{
			const int32 FeatureSize = FeaturePtr->Size();
			
			FeaturePtr->CalculateDistanceSqrToMeanArrayForStandardDeviations(TotalDistanceSqrArray,
				MeanArray, PoseArray, FeatureOffset, PoseStartIndex);
		
			FeatureOffset += FeatureSize;
		}
		
		// for(int32 i = 0; i < MeasuredAtomCount; ++i)
		// {
		// 	const float DistanceToMean = PoseArray[PoseStartIndex + i] - MeanArray[i];
		// 	TotalDistanceSqrArray[i] += DistanceToMean * DistanceToMean;
		// }
	}

	//Calculate the standard deviation and final standard deviation weight
	for(int32 i = 0; i < MeasuredAtomCount; ++i)
	{
		const float StandardDeviation = TotalDistanceSqrArray[i] / SdPoseCount;
		Weights[i] = FMath::IsNearlyZero(StandardDeviation)? 0.0f : 1.0f / StandardDeviation;
	}
}

void FCalibrationData::GenerateFinalWeights(const UMotionCalibration* SourceCalibration, const FCalibrationData& StdDeviationNormalizers)
{
	if (!SourceCalibration || !SourceCalibration->MotionMatchConfig)
	{
		return;
	}

	Initialize(SourceCalibration->MotionMatchConfig);
	
	const float ResponseMultiplier = SourceCalibration->QualityVsResponsivenessRatio * 2.0f;
	const float QualityMultiplier = (1.0f - SourceCalibration->QualityVsResponsivenessRatio) * 2.0f;

	int32 AtomIndex = 0;
	TArray<TObjectPtr<UMatchFeatureBase>>& Features = SourceCalibration->MotionMatchConfig->Features;
	for(TObjectPtr<UMatchFeatureBase> FeaturePtr : Features)
	{
		const UMatchFeatureBase* Feature = FeaturePtr.Get();

		if(!Feature)
		{
			continue;
		}

		const int32 FeatureSize = Feature->Size();
		if(Feature->PoseCategory == EPoseCategory::Quality)
		{
			for(int32 i = 0; i < FeatureSize; ++i)
			{
				Weights[AtomIndex] = SourceCalibration->CalibrationArray[AtomIndex] * QualityMultiplier * StdDeviationNormalizers.Weights[AtomIndex];
				++AtomIndex;
			}
		}
		else
		{
			for(int32 i = 0; i < FeatureSize; ++i)
			{
				Weights[AtomIndex] = SourceCalibration->CalibrationArray[AtomIndex] * ResponseMultiplier * StdDeviationNormalizers.Weights[AtomIndex];
				++AtomIndex;
			}
		}
	}
}