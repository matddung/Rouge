// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RogueGameInstance.h"
#include "RogueCharacterStatComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnHPIsZeroDelegate);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnExpChangedDelegate, int32 /*CurrentExp*/, int32 /*NextExp*/);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ROGUE_API URogueCharacterStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	URogueCharacterStatComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;

public:	
	void SetNewLevel(int32 NewLevel);
	void SetDamage(float NewDamage);
	float GetAttack();

	bool ConsumeStamina(float Amount);
	void RecoverStamina(float DeltaTime);

	void AddExp(int32 ExpAmount);

	FOnHPIsZeroDelegate OnHPIsZero;
	FOnExpChangedDelegate OnExpChanged;

	int32 GetCurrentExp() const { return CurrentExp; }
	int32 GetNextExp() const { return CurrentStatData ? CurrentStatData->NextExp : 0; }

	float GetCurrentHP() const { return CurrentHP; }
	float GetMaxHP() const { return CurrentStatData ? CurrentStatData->MaxHP : 1; }

	float GetCurrentStamina() const { return CurrentStamina; }
	float GetMaxStamina() const { return CurrentStatData ? CurrentStatData->Stamina : 1; }

	int32 GetLevel() const { return Level; }

private:
	struct FRogueCharacterData* CurrentStatData = nullptr;
	
	UPROPERTY(EditInstanceOnly, Category = "Stat", Meta = (AllowPrivateAccess = true))
	int32 Level;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Stat", Meta = (AllowPrivateAccess = true))
	float CurrentHP;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Stat", Meta = (AllowPrivateAccess = true))
	float CurrentStamina;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Stat", Meta = (AllowPrivateAccess = true))
	float StaminaRecoveryRate = 10;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Stat", Meta = (AllowPrivateAccess = true))
	int32 CurrentExp = 0;
};
