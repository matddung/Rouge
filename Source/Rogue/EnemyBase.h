#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Character.h"
#include "EnemyBase.generated.h"

USTRUCT(BlueprintType)
struct FEnemyStats : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString EnemyID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AttackDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MovementSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AttackRange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AttackCooldown;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ExpReward;
};

DECLARE_MULTICAST_DELEGATE(FOnAttackEndDelegate);

UCLASS()
class ROGUE_API AEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
    AEnemyBase();

    void Attack();

protected:
    virtual void BeginPlay() override;

    virtual void Tick(float DeltaTime) override;

    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

private:
    UFUNCTION()
    virtual void Die();

    UFUNCTION()
    void HandleAttackHitCheck();

    void ResetAttack();

    void ClearAttackCooldown();

    void DestroySelf();

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    FEnemyStats EnemyStats;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
    FName EnemyRowName;

    bool bIsAttacking = false;

    UPROPERTY(EditAnywhere)
    UAnimMontage* AttackMontage;

    UPROPERTY()
    class UNormalEnemyAnimInstance* Anim;

private:
    UPROPERTY(EditDefaultsOnly, Category = "Data")
    UDataTable* EnemyDataTable;

    UPROPERTY(VisibleAnywhere, Category = "Stats")
    float CurrentHealth;

    FOnAttackEndDelegate OnAttackEnd;

    bool bAttackOnCooldown = false;

    FTimerHandle AttackCooldownHandle;
};