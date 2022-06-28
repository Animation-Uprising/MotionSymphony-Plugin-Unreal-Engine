// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PoseMatrix.generated.h"

class UMotionDataAsset;
/**
 * 
 */
USTRUCT()
struct MOTIONSYMPHONY_API FPoseMatrix
{
	GENERATED_BODY()
	
public:
	/** The number of poses*/
	UPROPERTY()
	int32 PoseCount;

	/** The number of atoms per pose*/
	UPROPERTY()
	int32 AtomCount;

	/** The animation matrix */
	UPROPERTY()
	TArray<float> PoseArray;
	
	FPoseMatrix(int32 InPoseCount, int32 InAtomCont);
	FPoseMatrix();

	float& GetAtom(int32 PoseId, int32 AtomId);
	const float& GetAtom(int32 PoseId, int32 AtomId) const;
};