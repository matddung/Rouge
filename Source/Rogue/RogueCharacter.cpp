#include "RogueCharacter.h"
#include "RogueAnimInstance.h"
#include "RogueCharacterStatComponent.h"
#include "RogueUserWidget.h"
#include "FloatingDamageActor.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystemComponent.h"

ARogueCharacter::ARogueCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = 400;
    SpringArm->bUsePawnControlRotation = true;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);
    Camera->bUsePawnControlRotation = false;

    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    
    TargetSpeed = WalkSpeed;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

    GetCharacterMovement()->JumpZVelocity = 500;

    GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);

    static ConstructorHelpers::FClassFinder<UAnimInstance> AttackAnim(TEXT("/Game/Blueprints/ABP_RogueCharacter"));

    if (AttackAnim.Succeeded())
    {
        GetMesh()->SetAnimInstanceClass(AttackAnim.Class);
    }

    AttackEndComboState();

    GetCapsuleComponent()->SetCollisionProfileName(TEXT("RogueCharacter"));

    CharacterStat = CreateDefaultSubobject<URogueCharacterStatComponent>(TEXT("CharacterStat"));
}

void ARogueCharacter::BeginPlay()
{
	Super::BeginPlay();

    check(CharacterStat);
	
    if (StatusWidgetClass)
    {
        StatusWidget = CreateWidget<URogueUserWidget>(GetWorld(), StatusWidgetClass);
        if (StatusWidget)
        {
            StatusWidget->AddToViewport();
        }
    }
}

void ARogueCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    UpdateMovementSpeed(DeltaTime);
    HandleStaminaLogic(DeltaTime);
    UpdateStatusWidget();

    UE_LOG(LogTemp, Warning, TEXT("ActionState: %s, AttackType: %s"),
        *UEnum::GetValueAsString(ActionState),
        *UEnum::GetValueAsString(AttackType));
}

void ARogueCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    RogueAnim = Cast<URogueAnimInstance>(GetMesh()->GetAnimInstance());
    if (!ensureMsgf(RogueAnim != nullptr, TEXT("PostInitializeComponents AnimInstance is nullptr")))
    {
        return;
    }

    RogueAnim->OnMontageEnded.AddDynamic(this, &ARogueCharacter::OnAttackMontageEnded);

    RogueAnim->OnNextAttackCheck.AddLambda([this]() -> void {
        if (ActionState != ECharacterActionState::Attacking || AttackType != EAttackType::Combo)
        {
            return;
        }

        if (IsComboInputOn)
        {
            if (!CharacterStat->ConsumeStamina(AttackCost))
            {
                UE_LOG(LogTemp, Warning, TEXT("Not enough stamina for combo"));
                return;
            }

            AttackStartComboState();
            RogueAnim->JumpToAttackMontageSection(CurrentCombo);
        }
    });

    RogueAnim->OnAttackHitCheck.AddLambda([this]() {
        PerformAttackHit(EAttackType::Combo);
    });

    RogueAnim->OnDashAttackHitCheck.AddLambda([this]() -> void {
        PerformAttackHit(EAttackType::Dash);
    });

    RogueAnim->OnJumpAttackHitCheck.AddLambda([this]() {
        PerformAttackHit(EAttackType::Jump);
    });

    RogueAnim->OnSkillHitCheck.AddLambda([this]() {
        PerformAttackHit(EAttackType::Skill);
    });

    RogueAnim->OnDodgeEffectStart.AddLambda([this]() {
        HandleDodgeEffectStart();
    });

    RogueAnim->OnDodgeEffectEnd.AddLambda([this]() {
        HandleDodgeEffectEnd();
    });

    CharacterStat->OnHPIsZero.AddLambda([this]() -> void {
        SetActorEnableCollision(false);
        RogueAnim->SetDeadAnim();
     });
}

float ARogueCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
    if (AttackType == EAttackType::Skill || bIsDodgeInvincible)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invincible No damage taken"));
        return 0;
    }

    float FinalDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    CharacterStat->SetDamage(FinalDamage);

    if (CharacterStat->GetCurrentHP() <= 0 && EventInstigator)
    {
        ARogueCharacter* Killer = Cast<ARogueCharacter>(EventInstigator->GetPawn());
        if (Killer && Killer != this)
        {
            Killer->CharacterStat->AddExp(10000);
        }
    }

    return FinalDamage;
}

void ARogueCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ARogueCharacter::StartSprinting);
    PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ARogueCharacter::StopSprinting);
    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ARogueCharacter::Jump);
    PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &ARogueCharacter::Attack);
    PlayerInputComponent->BindAction("UseSkill", IE_Pressed, this, &ARogueCharacter::UseSkill);
    PlayerInputComponent->BindAction("ZoomInCamera", IE_Pressed, this, &ARogueCharacter::ZoomInCamera);
    PlayerInputComponent->BindAction("ZoomOutCamera", IE_Pressed, this, &ARogueCharacter::ZoomOutCamera);
    PlayerInputComponent->BindAction("Dodge", IE_Pressed, this, &ARogueCharacter::Dodge);

    PlayerInputComponent->BindAxis("MoveForward", this, &ARogueCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &ARogueCharacter::MoveRight);
    PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
    PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
}

void ARogueCharacter::UpdateMovementSpeed(float DeltaTime)
{
    float CurrentSpeed = GetCharacterMovement()->MaxWalkSpeed;
    float NewSpeed = FMath::FInterpTo(CurrentSpeed, TargetSpeed, DeltaTime, SpeedInterpRate);
    GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
}

void ARogueCharacter::HandleStaminaLogic(float DeltaTime)
{
    if (CharacterStat)
    {
        if (!bWantsToSprint && ActionState == ECharacterActionState::Idle)
        {
            CharacterStat->RecoverStamina(DeltaTime);
        }

        if (bWantsToSprint && !CharacterStat->ConsumeStamina(SprintStaminaCostPerSec * DeltaTime))
        {
            StopSprinting();
        }
    }
}

void ARogueCharacter::UpdateStatusWidget()
{
    if (!StatusWidget || !CharacterStat) return;

    StatusWidget->UpdateHP(CharacterStat->GetCurrentHP(), CharacterStat->GetMaxHP());
    StatusWidget->UpdateStamina(CharacterStat->GetCurrentStamina(), CharacterStat->GetMaxStamina());
    StatusWidget->UpdateLevel(CharacterStat->GetLevel());
    StatusWidget->UpdateExpBar(CharacterStat->GetCurrentExp(), CharacterStat->GetNextExp());

    float RemainingCooldown = FMath::Max(0.0f, SkillCooldownTime - (GetWorld()->GetTimeSeconds() - LastSkillTime));
    StatusWidget->UpdateSkillCooldown(RemainingCooldown, SkillCooldownTime);
}

void ARogueCharacter::MoveForward(float Value)
{
    if (AttackType == EAttackType::Skill || Controller == nullptr || Value == 0.0f)
    {
        return;
    }

    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0, Rotation.Yaw, 0);
    const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    AddMovementInput(Direction, Value);
}

void ARogueCharacter::MoveRight(float Value)
{
    if (AttackType == EAttackType::Skill || Controller == nullptr || Value == 0.0f)
    {
        return;
    }

    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0, Rotation.Yaw, 0);
    const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
    AddMovementInput(Direction, Value);
}

void ARogueCharacter::ZoomInCamera()
{
    if (!SpringArm) return;

    float NewLength = SpringArm->TargetArmLength - ZoomStep;
    SpringArm->TargetArmLength = FMath::Clamp(NewLength, MinZoomLength, MaxZoomLength);
}

void ARogueCharacter::ZoomOutCamera()
{
    if (!SpringArm) return;

    float NewLength = SpringArm->TargetArmLength + ZoomStep;
    SpringArm->TargetArmLength = FMath::Clamp(NewLength, MinZoomLength, MaxZoomLength);
}

void ARogueCharacter::StartSprinting()
{
    bWantsToSprint = true;
    TargetSpeed = RunSpeed;
}

void ARogueCharacter::StopSprinting()
{
    bWantsToSprint = false;
    TargetSpeed = WalkSpeed;
}

void ARogueCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);

    ActionState = ECharacterActionState::Idle;
    AttackType = EAttackType::None;
}

void ARogueCharacter::Jump()
{
    if (!CharacterStat->ConsumeStamina(JumpStaminaCost))
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough stamina to jump"));
        return;
    }

    if (ActionState != ECharacterActionState::Idle)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot jump while attacking"));
        return;
    }

    Super::Jump();
}

