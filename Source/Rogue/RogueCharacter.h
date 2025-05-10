#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RogueCharacter.generated.h"

UENUM(BlueprintType)
enum class ECharacterActionState : uint8
{
	Idle,
	Attacking,
	Dodging,
	Jumping,
	Dead
};

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

UCLASS()
class ROGUE_API ARogueCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ARogueCharacter();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void PostInitializeComponents() override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void Jump() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void UpdateMovementSpeed(float DeltaTime);
	void HandleStaminaLogic(float DeltaTime);
	void UpdateStatusWidget();

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
	void JumpAttack();
	void UseSkill();

	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void AttackStartComboState();
	void AttackEndComboState();

	void Dodge();
	void HandleDodgeEffectStart();
	void HandleDodgeEffectEnd();

	void SpawnDamageText(AActor* DamagedActor, float Damage);

	void PerformAttackHit(EAttackType PerformAttackType);

private:
	ECharacterActionState ActionState = ECharacterActionState::Idle;

	EAttackType AttackType = EAttackType::None;

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

	UPROPERTY(VisibleAnywhere, Category = "Stat")
	class URogueCharacterStatComponent* CharacterStat;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> StatusWidgetClass;

	UPROPERTY()
	class URogueUserWidget* StatusWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class AFloatingDamageActor> DamageTextActorClass;

	/*UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	bool IsAttacking = false;*/

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	bool CanNextCombo;

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	bool IsComboInputOn;

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	int32 CurrentCombo;

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	int32 MaxCombo = 4;

	UPROPERTY()
	class URogueAnimInstance* RogueAnim;

	UPROPERTY(EditAnywhere, Category = "Attack")
	UAnimMontage* DashAttackMontage;

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	float AttackRange = 150;

	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	float AttackRadius = 50;

	UPROPERTY(EditAnywhere, Category = "Attack")
	UAnimMontage* JumpAttackMontage;

	//bool bIsJumpAttacking = false;

	UPROPERTY(EditAnywhere, Category = "Skill")
	UAnimMontage* SkillMontage;

	//bool bIsSkillInvincible = false;

	UPROPERTY(EditAnywhere, Category = "Skill")
	UParticleSystem* SkillEffect;

	UPROPERTY()
	UParticleSystemComponent* SkillEffectComponent = nullptr;

	float SkillCooldownTime = 30;
	float LastSkillTime = -SkillCooldownTime;

	//bool bIsDodging = false;
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

	float DodgeStaminaCost = 15;
	float SprintStaminaCostPerSec = 5;
	float AttackCost = 10;
	float JumpStaminaCost = 5;
	float SkillStaminaCost = 50;
};