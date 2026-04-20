// Copyright ProjectR. All Rights Reserved.

#include "PRPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Player.h"

APRPlayerState::APRPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UPRAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// 플레이어 ASC는 Mixed 모드: 본인 GE 전체 + 타 플레이어 요약
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	CommonSet = CreateDefaultSubobject<UPRAttributeSet_Common>(TEXT("CommonSet"));
	PlayerSet = CreateDefaultSubobject<UPRAttributeSet_Player>(TEXT("PlayerSet"));

	// PlayerState는 NetUpdate가 낮음. GAS 예측 응답성 확보를 위해 인상
	SetNetUpdateFrequency(100.0f);
}

// =====  복제 등록 =====

void APRPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRPlayerState, DisplayName);
	DOREPLIFETIME(APRPlayerState, CharacterLevel);
	DOREPLIFETIME(APRPlayerState, Experience);
}

void APRPlayerState::BeginPlay()
{
	Super::BeginPlay();
}

// =====  IAbilitySystemInterface =====

UAbilitySystemComponent* APRPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// =====  초기화 =====

void APRPlayerState::InitializeFromSaveData(const FPRCharacterSaveData& SaveData)
{
	if (!HasAuthority())
	{
		return;
	}

	DisplayName    = SaveData.DisplayName;
	CharacterLevel = SaveData.Level;
	Experience     = SaveData.Experience;
	StatUpgradeInfo          = SaveData.Stats;
}
