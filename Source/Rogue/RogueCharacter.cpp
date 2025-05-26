#include "RogueCharacter.h"
#include "RogueAnimInstance.h"
#include "RogueCharacterStatComponent.h"
#include "RogueUserWidget.h"
#include "FloatingDamageActor.h"
#include "RogueCharacterCombatComponent.h"
#include "RogueCharacterDodgeComponent.h"
#include "EnemyBase.h"
#include "NormalEnemyAnimInstance.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

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

    GetCapsuleComponent()->SetCollisionProfileName(TEXT("RogueCharacter"));

    StatComponent = CreateDefaultSubobject<URogueCharacterStatComponent>(TEXT("StatComponent"));
    CombatComponent = CreateDefaultSubobject<URogueCharacterCombatComponent>(TEXT("CombatComponent"));
    DodgeComponent = CreateDefaultSubobject<URogueCharacterDodgeComponent>(TEXT("DodgeComponent"));

    CombatComponent->AttackEndComboState();
}

void ARogueCharacter::BeginPlay()
{
	Super::BeginPlay();

    check(StatComponent);
	
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
    UpdateMovementState();
    UpdateStatusWidget();

    if (CurrentTarget)
    {
        UpdateTargets();

        // 타겟 사망 처리
        if (AEnemyBase* E = Cast<AEnemyBase>(CurrentTarget))
        {
            if (E->Anim->GetIsDead())
            {
                AcquireNearestTarget();
                if (!CurrentTarget)
                {
                    ToggleLockOn();
                    GetCharacterMovement()->bOrientRotationToMovement = true;
                    return;
                }
            }
        }

        FVector MyLoc = GetActorLocation();
        FVector TargetLoc = CurrentTarget->GetActorLocation();
        FRotator DesiredRot = (TargetLoc - MyLoc).Rotation();

        // 부드러운 캐릭터 회전 (보간 사용)
        FRotator CurrRot = GetActorRotation();
        FRotator TargetRot(0.f, DesiredRot.Yaw, 0.f);
        FRotator NewRot = FMath::RInterpTo(CurrRot, TargetRot, DeltaTime, DegreesPerSecond);
        SetActorRotation(NewRot);

        // 카메라 피치 -30° 고정, 부드러운 보간
        DesiredRot.Pitch = -30.f;
        DesiredRot.Roll = 0.f;
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            FRotator CurrCam = PC->GetControlRotation();
            FRotator NewCam = FMath::RInterpTo(CurrCam, DesiredRot, DeltaTime, CameraInterpSpeed);
            PC->SetControlRotation(NewCam);
        }
    }
}

void ARogueCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    RogueAnim = Cast<URogueAnimInstance>(GetMesh()->GetAnimInstance());
    if (!ensureMsgf(RogueAnim != nullptr, TEXT("PostInitializeComponents AnimInstance is nullptr")))
    {
        return;
    }
    if (ensure(CombatComponent))
    {
        CombatComponent->InitializeWithAnim(RogueAnim);
    }
    if (ensure(DodgeComponent))
    {
        DodgeComponent->InitializeWithAnim(RogueAnim);
    }
    if (ensure(StatComponent))
    {
        StatComponent->OnHPIsZero.AddLambda([this]() {
            SetActorEnableCollision(false);
            RogueAnim->SetDeadAnim();
        });
    }
}

float ARogueCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
    if (CombatComponent->GetAttackType() == EAttackType::Skill || DodgeComponent->bIsDodgeInvincible)
    {
        return 0;
    }

    float FinalDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    StatComponent->SetDamage(FinalDamage);

    return FinalDamage;
}

void ARogueCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ARogueCharacter::StartSprinting);
    PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ARogueCharacter::StopSprinting);
    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ARogueCharacter::Jump);
    PlayerInputComponent->BindAction("Attack", IE_Pressed, CombatComponent, &URogueCharacterCombatComponent::ComboAttack);
    PlayerInputComponent->BindAction("UseSkill", IE_Pressed, CombatComponent, &URogueCharacterCombatComponent::UseSkill);
    PlayerInputComponent->BindAction("ZoomInCamera", IE_Pressed, this, &ARogueCharacter::ZoomInCamera);
    PlayerInputComponent->BindAction("ZoomOutCamera", IE_Pressed, this, &ARogueCharacter::ZoomOutCamera);
    PlayerInputComponent->BindAction("Dodge", IE_Pressed, DodgeComponent, &URogueCharacterDodgeComponent::Dodge);

    PlayerInputComponent->BindAxis("MoveForward", this, &ARogueCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &ARogueCharacter::MoveRight);
    
    PlayerInputComponent->BindAction("LockOn", IE_Pressed, this, &ARogueCharacter::ToggleLockOn);
    PlayerInputComponent->BindAxis("Turn", this, &ARogueCharacter::OnTurn);
    PlayerInputComponent->BindAxis("LookUp", this, &ARogueCharacter::OnLookUp);
}

void ARogueCharacter::UpdateMovementSpeed(float DeltaTime)
{
    float CurrentSpeed = GetCharacterMovement()->MaxWalkSpeed;
    float NewSpeed = FMath::FInterpTo(CurrentSpeed, TargetSpeed, DeltaTime, SpeedInterpRate);
    GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
}

void ARogueCharacter::HandleStaminaLogic(float DeltaTime)
{
    const float MovementSpeed = GetVelocity().Size();

    if ((ActionState == ECharacterActionState::Idle || ActionState == ECharacterActionState::Walking) && !GetCharacterMovement()->IsFalling())
    {
        StatComponent->RecoverStamina(DeltaTime);
    }

    const float SprintSpeedThreshold = RunSpeed * 0.7f;

    if (bWantsToSprint && ActionState == ECharacterActionState::Running)
    {
        if (!StatComponent->ConsumeStamina(SprintStaminaCostPerSec * DeltaTime))
        {
            StopSprinting();
        }
    }
}

void ARogueCharacter::UpdateMovementState()
{
    if (ActionState == ECharacterActionState::Jumping || ActionState == ECharacterActionState::Attacking ||
        ActionState == ECharacterActionState::Dodging || ActionState == ECharacterActionState::Dead)
    {
        return;
    }

    float Speed = GetVelocity().Size();

    const float WalkThreshold = WalkSpeed * 0.5f;
    const float RunThreshold = RunSpeed * 0.7f;

    if (Speed < 5.0f || GetCharacterMovement()->IsFalling())
    {
        ActionState = ECharacterActionState::Idle;
    }
    else if (Speed < RunThreshold)
    {
        ActionState = ECharacterActionState::Walking;
    }
    else
    {
        ActionState = ECharacterActionState::Running;
    }
}

void ARogueCharacter::UpdateStatusWidget()
{
    if (!StatusWidget) return;

    StatusWidget->UpdateHP(StatComponent->GetCurrentHP(), StatComponent->GetMaxHP());
    StatusWidget->UpdateStamina(StatComponent->GetCurrentStamina(), StatComponent->GetMaxStamina());
    StatusWidget->UpdateLevel(StatComponent->GetLevel());
    StatusWidget->UpdateExpBar(StatComponent->GetCurrentExp(), StatComponent->GetNextExp());

    float RemainingCooldown = FMath::Max(0.0f, CombatComponent->SkillCooldownTime - (GetWorld()->GetTimeSeconds() - CombatComponent->LastSkillTime));
    StatusWidget->UpdateSkillCooldown(RemainingCooldown, CombatComponent->SkillCooldownTime);
}

void ARogueCharacter::MoveForward(float Value)
{
    if (CombatComponent->GetAttackType()  == EAttackType::Skill || Controller == nullptr || Value == 0.0f)
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
    if (CombatComponent->GetAttackType() == EAttackType::Skill || Controller == nullptr || Value == 0.0f)
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
    CombatComponent->SetAttackType(EAttackType::None);
}
    
