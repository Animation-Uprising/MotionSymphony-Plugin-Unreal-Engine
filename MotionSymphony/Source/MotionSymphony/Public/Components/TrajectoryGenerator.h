// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrajectoryGenerator_Base.h"
#include "TrajectoryGenerator.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MOTIONSYMPHONY_API UTrajectoryGenerator : public UTrajectoryGenerator_Base
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotonSettings")
	float MaxSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotonSettings")
	float MoveResponse;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotonSettings")
	float TurnResponse;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behaviour")
	bool bResetDirectionOnIdle;
	
private:
	TArray<FVector> NewTrajPosition;
	float LastDesiredOrientation;

public:
	UTrajectoryGenerator();

protected:
	virtual void UpdatePrediction(float DeltaTime) override;
	virtual void Setup(TArray<float>& TrajTimes);

private:
	void CalculateDesiredLinearVelocity(FVector& OutVelocity);
	
};