void ARogueCharacter::SpawnDamageText(AActor* DamagedActor, float Damage)
{
    if (!DamageTextActorClass || !DamagedActor) return;

    FVector TargetLocation = DamagedActor->GetActorLocation() + FVector(0.f, 0.f, 100.f);
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AFloatingDamageActor* DamageText = GetWorld()->SpawnActor<AFloatingDamageActor>(DamageTextActorClass, TargetLocation, FRotator::ZeroRotator, Params);
    if (DamageText)
    {
        DamageText->SetDamage(Damage);
    }
}

void ARogueCharacter::PerformAttackHit(EAttackType PerformAttackType)
{
    EAttackCollisionType ShapeType;
    float Radius = 0;
    float HalfHeight = 0;
    float Distance = 0;
    float DamageMultiplier = 1;
    FVector Direction = GetActorForwardVector();

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
        Direction = (GetActorForwardVector() + FVector(0, 0, -1)).GetSafeNormal();
        DamageMultiplier = 1.25;
        break;

    case EAttackType::Skill:
        if (SkillEffect)
        {
            SkillEffectComponent = UGameplayStatics::SpawnEmitterAttached(
                SkillEffect,
                GetRootComponent(),
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

    FVector Start = GetActorLocation() + FVector(0, 0, 50);
    FVector End = Start + Direction * Distance;

    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params(NAME_None, false, this);
    Params.AddIgnoredActor(this);

    bool bHit = false;

    if (ShapeType == EAttackCollisionType::Sphere)
    {
        FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
        bHit = GetWorld()->OverlapMultiByChannel(Overlaps, End, FQuat::Identity, ECC_Pawn, Shape, Params);

#if WITH_EDITOR
        FVector DrawCenter = GetActorLocation() + FVector(0, 0, 50);
        DrawDebugSphere(GetWorld(), DrawCenter, Radius, 24, FColor::Red, false, 1);
#endif
    }
    else if (ShapeType == EAttackCollisionType::Capsule)
    {
        FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
        bHit = GetWorld()->OverlapMultiByChannel(Overlaps, End, FQuat::Identity, ECC_Pawn, Shape, Params);

#if WITH_EDITOR
        FVector TraceVec = Direction * Distance;
        FVector Center = GetActorLocation() + TraceVec * 0.5f + FVector(0, 0, 50);
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
        if (HitActor && HitActor != this)
        {
            float Damage = CharacterStat->GetAttack() * DamageMultiplier;
            FDamageEvent DamageEvent;
            HitActor->TakeDamage(Damage, DamageEvent, GetController(), this);

            if (ACharacter* DamagedCharacter = Cast<ACharacter>(HitActor))
            {
                SpawnDamageText(HitActor, Damage);
            }
        }
    }
}

void ARogueCharacter::Attack()
{
    if (ActionState == ECharacterActionState::Jumping || ActionState == ECharacterActionState::Dodging)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot attack during dodge or while hidden"));
        return;
    }

    if (ActionState == ECharacterActionState::Attacking && AttackType == EAttackType::Combo)
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

    if (RogueAnim->IsInAir)
    {
        JumpAttack();
        return;
    }

    float CurrentSpeed = GetVelocity().Size();
    if (bWantsToSprint && CurrentSpeed >= RunSpeed - 30 && !RogueAnim->IsInAir)
    {
        DashAttack();
        return;
    }

    TargetSpeed = WalkSpeed;

    if (!CharacterStat->ConsumeStamina(AttackCost))
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough stamina to attack"));
        return;
    }

    AttackStartComboState();
    RogueAnim->PlayAttackMontage();
    RogueAnim->JumpToAttackMontageSection(CurrentCombo);
    ActionState = ECharacterActionState::Attacking;
    AttackType = EAttackType::Combo;
}

void ARogueCharacter::DashAttack()
{
    if (!CharacterStat->ConsumeStamina(AttackCost))
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough stamina for dash attack"));
        return;
    }

    TargetSpeed = WalkSpeed;
    ActionState = ECharacterActionState::Attacking;
    AttackType = EAttackType::Dash;

    FVector Forward = GetActorForwardVector();

    if (DashAttackMontage && RogueAnim)
    {
        RogueAnim->Montage_Play(DashAttackMontage);
        LaunchCharacter(Forward * 1000, true, true);
    }
}

