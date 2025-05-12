// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueCharacterCombatComponent.h"
#include "RogueCharacter.h"
#include "RogueAnimInstance.h"
#include "RogueCharacterStatComponent.h"
#include "RogueCharacterDodgeComponent.h"

#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

// Sets default values for this component's properties
URogueCharacterCombatComponent::URogueCharacterCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void URogueCharacterCombatComponent::BeginPlay()
{
	Super::BeginPlay();

    Character = Cast<ARogueCharacter>(GetOwner());
    if (!Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatComponent: Owner is not ARogueCharacter"));
    }

    DodgeComponent = Character->FindComponentByClass<URogueCharacterDodgeComponent>();
    if (!DodgeComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatComponent: DodgeComponent not found!"));
    }

    CharacterStat = Character->FindComponentByClass<URogueCharacterStatComponent>();
    if (!CharacterStat)
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatComponent : CharacterStat not found!"));
    }
}

void URogueCharacterCombatComponent::Attack()
{
    
    if (Character->GetActionState() == ECharacterActionState::Attacking && AttackType != EAttackType::Combo)
    {
        return;
    }

    if (Character->GetActionState() == ECharacterActionState::Dodging || AttackType == EAttackType::Skill)
    {
        return;
    }

    if (Character->GetActionState() == ECharacterActionState::Attacking && AttackType == EAttackType::Combo)
    {
        if (!ensureMsgf((FMath::IsWithinInclusive<int32>(CurrentCombo, 1, MaxCombo)), TEXT("Attack IsAttacking true")))
        {
            return;
        }

        if (CanNextCombo)
        {
            IsComboInputOn = true;
        }

        return;
    }

    if (!ensureMsgf(CurrentCombo == 0, TEXT("Attack IsAttacking false")))
    {
        return;
    }

    if (!RogueAnim || RogueAnim->IsInAir)
    {
        JumpAttack();
        return;
    }

    float CurrentSpeed = Character->GetVelocity().Size();
    if (Character->bWantsToSprint && CurrentSpeed >= Character->GetRunSpeed() - 30 && !RogueAnim->IsInAir)
    {
        DashAttack();
        return;
    }

    Character->TargetSpeed = Character->GetWalkSpeed();

    if (!CharacterStat || !CharacterStat->ConsumeStamina(AttackCost))
    {
        return;
    }

    AttackStartComboState();
    RogueAnim->PlayAttackMontage();
    RogueAnim->JumpToAttackMontageSection(CurrentCombo);
    Character->SetActionState(ECharacterActionState::Attacking);
    AttackType = EAttackType::Combo;
}

void URogueCharacterCombatComponent::DashAttack()
{
    if (!CharacterStat || !CharacterStat->ConsumeStamina(AttackCost))
    {
        return;
    }

    Character->TargetSpeed = Character->GetWalkSpeed();
    Character->SetActionState(ECharacterActionState::Attacking);
    AttackType = EAttackType::Dash;

    FVector Forward = Character->GetActorForwardVector();

    if (DashAttackMontage && RogueAnim)
    {
        RogueAnim->Montage_Play(DashAttackMontage);
    }
}

void URogueCharacterCombatComponent::JumpAttack()
{
    if (!JumpAttackMontage || !RogueAnim)
    {
        return;
    }

    if (Character->GetActionState() == ECharacterActionState::Attacking)
    {
        return;
    }

    if (!CharacterStat || !CharacterStat->ConsumeStamina(AttackCost))
    {
        return;
    }

    Character->SetActionState(ECharacterActionState::Attacking);
    AttackType = EAttackType::Jump;
    RogueAnim->Montage_Play(JumpAttackMontage);
}

void URogueCharacterCombatComponent::UseSkill()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastSkillTime < SkillCooldownTime)
    {
        float RemainingTime = SkillCooldownTime - (CurrentTime - LastSkillTime);
        return;
    }

    if (Character->GetActionState() != ECharacterActionState::Idle &&
        Character->GetActionState() != ECharacterActionState::Walking &&
        Character->GetActionState() != ECharacterActionState::Running)
    {
        return;
    }

    if (!SkillMontage || !RogueAnim)
    {
        return;
    }

    if (!CharacterStat || !CharacterStat->ConsumeStamina(SkillStaminaCost))
    {
        return;
    }

    LastSkillTime = CurrentTime;

    Character->SetActionState(ECharacterActionState::Attacking);
    AttackType = EAttackType::Skill;

    RogueAnim->Montage_Play(SkillMontage);
}

void URogueCharacterCombatComponent::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    Character->SetActionState(ECharacterActionState::Idle);
    AttackType = EAttackType::None;

    if (Montage == DodgeComponent->DodgeMontage)
    {
        DodgeComponent->bIsDodgeInvincible = false;
        return;
    }

    if (Montage == DashAttackMontage || Montage == JumpAttackMontage || Montage == SkillMontage)
    {
        if (SkillEffectComponent)
        {
            SkillEffectComponent->DeactivateSystem();
            SkillEffectComponent = nullptr;
        }

        if (Character->bWantsToSprint)
        {
            Character->TargetSpeed = Character->GetRunSpeed();
        }
        return;
    }

    if (!ensureMsgf(CurrentCombo > 0, TEXT("OnAttackMontageEnded CurrentCombo > 0"))) return;

    AttackEndComboState();

    if (Character->bWantsToSprint)
    {
        Character->TargetSpeed = Character->GetRunSpeed();
    }
}

