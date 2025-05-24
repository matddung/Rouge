#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NormalEnemyAnimInstance.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnAttackHitCheckDelegate);
DECLARE_MULTICAST_DELEGATE(FOnAttackEndDelegate);

UCLASS()
class ROGUE_API UNormalEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	FOnAttackHitCheckDelegate OnAttackHitCheck;
	FOnAttackEndDelegate OnAttackEnd;

	void SetDeadAnim() { bIsDead = true; }
	bool GetIsDead() { return bIsDead; }
		
	UFUNCTION(BlueprintCallable)
	void PlayRandomAttack();
	
protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;

private:
	void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void AnimNotify_AttackHitCheck();

	UFUNCTION()
	void AnimNotify_AttackEnd();

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Attack, Meta = (AllowPrivateAccess = true))
	class UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pawn, Meta = (AllowPrivateAccess = true))
	bool bIsDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pawn, Meta = (AllowPrivateAccess = true))
	float Speed;

};