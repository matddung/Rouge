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

// Sets default values
ARogueCharacter::ARogueCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
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

    MaxCombo = 4;
    AttackEndComboState();

    GetCapsuleComponent()->SetCollisionProfileName(TEXT("RogueCharacter"));
    AttackRange = 150;
    AttackRadius = 50;

    CharacterStat = CreateDefaultSubobject<URogueCharacterStatComponent>(TEXT("CharacterStat"));
}

// Called when the game starts or when spawned
void ARogueCharacter::BeginPlay()
{
	Super::BeginPlay();
	
    if (StatusWidgetClass)
    {
        StatusWidget = CreateWidget<URogueUserWidget>(GetWorld(), StatusWidgetClass);
        if (StatusWidget)
        {
            StatusWidget->AddToViewport();
        }
    }
}

// Called every frame
void ARogueCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    float CurrentSpeed = GetCharacterMovement()->MaxWalkSpeed;
    float NewSpeed = FMath::FInterpTo(CurrentSpeed, TargetSpeed, DeltaTime, SpeedInterpRate);
    GetCharacterMovement()->MaxWalkSpeed = NewSpeed;

    if (CharacterStat && !bWantsToSprint && !IsAttacking && !bIsJumpAttacking && !bIsDodging)
    {
        CharacterStat->RecoverStamina(DeltaTime);
    }

    if (bWantsToSprint && !CharacterStat->ConsumeStamina(SprintStaminaCostPerSec * DeltaTime))
    {
        StopSprinting();
    }

    if (StatusWidget && CharacterStat)
    {
        StatusWidget->UpdateHP(CharacterStat->GetCurrentHP(), CharacterStat->GetMaxHP());
        StatusWidget->UpdateStamina(CharacterStat->GetCurrentStamina(), CharacterStat->GetMaxStamina());
        StatusWidget->UpdateLevel(CharacterStat->GetLevel());
        StatusWidget->UpdateExpBar(CharacterStat->GetCurrentExp(), CharacterStat->GetNextExp());
        float CurrentTime = GetWorld()->GetTimeSeconds();
        float RemainingCooldown = FMath::Max(0.0f, SkillCooldownTime - (CurrentTime - LastSkillTime));
        StatusWidget->UpdateSkillCooldown(RemainingCooldown, SkillCooldownTime);
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

    RogueAnim->OnMontageEnded.AddDynamic(this, &ARogueCharacter::OnAttackMontageEnded);

    RogueAnim->OnNextAttackCheck.AddLambda([this]() -> void {
        UE_LOG(LogTemp, Warning, TEXT("OnNextAttackCheck"));
        CanNextCombo = false;

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

    RogueAnim->OnAttackHitCheck.AddUObject(this, &ARogueCharacter::AttackCheck);

    RogueAnim->OnDashAttackHitCheck.AddLambda([this]() -> void {
        DoDashAttackHit();
    });

    RogueAnim->OnJumpAttackHitCheck.AddLambda([this]() {
        DoJumpAttackHit();
    });

    RogueAnim->OnSkillHitCheck.AddLambda([this]() {
        DoSkillHit();
    });

    RogueAnim->OnDodgeEffectStart.AddLambda([this]() {
        HandleDodgeEffectStart();
    });

    RogueAnim->OnDodgeEffectEnd.AddLambda([this]() {
        HandleDodgeEffectEnd();
    });

    CharacterStat->OnHPIsZero.AddLambda([this]() -> void {
        UE_LOG(LogTemp, Warning, TEXT("OnHPIsZero"));
        SetActorEnableCollision(false);
        RogueAnim->SetDeadAnim();
     });
}

float ARogueCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsSkillInvincible || bIsDodgeInvincible)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invincible No damage taken"));
        return 0;
    }

    float FinalDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    UE_LOG(LogTemp, Warning, TEXT("Actor : %s took Damage : %f"), *GetName(), FinalDamage);

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

// Called to bind functionality to input
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

void ARogueCharacter::MoveForward(float Value)
{
    if (bIsSkillInvincible || Controller == nullptr || Value == 0.0f)
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
    if (bIsSkillInvincible || Controller == nullptr || Value == 0.0f)
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
    bIsSprintKeyDown = true;
    bWantsToSprint = true;
    TargetSpeed = RunSpeed;
}

void ARogueCharacter::StopSprinting()
{
    bIsSprintKeyDown = false;
    bWantsToSprint = false;
    TargetSpeed = WalkSpeed;
}

