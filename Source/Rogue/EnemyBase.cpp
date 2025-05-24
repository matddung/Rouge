#include "EnemyBase.h"
#include "NormalEnemyAnimInstance.h"
#include "RogueCharacterStatComponent.h"
#include "NormalEnemyAIController.h"
#include "RogueCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BlackboardComponent.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;
    AIControllerClass = ANormalEnemyAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bUseControllerDesiredRotation = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0, 480, 0);
}

void AEnemyBase::BeginPlay()
{
    Super::BeginPlay();

    if (EnemyDataTable)
    {
        if (FEnemyStats* Row = EnemyDataTable->FindRow<FEnemyStats>(EnemyRowName, TEXT("")))
        {
            EnemyStats = *Row;
            CurrentHealth = EnemyStats.MaxHealth;
            GetCharacterMovement()->MaxWalkSpeed = EnemyStats.MovementSpeed;
        }
    }

    Anim = Cast<UNormalEnemyAnimInstance>(GetMesh()->GetAnimInstance());
    if (Anim)
    {
        Anim->OnAttackHitCheck.AddUObject(this, &AEnemyBase::HandleAttackHitCheck);
        Anim->OnAttackEnd.AddUObject(this, &AEnemyBase::ResetAttack);
    }
}

void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

float AEnemyBase::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
    const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    if (ActualDamage > 0.0f)
    {
        CurrentHealth -= ActualDamage;

        if (CurrentHealth <= 0.0f)
        {
            Anim->SetDeadAnim();
            Die();
        }
    }

    return ActualDamage;
}

void AEnemyBase::Die()
{
    SetActorEnableCollision(false);

    if (Anim)
    {
        Anim->SetDeadAnim();
    }

    if (auto AI = Cast<ANormalEnemyAIController>(GetController()))
    {
        if (auto BB = AI->GetBlackboardComponent())
        {
            BB->SetValueAsBool(TEXT("IsDead"), true);
        }
    }

    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (!PlayerCharacter) return;

    URogueCharacterStatComponent* StatComp = PlayerCharacter->FindComponentByClass<URogueCharacterStatComponent>();
    if (StatComp)
    {
        StatComp->AddExp(EnemyStats.ExpReward);
    }

    FTimerHandle DestroyTimerHandle;
    GetWorldTimerManager().SetTimer(DestroyTimerHandle, this, &AEnemyBase::DestroySelf, 3, false);
}

void AEnemyBase::Attack()
{
    if (!Anim || Anim->GetIsDead() || bIsAttacking) return;

    if (GetWorldTimerManager().IsTimerActive(AttackCooldownHandle)) return;

    bIsAttacking = true;
    Anim->PlayRandomAttack();

    bAttackOnCooldown = true;
}

void AEnemyBase::HandleAttackHitCheck()
{
    ARogueCharacter* Rogue = Cast<ARogueCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    if (!Rogue) return;

    const float Distance = GetDistanceTo(Rogue);
    if (Distance > EnemyStats.AttackRange) return;

    if (URogueCharacterStatComponent* StatComp = Rogue->FindComponentByClass<URogueCharacterStatComponent>())
    {
        StatComp->SetDamage(EnemyStats.AttackDamage);
    }
}

void AEnemyBase::ResetAttack()
{
    bIsAttacking = false;

    GetWorldTimerManager().SetTimer(
        AttackCooldownHandle,
        this,
        &AEnemyBase::Attack,
        EnemyStats.AttackCooldown,
        false
    );
}

void AEnemyBase::ClearAttackCooldown()
{
    bAttackOnCooldown = false;
}

void AEnemyBase::DestroySelf()
{
    Destroy();
}