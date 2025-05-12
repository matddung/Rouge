#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RogueCharacter.generated.h"

UENUM(BlueprintType)
enum class ECharacterActionState : uint8
{
	Idle,
	Walking,
	Running,
	Attacking,
	Dodging,
	Jumping,
	Dead
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

	ECharacterActionState GetActionState() { return ActionState; };
	void SetActionState(ECharacterActionState NewState) { ActionState = NewState; }

	float GetWalkSpeed() { return WalkSpeed; };
	float GetRunSpeed() { return RunSpeed; };

	void SpawnDamageText(AActor* DamagedActor, float Damage);

private:
	void UpdateMovementSpeed(float DeltaTime);
	void HandleStaminaLogic(float DeltaTime);
	void UpdateMovementState();
	void UpdateStatusWidget();

	void MoveForward(float Value);
	void MoveRight(float Value);

	void ZoomInCamera();
	void ZoomOutCamera();

	UFUNCTION()
	void StartSprinting();

	UFUNCTION()
	void StopSprinting();

	void InputAttack();
	void InputUseSkill();
	void InputDodge();

public:
	float TargetSpeed;
	bool bWantsToSprint = false;

	float SprintStaminaCostPerSec = 5;
	float JumpStaminaCost = 5;
	

private:
	ECharacterActionState ActionState = ECharacterActionState::Idle;

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

	UPROPERTY(VisibleAnywhere, Category = "Stat")
	class URogueCharacterStatComponent* CharacterStat;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	class URogueCharacterCombatComponent* CombatComponent;

	UPROPERTY(VisibleAnywhere, Category = "Dodge")
	class URogueCharacterDodgeComponent* DodgeComponent;

	UPROPERTY()
	class URogueUserWidget* StatusWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> StatusWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class AFloatingDamageActor> DamageTextActorClass;

	UPROPERTY()
	class URogueAnimInstance* RogueAnim;
};