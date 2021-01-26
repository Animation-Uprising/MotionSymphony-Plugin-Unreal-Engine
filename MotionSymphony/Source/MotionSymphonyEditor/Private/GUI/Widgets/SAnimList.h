// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "ContentBrowserDelegates.h"

class SBorder;
class SScrollBox;
class SBox;
class SButton;
class STextBlock;
class FMotionPreProcessToolkit;

class SAnimWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimWidget){}
	SLATE_END_ARGS()

protected:
	int32 AnimIndex;

private:
	TWeakPtr<FMotionPreProcessToolkit> MotionPreProcessToolkitPtr;

public:
	void Construct(const FArguments& InArgs, int32 InFrameIndex, TWeakPtr<FMotionPreProcessToolkit> InMotionPreProcessToolkit);

	FReply OnAnimClicked();
	void OnRemoveAnim();

protected:
	FText GetAnimAssetName() const;
};

class SBlendSpaceWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBlendSpaceWidget) {}
	SLATE_END_ARGS()

protected:
	int32 BlendSpaceIndex;

private:
	TWeakPtr<FMotionPreProcessToolkit> MotionPreProcessToolkitPtr;

public:
	void Construct(const FArguments& InArgs, int32 InFrameIndex, TWeakPtr<FMotionPreProcessToolkit> InMotionPreProcessToolkit);

	FReply OnBlendSpaceClicked();
	void OnRemoveBlendSpace();

protected:
	FText GetBlendSpaceAssetName() const;
};

class SAnimList : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimList) {}
	SLATE_END_ARGS()

public:
	TWeakPtr<class FMotionPreProcessToolkit> MotionPreProcessToolkitPtr;

protected:
	TSharedPtr<SVerticalBox> AnimListBox;
	TSharedPtr<SVerticalBox> BlendSpaceListBox;

	TSet<FName> AssetRegistryTagsToIgnore;

	FSyncToAssetsDelegate SyncToAssetsDelegate;

	TArray<TSharedPtr<SAnimWidget>> AnimWidgets;
	TArray<TSharedPtr<SAnimWidget>> BlendSpaceWidget;

public:
	void Construct(const FArguments& InArgs, TWeakPtr<class FMotionPreProcessToolkit> InMotionFieldEditor);

	virtual void Tick(const FGeometry& AllottedGeometry, 
		const double InCurrentTime, const float InDeltaTime) override;

	FReply OnAddNewAnim();
	FReply OnClearAnims();

	void Rebuild();
};