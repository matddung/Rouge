// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NormalEnemyAIController.generated.h"

/**
 * 
 */
UCLASS()
class ROGUE_API ANormalEnemyAIController : public AAIController
{
	GENERATED_BODY()
	
public:
    ANormalEnemyAIController();
	virtual void OnPossess(APawn* InPawn) override;

	static const FName HomePosKey;
	static const FName PatrolPosKey;
	static const FName TargetKey;

private:
	UPROPERTY()
	class UBehaviorTree* BTAsset;

	UPROPERTY()
	class UBlackboardData* BBAsset;
};