void URogueCharacterCombatComponent::AttackStartComboState()
{
    CanNextCombo = true;
    IsComboInputOn = false;
    CurrentCombo = FMath::Clamp(CurrentCombo + 1, 1, MaxCombo);
}

void URogueCharacterCombatComponent::AttackEndComboState()
{
    IsComboInputOn = false;
    CanNextCombo = false;
    CurrentCombo = 0;
}

void URogueCharacterCombatComponent::PerformAttackHit(EAttackType PerformAttackType)
{
    EAttackCollisionType ShapeType;
    float Radius = 0;
    float HalfHeight = 0;
    float Distance = 0;
    float DamageMultiplier = 1;
    FVector Direction = Character->GetActorForwardVector();

    switch (PerformAttackType)
    {
    case EAttackType::Combo:
        ShapeType = EAttackCollisionType::Capsule;
        Radius = AttackRadius;
        HalfHeight = AttackRange * 0.5;
        Distance = AttackRange;
        DamageMultiplier = 1;
        break;

    case EAttackType::Dash:
        ShapeType = EAttackCollisionType::Sphere;
        Radius = 150;
        DamageMultiplier = 1.5;
        Direction = FVector::ZeroVector;
        break;

    case EAttackType::Jump:
        ShapeType = EAttackCollisionType::Capsule;
        Radius = 60;
        HalfHeight = 100;
        Distance = 200;
        Direction = (Character->GetActorForwardVector() + FVector(0, 0, -1)).GetSafeNormal();
        DamageMultiplier = 1.25;
        break;

    case EAttackType::Skill:
        if (SkillEffect)
        {
            SkillEffectComponent = UGameplayStatics::SpawnEmitterAttached(
                SkillEffect,
                Character->GetRootComponent(),
                NAME_None,
                FVector::ZeroVector,
                FRotator::ZeroRotator,
                EAttachLocation::KeepRelativeOffset,
                true
            );

            if (SkillEffectComponent)
            {
                SkillEffectComponent->SetWorldScale3D(FVector(2.5));
            }
        }

        ShapeType = EAttackCollisionType::Sphere;
        Radius = 550;
        DamageMultiplier = 5;
        Direction = FVector::ZeroVector;
        break;

    default:
        return;
    }

    FVector Start = Character->GetActorLocation() + FVector(0, 0, 50);
    FVector End = Start + Direction * Distance;

    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params(NAME_None, false, Character);
    Params.AddIgnoredActor(Character);

    bool bHit = false;

    if (ShapeType == EAttackCollisionType::Sphere)
    {
        FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
        bHit = GetWorld()->OverlapMultiByChannel(Overlaps, End, FQuat::Identity, ECC_Pawn, Shape, Params);

#if WITH_EDITOR
        FVector DrawCenter = Character->GetActorLocation() + FVector(0, 0, 50);
        DrawDebugSphere(GetWorld(), DrawCenter, Radius, 24, FColor::Red, false, 1);
#endif
    }
    else if (ShapeType == EAttackCollisionType::Capsule)
    {
        FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
        bHit = GetWorld()->OverlapMultiByChannel(Overlaps, End, FQuat::Identity, ECC_Pawn, Shape, Params);

#if WITH_EDITOR
        FVector TraceVec = Direction * Distance;
        FVector Center = Character->GetActorLocation() + TraceVec * 0.5f + FVector(0, 0, 50);
        float DebugHalfHeight = HalfHeight + Radius;

        FQuat CapsuleRot = FRotationMatrix::MakeFromZ(TraceVec).ToQuat();

        DrawDebugCapsule(GetWorld(),
            Center,
            DebugHalfHeight,
            Radius,
            CapsuleRot,
            FColor::Red,
            false,
            1);
#endif
    }

    for (const auto& Result : Overlaps)
    {
        AActor* HitActor = Result.GetActor();
        if (HitActor && HitActor != Character)
        {
            float Damage = CharacterStat->GetAttack() * DamageMultiplier;
            FDamageEvent DamageEvent;
            HitActor->TakeDamage(Damage, DamageEvent, Character->GetController(), Character);

            if (ACharacter* DamagedCharacter = Cast<ACharacter>(HitActor))
            {
                Character->SpawnDamageText(HitActor, Damage);
            }
        }
    }
}

void URogueCharacterCombatComponent::InitializeWithAnim(URogueAnimInstance* AnimInstance)
{
    if (!AnimInstance) return;

    RogueAnim = AnimInstance;

    RogueAnim->OnMontageEnded.AddDynamic(this, &URogueCharacterCombatComponent::OnAttackMontageEnded);

    RogueAnim->OnNextAttackCheck.AddLambda([this]() {
        if (Character->GetActionState() != ECharacterActionState::Attacking || AttackType != EAttackType::Combo)
            return;

        if (IsComboInputOn)
        {
            if (!CharacterStat || !CharacterStat->ConsumeStamina(AttackCost))
                return;

            AttackStartComboState();
            RogueAnim->JumpToAttackMontageSection(CurrentCombo);
        }
        });

    RogueAnim->OnAttackHitCheck.AddLambda([this]() {
        PerformAttackHit(EAttackType::Combo);
        });

    RogueAnim->OnDashAttackHitCheck.AddLambda([this]() {
        PerformAttackHit(EAttackType::Dash);
        });

    RogueAnim->OnJumpAttackHitCheck.AddLambda([this]() {
        PerformAttackHit(EAttackType::Jump);
        });

    RogueAnim->OnSkillHitCheck.AddLambda([this]() {
        PerformAttackHit(EAttackType::Skill);
        });
}
