// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "RogueAnimInstance.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnNextAttackCheckDelegate);
DECLARE_MULTICAST_DELEGATE(FOnAttackHitCheckDelegate);
DECLARE_MULTICAST_DELEGATE(FOnDashAttackHitCheckDelegate);
DECLARE_MULTICAST_DELEGATE(FOnJumpAttackHitCheckDelegate);
DECLARE_MULTICAST_DELEGATE(FOnSkillHitCheckDelegate);
DECLARE_MULTICAST_DELEGATE(FOnDodgeEffectDelegate);

/**
 * 
 */
UCLASS()
class ROGUE_API URogueAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	URogueAnimInstance();

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	void PlayAttackMontage();
	void JumpToAttackMontageSection(int32 NewSection);

public:
	FOnNextAttackCheckDelegate OnNextAttackCheck;
	FOnAttackHitCheckDelegate OnAttackHitCheck;
	FOnDashAttackHitCheckDelegate OnDashAttackHitCheck;
	FOnJumpAttackHitCheckDelegate OnJumpAttackHitCheck;
	FOnSkillHitCheckDelegate OnSkillHitCheck;
	FOnDodgeEffectDelegate OnDodgeEffect;

	void SetDeadAnim() { IsDead = true; }

private:
	UFUNCTION()
	void AnimNotify_AttackHitCheck();

	UFUNCTION()
	void AnimNotify_NextAttackCheck();

	UFUNCTION()
	void AnimNotify_DashAttackHitCheck();

	UFUNCTION()
	void AnimNotify_JumpAttackHitCheck();

	UFUNCTION()
	void AnimNotify_SkillHitCheck();

	FName GetAttackMontageSectionName(int32 Section);

	UFUNCTION()
	void AnimNotify_DodgeEffect();

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pawn, Meta = (AllowPrivateAccess = true))
	bool IsInAir;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Attack, Meta = (AllowPrivateAccess = true))
	UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pawn, Meta = (AllowPrivateAccess = true))
	bool IsDead;
};
