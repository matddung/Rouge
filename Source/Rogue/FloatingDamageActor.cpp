#include "FloatingDamageActor.h"

#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"

AFloatingDamageActor::AFloatingDamageActor()
{
    PrimaryActorTick.bCanEverTick = false;

    WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
    WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
    WidgetComponent->SetDrawSize(FVector2D(150, 50));
    RootComponent = WidgetComponent;

    static ConstructorHelpers::FClassFinder<UUserWidget> DamageTextWidgetClass(TEXT("/Game/UI/WBP_DamageText.WBP_DamageText_C"));
    if (DamageTextWidgetClass.Succeeded())
    {
        WidgetComponent->SetWidgetClass(DamageTextWidgetClass.Class);
    }
}

void AFloatingDamageActor::BeginPlay()
{
    Super::BeginPlay();
    SetLifeSpan(LifeTime);
}

void AFloatingDamageActor::SetDamage(float Damage)
{
    if (UUserWidget* Widget = WidgetComponent->GetWidget())
    {
        if (UTextBlock* Text = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("DamageText"))))
        {
            Text->SetText(FText::AsNumber(FMath::RoundToInt(Damage)));
        }
    }
}