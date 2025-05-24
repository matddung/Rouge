// Fill out your copyright notice in the Description page of Project Settings.


#include "BTDecorator_IsInAttackRange.h"
#include "NormalEnemyAIController.h"
#include "RogueCharacter.h"
#include "EnemyBase.h"

#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_IsInAttackRange::UBTDecorator_IsInAttackRange()
{
	NodeName = TEXT("CanAttack");
}

bool UBTDecorator_IsInAttackRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	auto ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (ControllingPawn == nullptr) return false;

	auto Target = Cast<ARogueCharacter>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(ANormalEnemyAIController::TargetKey));
	if (Target == nullptr) return false;

	auto Enemy = Cast<AEnemyBase>(ControllingPawn);
	if (Enemy == nullptr) return false;

	bool bResult = (Target->GetDistanceTo(ControllingPawn) <= Enemy->EnemyStats.AttackRange);
	return bResult;
}