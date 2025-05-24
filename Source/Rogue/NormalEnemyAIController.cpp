// Fill out your copyright notice in the Description page of Project Settings.


#include "NormalEnemyAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"

const FName ANormalEnemyAIController::HomePosKey(TEXT("HomePos"));
const FName ANormalEnemyAIController::PatrolPosKey(TEXT("PatrolPos"));
const FName ANormalEnemyAIController::TargetKey(TEXT("Target"));

ANormalEnemyAIController::ANormalEnemyAIController()
{
	static ConstructorHelpers::FObjectFinder<UBlackboardData> BBObject(TEXT("/Game/AI/BB_NormalEnemy.BB_NormalEnemy"));

	if (BBObject.Succeeded())
	{
		BBAsset = BBObject.Object;
	}

	static ConstructorHelpers::FObjectFinder<UBehaviorTree> BTObject(TEXT("/Game/AI/BT_NormalEnemy.BT_NormalEnemy"));

	if (BTObject.Succeeded())
	{
		BTAsset = BTObject.Object;
	}
}

void ANormalEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (UseBlackboard(BBAsset, Blackboard))
	{
		Blackboard->SetValueAsVector(HomePosKey, InPawn->GetActorLocation());
		Blackboard->SetValueAsBool(TEXT("IsDead"), false);
		if (!RunBehaviorTree(BTAsset))
		{
			UE_LOG(LogTemp, Warning, TEXT("NormalEnemyAIController couldn't run behavior tree"));
		}
	}
}