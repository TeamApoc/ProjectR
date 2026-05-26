// Copyright ProjectR. All Rights Reserved.

#include "PRCharacterBase.h"

#include "ProjectR/PRGameplayTags.h"
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

void APRCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	// 바인딩한 태그 변경 이벤트 제거
	if (UPRAbilitySystemComponent* ASC = GetPRAbilitySystemComponent())
	{
		ASC->OnGameplayTagUpdated.RemoveAll(this);
	}
}

void APRCharacterBase::BindTagChangeEvent()
{
	UPRAbilitySystemComponent* ASC = GetPRAbilitySystemComponent();
	if (!ensureMsgf(ASC, TEXT("BindTagChangeEvent 타이밍 에러: ASC가 아직 존재하지 않음")))
	{
		return;
	}
	
	ASC->OnGameplayTagUpdated.AddUObject(this, &ThisClass::HandleGameplayTagUpdated);
	
	// 이미 가지고 있던 태그가 있을 수 있으므로 명시적 호출
	FGameplayTagContainer OwnedGameplayTags;
	ASC->GetOwnedGameplayTags(OwnedGameplayTags);
	
	for (const FGameplayTag& Tag : OwnedGameplayTags)
	{
		HandleGameplayTagUpdated(Tag, true);
	}
}

void APRCharacterBase::HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool bTagExists)
{
	
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

bool APRCharacterBase::IsDead() const
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		return ASC->HasMatchingGameplayTag(PRGameplayTags::State_Dead);
	}
	return false;
}

UPRAbilitySystemComponent* APRCharacterBase::GetPRAbilitySystemComponent() const
{
	// 기본 구현은 nullptr. 플레이어는 PlayerState, 적은 자기 컴포넌트에서 override
	return nullptr;
}
