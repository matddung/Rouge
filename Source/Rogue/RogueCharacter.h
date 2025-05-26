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
	float GetWalkSpeed() { return WalkSpeed; };
	float GetRunSpeed() { return RunSpeed; };

	ECharacterActionState GetActionState() { return ActionState; };
	void SetActionState(ECharacterActionState NewState) { ActionState = NewState; }

	void SpawnDamageText(AActor* DamagedActor, float Damage);

private:
	ARogueCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void PostInitializeComponents() override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void Jump() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

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

public:
	float TargetSpeed;
	bool bWantsToSprint = false;

	float SprintStaminaCostPerSec = 5;
	float JumpStaminaCost = 5;

private:
	ECharacterActionState ActionState = ECharacterActionState::Idle;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float WalkSpeed = 160;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float RunSpeed = 600;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float SpeedInterpRate = 4;

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

	UPROPERTY(VisibleAnywhere, Category = "Stat")
	class URogueCharacterStatComponent* StatComponent;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	class URogueCharacterCombatComponent* CombatComponent;

	UPROPERTY(VisibleAnywhere, Category = "Dodge")
	class URogueCharacterDodgeComponent* DodgeComponent;

	UPROPERTY(VisibleAnywhere, Category = "UI")
	class URogueUserWidget* StatusWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> StatusWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class AFloatingDamageActor> DamageTextActorClass;

	UPROPERTY()
	class URogueAnimInstance* RogueAnim;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float SearchRadius = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float ViewAngle = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float TurnSwitchThreshold = 0.2f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LockOn")
	AActor* CurrentTarget = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LockOn")
	TArray<AActor*> Targets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float DegreesPerSecond = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float CameraInterpSpeed = 5;

	void UpdateTargets();
	void AcquireNearestTarget();
	void SwitchTarget(float Direction);

	void ToggleLockOn();
	void OnTurn(float Value);
	void OnLookUp(float Value);
};