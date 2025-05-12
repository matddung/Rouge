// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RogueCharacterCombatComponent.generated.h"

UENUM(BlueprintType)
enum class EAttackType : uint8
{
	None,
	Combo,
	Jump,
	Dash,
	Skill
};

UENUM()
enum class EAttackCollisionType : uint8
{
	Sphere,
	Capsule
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ROGUE_API URogueCharacterCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	URogueCharacterCombatComponent();

protected:
	virtual void BeginPlay() override;

public:	
	void Attack();
	void DashAttack();
	void JumpAttack();
	void UseSkill();

	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void AttackStartComboState();
	void AttackEndComboState();

	void PerformAttackHit(EAttackType PerformAttackType);

	EAttackType GetAttackType() { return AttackType; };
	void SetAttackType(EAttackType AttackTypeValue) { AttackType = AttackTypeValue; };

	void InitializeWithAnim(class URogueAnimInstance* AnimInstance);

public:
	float SkillCooldownTime = 30;
	float LastSkillTime = -SkillCooldownTime;

private:
	UPROPERTY()
	class ARogueCharacter* Character;

	EAttackType AttackType = EAttackType::None;

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	bool CanNextCombo;

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	bool IsComboInputOn;

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	int32 CurrentCombo;

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	int32 MaxCombo = 4;

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	float AttackRange = 150;

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	float AttackRadius = 50;

	UPROPERTY()
	class URogueAnimInstance* RogueAnim;

	UPROPERTY(EditAnywhere, Category = "Attack")
	UAnimMontage* DashAttackMontage;

	UPROPERTY(EditAnywhere, Category = "Attack")
	UAnimMontage* JumpAttackMontage;

	UPROPERTY(EditAnywhere, Category = "Attack")
	UAnimMontage* SkillMontage;

	UPROPERTY(EditAnywhere, Category = "Attack")
	UParticleSystem* SkillEffect;

	UPROPERTY()
	UParticleSystemComponent* SkillEffectComponent = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Stat")
	class URogueCharacterStatComponent* CharacterStat;
	
	UPROPERTY()
	class URogueCharacterDodgeComponent* DodgeComponent;

	float AttackCost = 10;
	float SkillStaminaCost = 50;
};
