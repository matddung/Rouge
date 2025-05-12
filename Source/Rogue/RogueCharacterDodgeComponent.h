// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RogueCharacterDodgeComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ROGUE_API URogueCharacterDodgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	URogueCharacterDodgeComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	void Dodge();
	void HandleDodgeEffectStart();
	void HandleDodgeEffectEnd();

	void InitializeWithAnim(class URogueAnimInstance* AnimInstance);

public:
	UPROPERTY(EditAnywhere, Category = "Dodge")
	UAnimMontage* DodgeMontage;

	bool bIsDodgeInvincible = false;

private:
	UPROPERTY()
	class ARogueCharacter* Character;

	UPROPERTY()
	class URogueCharacterCombatComponent* CombatComponent;

	UPROPERTY(VisibleAnywhere, Category = "Stat")
	class URogueCharacterStatComponent* CharacterStat;

	UPROPERTY()
	class URogueAnimInstance* RogueAnim;

	FVector DodgeDirection;

	UPROPERTY(EditAnywhere)
	float DodgeDistance = 300;

	FTimerHandle DodgeTimerHandle;

	UPROPERTY(EditAnywhere, Category = "Dodge")
	UParticleSystem* DodgeEffect;

	bool bDidDodgeTeleport = false;
};