void ARogueCharacter::JumpAttack()
{
    if (!JumpAttackMontage || !RogueAnim)
    {
        return;
    }

    if (ActionState == ECharacterActionState::Attacking)
    {
        return;
    }

    if (!CharacterStat->ConsumeStamina(AttackCost))
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough stamina to jump attack"));
        return;
    }

    ActionState = ECharacterActionState::Attacking;
    AttackType = EAttackType::Jump;
    RogueAnim->Montage_Play(JumpAttackMontage);
}

void ARogueCharacter::UseSkill()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastSkillTime < SkillCooldownTime)
    {
        float RemainingTime = SkillCooldownTime - (CurrentTime - LastSkillTime);
        UE_LOG(LogTemp, Warning, TEXT("Skill is on cooldown: %.1f seconds left"), RemainingTime);
        return;
    }

    if (ActionState != ECharacterActionState::Idle)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot use skill now"));
        return;
    }

    if (!SkillMontage || !RogueAnim)
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillMontage or AnimInstance missing"));
        return;
    }

    if (!CharacterStat->ConsumeStamina(SkillStaminaCost))
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough stamina to use skill"));
        return;
    }

    LastSkillTime = CurrentTime;

    ActionState = ECharacterActionState::Attacking;
    AttackType = EAttackType::Skill;

    RogueAnim->Montage_Play(SkillMontage);
}

void ARogueCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    ActionState = ECharacterActionState::Idle;
    AttackType = EAttackType::None;

    if (Montage == DodgeMontage)
    {
        bIsDodgeInvincible = false;
        return;
    }

    if (Montage == DashAttackMontage || Montage == JumpAttackMontage || Montage == SkillMontage)
    {
        if (SkillEffectComponent)
        {
            SkillEffectComponent->DeactivateSystem();
            SkillEffectComponent = nullptr;
        }

        if (bWantsToSprint)
        {
            TargetSpeed = RunSpeed;
        }
        return;
    }

    if (!ensureMsgf(CurrentCombo > 0, TEXT("OnAttackMontageEnded CurrentCombo > 0"))) return;

    AttackEndComboState();

    if (bWantsToSprint)
    {
        TargetSpeed = RunSpeed;
    }
}

void ARogueCharacter::AttackStartComboState()
{
    CanNextCombo = true;
    IsComboInputOn = false;
    CurrentCombo = FMath::Clamp(CurrentCombo + 1, 1, MaxCombo);
}

void ARogueCharacter::AttackEndComboState()
{
    IsComboInputOn = false;
    CanNextCombo = false;
    CurrentCombo = 0;
}

void ARogueCharacter::Dodge()
{
    if (ActionState != ECharacterActionState::Idle || AttackType == EAttackType::Skill || RogueAnim->IsInAir) return;

    if (!CharacterStat->ConsumeStamina(DodgeStaminaCost))
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough stamina to dodge"));
        return;
    }

    FVector InputDir = GetLastMovementInputVector().GetSafeNormal();
    if (InputDir.IsNearlyZero())
    {
        InputDir = GetActorForwardVector();
    }

    DodgeDirection = InputDir;

    ActionState = ECharacterActionState::Dodging;
    bDidDodgeTeleport = false;

    if (DodgeMontage && RogueAnim)
    {
        RogueAnim->Montage_Play(DodgeMontage);
    }
}

void ARogueCharacter::HandleDodgeEffectStart()
{
    bIsDodgeInvincible = true;
    SetActorHiddenInGame(true);
    if (DodgeEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DodgeEffect, GetActorLocation(), GetActorRotation());
    }

    if (!bDidDodgeTeleport)
    {
        FVector NewLocation = GetActorLocation() + DodgeDirection * DodgeDistance;
        FHitResult HitResult;
        bool bMoved = SetActorLocation(NewLocation, true, &HitResult, ETeleportType::TeleportPhysics);

        if (!bMoved)
        {
            UE_LOG(LogTemp, Warning, TEXT("Dodge blocked"));
        }

        bDidDodgeTeleport = true;
    }
}

void ARogueCharacter::HandleDodgeEffectEnd()
{
    bIsDodgeInvincible = false;
    SetActorHiddenInGame(false);
}