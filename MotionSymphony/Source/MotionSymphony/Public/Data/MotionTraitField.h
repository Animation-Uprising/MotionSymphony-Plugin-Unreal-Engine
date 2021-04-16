// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MotionTraitField.generated.h"

USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FMotionTraitField
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	int32 A;

	UPROPERTY()
	int32 B;

public:
	FMotionTraitField();
	FMotionTraitField(const int32 TraitIndex);
	FMotionTraitField(const int32 InA, const int32 InB);

	void Clear();

	void SetTraitPosition(const int32 TraitPosition);
	void UnSetTraitPosition(const int32 TraitPosition);
	void SetTraits(const FMotionTraitField Traits);
	void UnSetTraits(const FMotionTraitField Traits);

	bool HasTrait(const int32 TraitPosition) const;
	bool HasTraits(const FMotionTraitField Traits) const;

	bool operator != (const FMotionTraitField& rhs) const;
	bool operator == (const FMotionTraitField& rhs) const;
	FMotionTraitField operator | (const FMotionTraitField& rhs) const;
	FMotionTraitField operator & (const FMotionTraitField& rhs) const;
	void operator |= (const FMotionTraitField& rhs);
	void operator &= (const FMotionTraitField& rhs);
};

inline uint64 GetTypeHash(const FMotionTraitField A)
{
	return ((uint64)A.A << 32) | (uint32)A.B;
}
