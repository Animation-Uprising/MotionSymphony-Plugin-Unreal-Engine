// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once
#include "SlateBasics.h"

class FMotionSymphonyStyle
{
public:
	static void Initialize();
	static void Shutdown();
	static FName GetStyleSetName();
	static TSharedPtr<class ISlateStyle> Get();

private:
	static TSharedRef<class FSlateStyleSet> Create();

private:
	static TSharedPtr<class FSlateStyleSet> StyleInstance;

};