// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "MotionDataAsset.h"
#include "Data/Trajectory.h"
#include "TrajectoryGenerator_Base.generated.h"

class UCameraComponent;

UCLASS(BlueprintType, Category = "Motion Matching", meta = (BlueprintSpawnableComponent))
class MOTIONSYMPHONY_API UTrajectoryGenerator_Base : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMotionDataAsset* MotionData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float RecordingFrequency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 1.0f))
	float SampleRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bFlattenTrajectory;

	//Trajectory
	UPROPERTY()
	FTrajectory Trajectory;

	UPROPERTY()
	FVector2D InputVector;

protected:
	//Past Trajectory
	float MaxRecordTime;
	float TimeSinceLastRecord;
	TArray<FVector> RecordedPastPositions; //Todo: Make these circular buffers
	TArray<float> RecordedPastRotations;
	TArray<float> RecordedPastTimes;
	float CumActiveTime;

	//Tracking
	float TimeHorizon;
	float TimeStep;
	int TrajectoryIterations;
	float CurFacingAngle;
	
	TArray<FVector> TrajPositions;
	TArray<float> TrajRotations;
	TArray<float> TrajTimes;

	//Character
	AActor* OwningActor;

private:
	bool bExtractedThisFrame;

	FTransform CacheActorTransform;

public:	
	// Sets default values for this component's properties
	UTrajectoryGenerator_Base();

public:
	UFUNCTION(BluePrintCallable, Category = "MotionMatching")
	void SetTrajectoryInputX(float XAxisValue);

	UFUNCTION(BluePrintCallable, Category = "MotionMatching")
	void SetTrajectoryInputY(float YAxisValue);

	UFUNCTION(BluePrintCallable, Category = "MotionMatching")
	void SetTrajectoryInput(float XAxisValue, float YAxisValue);

	UFUNCTION(BlueprintCallable, Category = "MotionMatching")
	FTrajectory& GetCurrentTrajectory();

	UFUNCTION(BlueprintCallable, Category = "MotionMatching/Debug")
	void DrawTrajectoryDebug(FVector DrawOffset);

	UFUNCTION(BlueprintCallable, Category = "MotionMatching")
	bool IsIdle();

	UFUNCTION(BlueprintCallable, Category = "MotionMatching")
	bool HasMoveInput();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction) override;

	virtual void BeginPlay() override;

protected:
	
	void RecordPastTrajectory(float DeltaTime);
	virtual void UpdatePrediction(float DeltaTime);

private:
	virtual void Setup(TArray<float>& InTrajTimes);
	void ExtractTrajectory();
	inline void ClampInputVector();
};
