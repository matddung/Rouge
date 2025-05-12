// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueCharacterDodgeComponent.h"
#include "RogueCharacter.h"
#include "RogueCharacterStatComponent.h"
#include "RogueAnimInstance.h"
#include "RogueCharacterCombatComponent.h"

#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
URogueCharacterDodgeComponent::URogueCharacterDodgeComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void URogueCharacterDodgeComponent::BeginPlay()
{
	Super::BeginPlay();

    Character = Cast<ARogueCharacter>(GetOwner());
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("DodgeComponent: Owner is not ARogueCharacter"));
        return;
    }

    CombatComponent = Character->FindComponentByClass<URogueCharacterCombatComponent>();
    if (!CombatComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("DodgeComponent: CombatComponent not found!"));
        return;
    }

    CharacterStat = Character->FindComponentByClass<URogueCharacterStatComponent>();
    if (!CharacterStat)
    {
        UE_LOG(LogTemp, Error, TEXT("DodgeComponent: CharacterStat not found!"));
        return;
    }
}

void URogueCharacterDodgeComponent::Dodge()
{
    if (Character->GetActionState() != ECharacterActionState::Idle &&
        Character->GetActionState() != ECharacterActionState::Walking &&
        Character->GetActionState() != ECharacterActionState::Running)
    {
        return;
    }

    if (!CharacterStat->ConsumeStamina(Character->DodgeStaminaCost))
    {
        return;
    }

    FVector InputDir = Character->GetLastMovementInputVector().GetSafeNormal();
    if (InputDir.IsNearlyZero())
    {
        InputDir = Character->GetActorForwardVector();
    }

    DodgeDirection = InputDir;

    Character->SetActionState(ECharacterActionState::Dodging);
    bDidDodgeTeleport = false;

    if (DodgeMontage && RogueAnim)
    {
        RogueAnim->Montage_Play(DodgeMontage);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("DodgeComponent: Missing DodgeMontage or RogueAnim"));
    }
}

void URogueCharacterDodgeComponent::HandleDodgeEffectStart()
{
    bIsDodgeInvincible = true;
    Character->SetActorHiddenInGame(true);
    if (DodgeEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DodgeEffect, Character->GetActorLocation(), Character->GetActorRotation());
    }

    if (!bDidDodgeTeleport)
    {
        FVector NewLocation = Character->GetActorLocation() + DodgeDirection * DodgeDistance;
        FHitResult HitResult;
        bool bMoved = Character->SetActorLocation(NewLocation, true, &HitResult, ETeleportType::TeleportPhysics);

        bDidDodgeTeleport = true;
    }
}

void URogueCharacterDodgeComponent::HandleDodgeEffectEnd()
{
    bIsDodgeInvincible = false;
    Character->SetActorHiddenInGame(false);
}

void URogueCharacterDodgeComponent::InitializeWithAnim(URogueAnimInstance* AnimInstance)
{
    if (!AnimInstance) return;

    RogueAnim = AnimInstance;

    RogueAnim->OnDodgeEffectStart.AddLambda([this]() {
        HandleDodgeEffectStart();
        });

    RogueAnim->OnDodgeEffectEnd.AddLambda([this]() {
        HandleDodgeEffectEnd();
        });
}