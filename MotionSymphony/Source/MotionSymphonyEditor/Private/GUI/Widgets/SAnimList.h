// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Enumerations/EMotionMatchingEnums.h"
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

	void SelectWidget();
	void DeselectWidget();

protected:
	FText GetAnimAssetName() const;

	TSharedPtr<SButton> ButtonWidget;
};

class SCompositeWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCompositeWidget) {}
	SLATE_END_ARGS()

protected:
	int32 CompositeIndex;

private:
	TWeakPtr<FMotionPreProcessToolkit> MotionPreProcessToolkitPtr;

public:
	void Construct(const FArguments& InArgs, int32 InFrameIndex, TWeakPtr<FMotionPreProcessToolkit> InMotionPreProcessToolkit);

	FReply OnCompositeClicked();
	void OnRemoveComposite();

	void SelectWidget();
	void DeselectWidget();

protected:
	FText GetCompositeAssetName() const; 

	TSharedPtr<SButton> ButtonWidget;
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

	void SelectWidget();
	void DeselectWidget();

protected:
	FText GetBlendSpaceAssetName() const;

	TSharedPtr<SButton> ButtonWidget;
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
	TSharedPtr<SVerticalBox> CompositeListBox;
	TSharedPtr<SVerticalBox> BlendSpaceListBox;
	
	TSet<FName> AssetRegistryTagsToIgnore;

	FSyncToAssetsDelegate SyncToAssetsDelegate;

	TArray<TSharedPtr<SAnimWidget>> AnimWidgets;
	TArray<TSharedPtr<SCompositeWidget>> CompositeWidgets;
	TArray<TSharedPtr<SBlendSpaceWidget>> BlendSpaceWidgets;

public:
	void Construct(const FArguments& InArgs, TWeakPtr<class FMotionPreProcessToolkit> InMotionFieldEditor);

	virtual void Tick(const FGeometry& AllottedGeometry, 
		const double InCurrentTime, const float InDeltaTime) override;

	FReply OnAddNewAnim();
	FReply OnClearAnims();

	void SelectAnim(EMotionAnimAssetType AnimType, int32 AnimIndex);
	void DeselectAnim(EMotionAnimAssetType AnimType, int32 AnimIndex);

	void Rebuild();
};