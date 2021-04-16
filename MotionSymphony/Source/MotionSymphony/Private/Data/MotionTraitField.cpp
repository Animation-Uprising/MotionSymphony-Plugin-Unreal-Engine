// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "Data/MotionTraitField.h"

FMotionTraitField::FMotionTraitField()
	: A(0),
	B(0)
{ }

FMotionTraitField::FMotionTraitField(const int32 TraitPosition)
	: A(0),
	B(0)
{
	if (TraitPosition < 32)
	{
		A = (1 << TraitPosition);
	}
	else
	{
		B = (1 << (TraitPosition - 32));
	}
}

FMotionTraitField::FMotionTraitField(const int32 InA, const int32 InB)
	: A(InA),
	B(InB)
{ }



void FMotionTraitField::Clear()
{
	B = A = 0;
}

bool FMotionTraitField::HasTrait(const int32 TraitPosition) const
{
	if (TraitPosition < 32)
	{
		int32 Trait = (1 << TraitPosition);

		return (A & Trait) == Trait;
	}
	else
	{
		int32 Trait = (1 << (TraitPosition - 32));

		return (B & Trait) == Trait;
	}

	return false;
}

bool FMotionTraitField::HasTraits(const FMotionTraitField Traits) const
{
	return ((A & Traits.A) == Traits.A) && ((B & Traits.B) == Traits.B);
}

void FMotionTraitField::SetTraitPosition(const int32 TraitPosition)
{
	if (TraitPosition < 32)
	{
		A |= (1 << TraitPosition);
	}
	else
	{
		B |= (1 << (TraitPosition - 32));
	}
}

void FMotionTraitField::UnSetTraitPosition(const int32 TraitPosition)
{
	if (TraitPosition < 32)
	{
		A &= ~(1 << TraitPosition);
	}
	else
	{
		B &= ~(1 << (TraitPosition - 32));
	}
}

void FMotionTraitField::SetTraits(const FMotionTraitField Traits)
{
	A |= Traits.A;
	B |= Traits.B;
}

void FMotionTraitField::UnSetTraits(const FMotionTraitField Traits)
{
	A &= ~Traits.A;
	B &= ~Traits.B;
}

bool FMotionTraitField::operator==(const FMotionTraitField& rhs) const
{
	return (A == rhs.A && B == rhs.B);
}

bool FMotionTraitField::operator!=(const FMotionTraitField& rhs) const
{
	return (A != rhs.A || B != rhs.B);
}

FMotionTraitField FMotionTraitField::operator&(const FMotionTraitField& rhs) const
{
	return FMotionTraitField(A & rhs.A, B | rhs.B);
}

FMotionTraitField FMotionTraitField::operator|(const FMotionTraitField& rhs) const
{
	return FMotionTraitField(A | rhs.A, B | rhs.B);
}

void FMotionTraitField::operator&=(const FMotionTraitField& rhs)
{
	A &= rhs.A;
	B &= rhs.B;
}

void FMotionTraitField::operator|=(const FMotionTraitField& rhs)
{
	A |= rhs.A;
	B |= rhs.B;
}
