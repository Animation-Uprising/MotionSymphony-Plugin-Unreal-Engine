// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_AssetPlayerBase.h"
#include "AnimNode_MotionMatching.h"
#include "AnimGraphNode_MotionMatching.generated.h"


UCLASS(BlueprintType, Blueprintable)
class MOTIONSYMPHONYEDITOR_API UAnimGraphNode_MotionMatching : public UAnimGraphNode_AssetPlayerBase
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Settings")
	FAnimNode_MotionMatching Node;

	//~ Begin UEdGraphNode Interface.
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//virtual void CreateOutputPins() override;
	//~ End UEdGraphNode Interface.

	//~ Begin UAnimGraphNode_Base Interface
	virtual FString GetNodeCategory() const override;
	//~ End UAnimGraphNode_Base Interface

	// UK2Node interface
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	// End of UK2Node interface

protected:
	virtual FString GetControllerDescription() const;
};
