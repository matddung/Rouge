// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RogueCharacter.generated.h"

UCLASS()
class ROGUE_API ARogueCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARogueCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void PostInitializeComponents() override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void Jump() override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, Category = "Stat")
	class URogueCharacterStatComponent* CharacterStat;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);

	void ZoomInCamera();

	void ZoomOutCamera();

	UFUNCTION()
	void StartSprinting();

	UFUNCTION()
	void StopSprinting();

	void Attack();
	void DashAttack();
	void DoDashAttackHit();

	void JumpAttack();
	void DoJumpAttackHit();

	void UseSkill();
	void DoSkillHit();

	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void AttackStartComboState();
	void AttackEndComboState();
	void AttackCheck();

	void Dodge();
	void HandleDodgeEffectStart();
	void HandleDodgeEffectEnd();

private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class UCameraComponent* Camera;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float MinZoomLength = 100;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float MaxZoomLength = 1000;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float ZoomStep = 100;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float WalkSpeed = 160;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float RunSpeed = 600;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float SpeedInterpRate = 4;

	float TargetSpeed;
	bool bWantsToSprint = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	bool IsAttacking;

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	bool CanNextCombo;

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	bool IsComboInputOn;

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	int32 CurrentCombo;

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	int32 MaxCombo;

	UPROPERTY()
	class URogueAnimInstance* RogueAnim;

	UPROPERTY(EditAnywhere, Category = "Attack")
	UAnimMontage* DashAttackMontage;

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	float AttackRange;

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	float AttackRadius;

	bool bIsSprintKeyDown = false;

	UPROPERTY(EditAnywhere, Category = "Attack")
	UAnimMontage* JumpAttackMontage;

	bool bIsJumpAttacking = false;
	bool bCanReceiveAttackInput = true;

	UPROPERTY(EditAnywhere, Category = "Skill")
	UAnimMontage* SkillMontage;

	bool bIsSkillInvincible = false;

	UPROPERTY(EditAnywhere, Category = "Skill")
	UParticleSystem* SkillEffect;

	UPROPERTY()
	UParticleSystemComponent* SkillEffectComponent = nullptr;

	bool bIsDodging = false;
	bool bIsDodgeInvincible = false;
	FVector DodgeDirection;

	UPROPERTY(EditAnywhere)
	float DodgeDistance = 300;

	UPROPERTY(EditAnywhere, Category = "Dodge")
	UAnimMontage* DodgeMontage;

	FTimerHandle DodgeTimerHandle;

	UPROPERTY(EditAnywhere, Category = "Dodge")
	UParticleSystem* DodgeEffect;

	bool bDidDodgeTeleport = false;
};