void ARogueCharacter::Attack()
{
    if (bIsJumpAttacking || bIsDodging || IsHidden())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot attack during dodge or while hidden"));
        return;
    }

    if (IsAttacking)
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
    IsAttacking = true;
}

void ARogueCharacter::DashAttack()
{
    if (!CharacterStat->ConsumeStamina(AttackCost))
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough stamina for dash attack"));
        return;
    }

    TargetSpeed = WalkSpeed;
    IsAttacking = true;

    FVector Forward = GetActorForwardVector();

    if (DashAttackMontage && RogueAnim)
    {
        RogueAnim->Montage_Play(DashAttackMontage);
        LaunchCharacter(Forward * 1000, true, true);
    }
}

void ARogueCharacter::DoDashAttackHit()
{
    TArray<FOverlapResult> Overlaps;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(150);
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    bool bHit = GetWorld()->OverlapMultiByChannel(
        Overlaps,
        GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,
        Sphere,
        QueryParams
    );

    if (bHit)
    {
        for (auto& Result : Overlaps)
        {
            AActor* HitActor = Result.GetActor();
            if (HitActor && HitActor != this)
            {
                float DashDamage = CharacterStat->GetAttack() * 1.5;
                FDamageEvent DamageEvent;
                HitActor->TakeDamage(DashDamage, DamageEvent, GetController(), this);
            }
        }
    }

#if WITH_EDITOR
    DrawDebugSphere(GetWorld(), GetActorLocation(), 150, 16, FColor::Red, false, 1);
#endif
}

void ARogueCharacter::JumpAttack()
{
    if (bIsJumpAttacking || IsAttacking || !JumpAttackMontage || !RogueAnim)
    {
        return;
    }

    if (!CharacterStat->ConsumeStamina(AttackCost))
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough stamina to jump attack"));
        return;
    }

    bIsJumpAttacking = true;
    IsAttacking = true;
    RogueAnim->Montage_Play(JumpAttackMontage);
}

void ARogueCharacter::DoJumpAttackHit()
{
    FVector Start = GetActorLocation() + FVector(0, 0, 50);
    FVector Forward = GetActorForwardVector();
    FVector Down = FVector(0, 0, -1);
    FVector Dir = (Forward + Down).GetSafeNormal();
    FVector End = Start + Dir * 200;

    float CapsuleHalfHeight = 100;
    float CapsuleRadius = 60;

    FCollisionShape Shape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    FHitResult HitResult;
    bool bHit = GetWorld()->SweepSingleByChannel(
        HitResult,
        Start,
        End,
        FQuat::FindBetweenVectors(FVector::ForwardVector, Dir),
        ECC_Pawn,
        Shape,
        Params
    );

#if ENABLE_DRAW_DEBUG
    FColor DrawColor = bHit ? FColor::Green : FColor::Red;
    DrawDebugCapsule(GetWorld(),
        (Start + End) * 0.5,
        CapsuleHalfHeight,
        CapsuleRadius,
        FQuat::FindBetweenVectors(FVector::UpVector, Dir),
        DrawColor,
        false,
        1);
#endif

    if (bHit && HitResult.GetActor())
    {
        float Damage = CharacterStat->GetAttack() * 1.25;
        FDamageEvent DamageEvent;
        HitResult.GetActor()->TakeDamage(Damage, DamageEvent, GetController(), this);
    }
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

    if (IsAttacking || bIsJumpAttacking || bIsSkillInvincible)
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

    IsAttacking = true;
    bIsSkillInvincible = true;

    RogueAnim->Montage_Play(SkillMontage);
}

void ARogueCharacter::DoSkillHit()
{
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

    TArray<FOverlapResult> Overlaps;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(550);
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    bool bHit = GetWorld()->OverlapMultiByChannel(
        Overlaps,
        GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,
        Sphere,
        Params
    );

    for (auto& Result : Overlaps)
    {
        AActor* HitActor = Result.GetActor();
        if (HitActor && HitActor != this)
        {
            float Damage = CharacterStat->GetAttack() * 5;
            FDamageEvent DamageEvent;
            HitActor->TakeDamage(Damage, DamageEvent, GetController(), this);
        }
    }

#if WITH_EDITOR
    DrawDebugSphere(GetWorld(), GetActorLocation(), 550, 24, FColor::Red, false, 1);
#endif
}

void ARogueCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);

    bIsJumpAttacking = false;
}

