// Copyright ProjectR. All Rights Reserved.

#include "PRPlayerState.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Player.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/Equipment/Components/PREquipmentManagerComponent.h"
#include "ProjectR/Inventory/Components/PRInventoryComponent.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/QuickSlot/Coponents/PRQuickSlotComponent.h"

APRPlayerState::APRPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UPRAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// 플레이어 ASC는 Mixed 모드: 본인 GE 전체 + 타 플레이어 요약
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	CommonSet = CreateDefaultSubobject<UPRAttributeSet_Common>(TEXT("CommonSet"));
	PlayerSet = CreateDefaultSubobject<UPRAttributeSet_Player>(TEXT("PlayerSet"));
	WeaponSet = CreateDefaultSubobject<UPRAttributeSet_Weapon>(TEXT("WeaponSet"));
	InventoryComponent = CreateDefaultSubobject<UPRInventoryComponent>(TEXT("InventoryComponent"));
	EquipmentManagerComponent = CreateDefaultSubobject<UPREquipmentManagerComponent>(TEXT("EquipmentManagerComponent"));
	QuickSlotComponent = CreateDefaultSubobject<UPRQuickSlotComponent>(TEXT("QuickSlotComponent"));

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
	DOREPLIFETIME(APRPlayerState, StatUpgradeInfo);
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

// =====  생존 상태 =====

bool APRPlayerState::IsCombatParticipant() const
{
	return !IsOnlyASpectator() && IsValid(AbilitySystemComponent);
}

bool APRPlayerState::IsDown() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return IsValid(ASC) && ASC->HasMatchingGameplayTag(PRGameplayTags::State_Down);
}

bool APRPlayerState::IsDead() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return IsValid(ASC) && ASC->HasMatchingGameplayTag(PRGameplayTags::State_Dead);
}

bool APRPlayerState::IsOutOfFight() const
{
	return IsDown() || IsDead();
}

bool APRPlayerState::HasFightCapableAllyExceptSelf() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* CurrentGameState = IsValid(World) ? World->GetGameState<AGameStateBase>() : nullptr;
	if (!IsValid(CurrentGameState))
	{
		return false;
	}

	for (APlayerState* PlayerState : CurrentGameState->PlayerArray)
	{
		const APRPlayerState* OtherPlayerState = Cast<APRPlayerState>(PlayerState);
		if (!IsValid(OtherPlayerState) || OtherPlayerState == this || !OtherPlayerState->IsCombatParticipant())
		{
			continue;
		}

		if (!OtherPlayerState->IsOutOfFight())
		{
			return true;
		}
	}

	return false;
}

void APRPlayerState::SendSurvivalGameplayEvent(const FGameplayTag& EventTag) const
{
	if (!HasAuthority() || !EventTag.IsValid())
	{
		return;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	AActor* AvatarActor = IsValid(ASC) ? ASC->GetAvatarActor() : nullptr;
	if (!IsValid(AvatarActor))
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Target = AvatarActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		AvatarActor,
		EventTag,
		Payload);

	if (EventTag.MatchesTagExact(PRGameplayTags::Event_Ability_PlayerDeathConfirmed))
	{
		APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwner());
		if (IsValid(PlayerController))
		{
			PlayerController->ClientDispatchSurvivalGameplayEvent(EventTag);
		}
	}
}
