// Fill out your copyright notice in the Description page of Project Settings.


#include "RoguePlayerController.h"

void ARoguePlayerController::PostInitializeComponents() {
	Super::PostInitializeComponents();

}

void ARoguePlayerController::OnPossess(APawn* aPawn) {
	Super::OnPossess(aPawn);

}

void ARoguePlayerController::BeginPlay() {
	Super::BeginPlay();

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}