void ARogueCharacter::Jump()
{
    if (ActionState == ECharacterActionState::Jumping || ActionState == ECharacterActionState::Dodging || ActionState == ECharacterActionState::Attacking || ActionState == ECharacterActionState::Dead)
    {
        return;
    }

    if (!StatComponent->ConsumeStamina(JumpStaminaCost))
    {
        return;
    }

    ActionState = ECharacterActionState::Jumping;
    CombatComponent->SetAttackType(EAttackType::None);

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

void ARogueCharacter::UpdateTargets()
{
    Targets.Empty();
    FVector Origin = GetActorLocation();
    TArray<FOverlapResult> Overlaps;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(SearchRadius);

    GetWorld()->OverlapMultiByObjectType(
        Overlaps,
        Origin,
        FQuat::Identity,
        FCollisionObjectQueryParams(ECC_Pawn),
        Sphere
    );

    for (const FOverlapResult& Result : Overlaps)
    {
        AActor* Actor = Result.GetActor();
        if (Actor && Actor->IsA(AEnemyBase::StaticClass()))
        {
            AEnemyBase* E = Cast<AEnemyBase>(Actor);
            if (!E->Anim->GetIsDead())
            {
                FVector Dir = (Actor->GetActorLocation() - Origin).GetSafeNormal();
                float AngleDeg = FMath::RadiansToDegrees(acosf(FVector::DotProduct(GetControlRotation().Vector(), Dir)));
                if (AngleDeg <= ViewAngle)
                {
                    Targets.Add(Actor);
                }
            }
        }
    }
}

void ARogueCharacter::AcquireNearestTarget()
{
    UpdateTargets();

    float BestDist = FLT_MAX;
    AActor* Best = nullptr;
    FVector MyLoc = GetActorLocation();

    for (AActor* T : Targets)
    {
        float DistSqr = FVector::DistSquared(MyLoc, T->GetActorLocation());
        if (DistSqr < BestDist)
        {
            BestDist = DistSqr;
            Best = T;
        }
    }

    CurrentTarget = Best;
}

void ARogueCharacter::SwitchTarget(float Direction)
{
    if (!CurrentTarget) return;

    UpdateTargets();

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FVector2D CurrentScreen;
    PC->ProjectWorldLocationToScreen(CurrentTarget->GetActorLocation(), CurrentScreen);
    AActor* NewTarget = nullptr;
    float MinDelta = FLT_MAX;

    for (AActor* T : Targets)
    {
        if (T == CurrentTarget) continue;
        FVector2D ScreenPos;
        PC->ProjectWorldLocationToScreen(T->GetActorLocation(), ScreenPos);
        float DeltaX = ScreenPos.X - CurrentScreen.X;
        if (FMath::Sign(DeltaX) == FMath::Sign(Direction) && FMath::Abs(DeltaX) < MinDelta)
        {
            MinDelta = FMath::Abs(DeltaX);
            NewTarget = T;
        }
    }

    if (NewTarget)
    {
        CurrentTarget = NewTarget;
    }
}

void ARogueCharacter::ToggleLockOn()
{
    if (CurrentTarget)
    {
        CurrentTarget = nullptr;
        GetCharacterMovement()->bOrientRotationToMovement = true;
    }
    else
    {
        AcquireNearestTarget();
        GetCharacterMovement()->bOrientRotationToMovement = false;
    }
}

void ARogueCharacter::OnTurn(float Value)
{
    if (CurrentTarget)
    {
        if (FMath::Abs(Value) > TurnSwitchThreshold)
        {
            SwitchTarget(FMath::Sign(Value));
        }
    }
    else
    {
        AddControllerYawInput(Value);
    }
}

void ARogueCharacter::OnLookUp(float Value)
{
    if (CurrentTarget)
    {
        if (FMath::Abs(Value) > TurnSwitchThreshold)
        {
            SwitchTarget(FMath::Sign(Value));
        }
    }
    else
    {
        AddControllerPitchInput(Value);
    }
}