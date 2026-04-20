// Copyright ProjectR. All Rights Reserved.

#include "PRCharacterBase.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"

APRCharacterBase::APRCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

// =====  AActor Interface =====

void APRCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void APRCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// =====  ACharacter Interface =====

void APRCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

// =====  IAbilitySystemInterface =====

UAbilitySystemComponent* APRCharacterBase::GetAbilitySystemComponent() const
{
	return GetPRAbilitySystemComponent();
}

UPRAbilitySystemComponent* APRCharacterBase::GetPRAbilitySystemComponent() const
{
	// 기본 구현은 nullptr. 플레이어는 PlayerState, 적은 자기 컴포넌트에서 override
	return nullptr;
}
