#include "RogueCharacter.h"
#include "RogueAnimInstance.h"
#include "RogueCharacterStatComponent.h"
#include "RogueUserWidget.h"
#include "FloatingDamageActor.h"
#include "RogueCharacterCombatComponent.h"
#include "RogueCharacterDodgeComponent.h"

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

    CharacterStat = CreateDefaultSubobject<URogueCharacterStatComponent>(TEXT("CharacterStat"));
    CombatComponent = CreateDefaultSubobject<URogueCharacterCombatComponent>(TEXT("CombatComponent"));
    DodgeComponent = CreateDefaultSubobject<URogueCharacterDodgeComponent>(TEXT("DodgeComponent"));

    CombatComponent->AttackEndComboState();
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
    UpdateMovementState();
    UpdateStatusWidget();

    UE_LOG(LogTemp, Warning, TEXT("ActionState: %s"), *UEnum::GetValueAsString(ActionState));
}

void ARogueCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    RogueAnim = Cast<URogueAnimInstance>(GetMesh()->GetAnimInstance());
    if (!ensureMsgf(RogueAnim != nullptr, TEXT("PostInitializeComponents AnimInstance is nullptr")))
    {
        return;
    }

    CombatComponent->InitializeWithAnim(RogueAnim);
    DodgeComponent->InitializeWithAnim(RogueAnim);

    CharacterStat->OnHPIsZero.AddLambda([this]() -> void {
        SetActorEnableCollision(false);
        RogueAnim->SetDeadAnim();
     });
}

float ARogueCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
    if (CombatComponent->GetAttackType() == EAttackType::Skill || DodgeComponent->bIsDodgeInvincible)
    {
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
    PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &ARogueCharacter::InputAttack);
    PlayerInputComponent->BindAction("UseSkill", IE_Pressed, this, &ARogueCharacter::InputUseSkill);
    PlayerInputComponent->BindAction("ZoomInCamera", IE_Pressed, this, &ARogueCharacter::ZoomInCamera);
    PlayerInputComponent->BindAction("ZoomOutCamera", IE_Pressed, this, &ARogueCharacter::ZoomOutCamera);
    PlayerInputComponent->BindAction("Dodge", IE_Pressed, this, &ARogueCharacter::InputDodge);

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
    const float MovementSpeed = GetVelocity().Size();

    if ((ActionState == ECharacterActionState::Idle || ActionState == ECharacterActionState::Walking) && !GetCharacterMovement()->IsFalling())
    {
        CharacterStat->RecoverStamina(DeltaTime);
    }

    const float SprintSpeedThreshold = RunSpeed * 0.7f;

    if (bWantsToSprint && ActionState == ECharacterActionState::Running)
    {
        if (!CharacterStat->ConsumeStamina(SprintStaminaCostPerSec * DeltaTime))
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

    StatusWidget->UpdateHP(CharacterStat->GetCurrentHP(), CharacterStat->GetMaxHP());
    StatusWidget->UpdateStamina(CharacterStat->GetCurrentStamina(), CharacterStat->GetMaxStamina());
    StatusWidget->UpdateLevel(CharacterStat->GetLevel());
    StatusWidget->UpdateExpBar(CharacterStat->GetCurrentExp(), CharacterStat->GetNextExp());

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

    if (!CharacterStat->ConsumeStamina(JumpStaminaCost))
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

void ARogueCharacter::InputAttack()
{
    if (CombatComponent)
    {
        CombatComponent->Attack();
    }
}

void ARogueCharacter::InputUseSkill()
{
    if (CombatComponent)
    {
        CombatComponent->UseSkill();
    }
}

void ARogueCharacter::InputDodge()
{
    if (DodgeComponent)
    {
        DodgeComponent->Dodge();
    }
}