void ARogueCharacter::Jump()
{
    if (!CharacterStat->ConsumeStamina(JumpStaminaCost))
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough stamina to jump"));
        return;
    }

    if (IsAttacking || bIsJumpAttacking || bIsDodging)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot jump while attacking"));
        return;
    }

    Super::Jump();
}

void ARogueCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage == DodgeMontage)
    {
        bIsDodging = false;
        bIsDodgeInvincible = false;
        return;
    }

    if (Montage == DashAttackMontage || Montage == JumpAttackMontage || Montage == SkillMontage)
    {
        IsAttacking = false;
        bIsSkillInvincible = false;

        if (SkillEffectComponent)
        {
            SkillEffectComponent->DeactivateSystem();
            SkillEffectComponent = nullptr;
        }

        if (bIsSprintKeyDown)
        {
            bWantsToSprint = true;
            TargetSpeed = RunSpeed;
        }

        return;
    }

    if (!ensureMsgf(IsAttacking, TEXT("OnAttackMontageEnded IsAttacking is false")))
    {
        return;
    }

    if (!ensureMsgf(CurrentCombo > 0, TEXT("OnAttackMontageEnded CurrentCombo more then 0")))
    {
        return;
    }

    IsAttacking = false;
    AttackEndComboState();

    if (bIsSprintKeyDown)
    {
        bWantsToSprint = true;
        TargetSpeed = RunSpeed;
    }
}

void ARogueCharacter::AttackStartComboState()
{
    CanNextCombo = true;
    IsComboInputOn = false;
    if (!ensureMsgf(FMath::IsWithinInclusive<int32>(CurrentCombo, 0, MaxCombo - 1), TEXT("AttackStartComboState error")))
    {
        return;
    }
    CurrentCombo = FMath::Clamp<int32>(CurrentCombo + 1, 1, MaxCombo);
}

void ARogueCharacter::AttackEndComboState()
{
    IsComboInputOn = false;
    CanNextCombo = false;
    CurrentCombo = 0;
}

void ARogueCharacter::AttackCheck()
{
    FHitResult HitResult;
    FCollisionQueryParams Params(NAME_None, false, this);
    bool bResult = GetWorld()->SweepSingleByChannel(
        HitResult,
        GetActorLocation(),
        GetActorLocation() + GetActorForwardVector() * AttackRange,
        FQuat::Identity,
        ECollisionChannel::ECC_GameTraceChannel2,
        FCollisionShape::MakeSphere(AttackRadius),
        Params);

#if ENABLE_DRAW_DEBUG

    FVector TraceVec = GetActorForwardVector() * AttackRange;
    FVector Center = GetActorLocation() + TraceVec * 0.5;
    float HalfHeight = AttackRange * 0.5 + AttackRadius;
    FQuat CapsuleRot = FRotationMatrix::MakeFromZ(TraceVec).ToQuat();
    FColor DrawColor = bResult ? FColor::Green : FColor::Red;
    float DebugLifeTime = 5;

    DrawDebugCapsule(GetWorld(),
        Center,
        HalfHeight,
        AttackRadius,
        CapsuleRot,
        DrawColor,
        false,
        DebugLifeTime);

#endif

    if (bResult)
    {
        if (HitResult.Actor.IsValid())
        {
            FDamageEvent DamageEvent;
            HitResult.Actor->TakeDamage(CharacterStat->GetAttack(), DamageEvent, GetController(), this);

            SpawnDamageText(HitResult.GetActor(), CharacterStat->GetAttack());
        }
    }
}

void ARogueCharacter::Dodge()
{
    if (IsAttacking || bIsDodging || bIsJumpAttacking || bIsSkillInvincible || RogueAnim->IsInAir) return;

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

    bIsDodging = true;
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

void ARogueCharacter::SpawnDamageText(AActor* DamagedActor, float Damage)
{
    if (!DamageTextActorClass || !DamagedActor) return;

    UE_LOG(LogTemp, Warning, TEXT("SpawnDamageText: Target=%s Damage=%.1f"), *GetName(), Damage);

    FVector TargetLocation = DamagedActor->GetActorLocation() + FVector(0.f, 0.f, 100.f);
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AFloatingDamageActor* DamageText = GetWorld()->SpawnActor<AFloatingDamageActor>(DamageTextActorClass, TargetLocation, FRotator::ZeroRotator, Params);
    if (DamageText)
    {
        DamageText->SetDamage(Damage);
    }
}