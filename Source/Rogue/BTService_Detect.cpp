// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_Detect.h"
#include "NormalEnemyAIController.h"
#include "RogueCharacter.h"

#include "DrawDebugHelpers.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTService_Detect::UBTService_Detect()
{
	NodeName = TEXT("Detect");
	Interval = 1;
}

void UBTService_Detect::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (ControllingPawn == nullptr) return;

	UWorld* World = ControllingPawn->GetWorld();
	FVector Center = ControllingPawn->GetActorLocation();
	float DetectRadius = 600;

	if (World == nullptr) return;

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams CollisionQueryParam(NAME_None, false, ControllingPawn);
	bool bResult = World->OverlapMultiByChannel(OverlapResults, Center, FQuat::Identity, ECollisionChannel::ECC_GameTraceChannel2, FCollisionShape::MakeSphere(DetectRadius), CollisionQueryParam);

	if (bResult)
	{
		for (auto const& OverlapResult : OverlapResults)
		{
			ARogueCharacter* RogueCharacter = Cast<ARogueCharacter>(OverlapResult.GetActor());
			if (RogueCharacter && RogueCharacter->GetController()->IsPlayerController())
			{
				OwnerComp.GetBlackboardComponent()->SetValueAsObject(ANormalEnemyAIController::TargetKey, RogueCharacter);
				DrawDebugSphere(World, Center, DetectRadius, 16, FColor::Green, false, 0.2);
				DrawDebugPoint(World, RogueCharacter->GetActorLocation(), 10, FColor::Blue, false, 0.2);
				DrawDebugLine(World, ControllingPawn->GetActorLocation(), RogueCharacter->GetActorLocation(), FColor::Blue, false, 0.2);
				return;
			}
		}
	}

	DrawDebugSphere(World, Center, DetectRadius, 16, FColor::Red, false, 0.2);
}