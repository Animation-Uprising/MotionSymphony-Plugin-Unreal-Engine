// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "MotionPreProcessToolkit.h"
#include "ContentBrowserDelegates.h"

class SBorder;
class SScrollBox;
class SBox;
class SButton;
class STextBlock;

class SAnimWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimWidget){}
	SLATE_END_ARGS()

protected:
	int32 AnimIndex;

private:
	TWeakPtr<class FMotionPreProcessToolkit> MotionPreProcessToolkitPtr;

public:
	void Construct(const FArguments& InArgs, int32 InFrameIndex, TWeakPtr<class FMotionPreProcessToolkit> InMotionPreProcessToolkit);

	FReply OnAnimClicked();
	void OnRemoveAnim();

protected:
	FText GetAnimAssetName() const;
};

class SAnimList : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimList) {}
	SLATE_END_ARGS()

public:
	TWeakPtr<class FMotionPreProcessToolkit> MotionPreProcessToolkitPtr;

protected:
	TSharedPtr<SVerticalBox> MainBox;

	TSet<FName> AssetRegistryTagsToIgnore;

	FSyncToAssetsDelegate SyncToAssetsDelegate;

	TArray<TSharedPtr<SAnimWidget>> AnimWidgets;

public:
	void Construct(const FArguments& InArgs, TWeakPtr<class FMotionPreProcessToolkit> InMotionFieldEditor);

	virtual void Tick(const FGeometry& AllottedGeometry, 
		const double InCurrentTime, const float InDeltaTime) override;

	FReply OnAddNewAnim();
	FReply OnClearAnims();

	void Rebuild();
};