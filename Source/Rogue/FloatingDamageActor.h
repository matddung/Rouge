// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FloatingDamageActor.generated.h"

UCLASS()
class ROGUE_API AFloatingDamageActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFloatingDamageActor();

    void SetDamage(float Damage);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    UPROPERTY(VisibleAnywhere)
    class UWidgetComponent* WidgetComponent;

    float LifeTime = 1.2f;

    UPROPERTY(EditDefaultsOnly, Category = "Damage")
    float RiseSpeed = 50.0f;

private:
    float ElapsedTime = 0.0f;
};