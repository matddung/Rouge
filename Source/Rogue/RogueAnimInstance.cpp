// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

URogueAnimInstance::URogueAnimInstance()
{
	IsInAir = false;
	IsDead = false;
	static ConstructorHelpers::FObjectFinder<UAnimMontage> Attack_Montage(TEXT("/Game/Blueprints/AM_Attack"));
	if (Attack_Montage.Succeeded())
	{
		AttackMontage = Attack_Montage.Object;
	}
}

void URogueAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	APawn* Pawn = TryGetPawnOwner();
	if (!::IsValid(Pawn)) return;

	if (!IsDead) {
		ACharacter* Character = Cast<ACharacter>(Pawn);
		if (Character) {
			IsInAir = Character->GetMovementComponent()->IsFalling();
		}
	}
}
void URogueAnimInstance::PlayAttackMontage()
{
	if (!ensureMsgf(!IsDead, TEXT("IsDead true")))
	{
		return;
	}
	Montage_Play(AttackMontage, 1);
}

void URogueAnimInstance::JumpToAttackMontageSection(int32 NewSection)
{
	if (!ensureMsgf(!IsDead, TEXT("IsDead true")))
	{
		return;
	}
	if (!ensureMsgf(Montage_IsPlaying(AttackMontage), TEXT("JumpToAttackMontageSection error")))
	{
		return;
	}
	Montage_JumpToSection(GetAttackMontageSectionName(NewSection), AttackMontage);
}

void URogueAnimInstance::AnimNotify_AttackHitCheck()
{
	OnAttackHitCheck.Broadcast();
}

void URogueAnimInstance::AnimNotify_NextAttackCheck()
{
	OnNextAttackCheck.Broadcast();
}

void URogueAnimInstance::AnimNotify_DashAttackHitCheck()
{
	OnDashAttackHitCheck.Broadcast();
}

void URogueAnimInstance::AnimNotify_JumpAttackHitCheck()
{
    OnJumpAttackHitCheck.Broadcast();
}

void URogueAnimInstance::AnimNotify_SkillHitCheck()
{
	OnSkillHitCheck.Broadcast();
}

FName URogueAnimInstance::GetAttackMontageSectionName(int32 Section)
{
	if (!ensureMsgf(FMath::IsWithinInclusive<int32>(Section, 1, 4), TEXT("GetAttackMontageSectionName error: Section %d is out of range (1~4)"), Section))
	{
		return NAME_None;
	}
	return FName(*FString::Printf(TEXT("Attack%d"), Section));
}

void URogueAnimInstance::AnimNotify_DodgeEffect()
{
	OnDodgeEffect.Broadcast();
}