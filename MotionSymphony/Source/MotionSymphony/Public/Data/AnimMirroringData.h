// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CustomAssets/MirroringProfile.h"
#include "AnimMirroringData.generated.h"

struct FBoneContainer;

/** Defines a pair of mirror bones in a skeleton */
USTRUCT()
struct FIndexedMirrorPair
{
	GENERATED_BODY();

public:
	/** Index of the bone */
	UPROPERTY()
	int32 BoneIndex;

	/** Index of the mirror bone */
	UPROPERTY()
	int32 MirrorBoneIndex;

public:
	FIndexedMirrorPair();
	FIndexedMirrorPair(int32 InBoneIndex, int32 InMirrorBoneIndex);
};


/** This struct holds all runtime mirroring data specific for the skeleton being used in 
the animation graph. It holds a list of mirroring paris and rules for mirroring them. This
is generated at runtime within a node that performs animation mirrroring and an associated
MirroringProfile asset. */
USTRUCT()
struct FAnimMirroringData
{
	GENERATED_BODY()

public:
	/** A list of bone mirror pairs for mirroring at runtime */
	UPROPERTY()
	TArray<FIndexedMirrorPair> IndexedMirrorPairs;

	FIndexedMirrorPair NullMirrorPair;
	
public:
	void Initialize(const UMirroringProfile* MirroringProfile, const USkeletalMeshComponent* SkelMesh);
	void Initialize(const TArray<FBoneMirrorPair>& OverrideMirrorPairs, const UMirroringProfile* MirroringProfile, const USkeletalMeshComponent* SkelMesh);
	void AddPair(const FBoneMirrorPair& MirrorPair, const USkeletalMeshComponent* SkelMesh);

	const FIndexedMirrorPair& FindPair(const int32 PairIndex) const;
	int32 FindMirrorBone(const int32 BoneIndex) const;

	//static void MirrorTransform(FTransform& Transform, const EAxis::Type MirrorAxis);

	private:

	bool IsMatchBoneName(const FString& BoneName, const FString MatchStr, EMirrorMatchingRule MatchRule);
	FString GetPairBoneName(const FString& BoneName, const FString& SourceMatchStr, const FString& TargetMatchStr, EMirrorMatchingRule MatchRule);
	

};