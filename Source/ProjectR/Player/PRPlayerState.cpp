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
#include "ProjectR/Inventory/Components/PRQuickSlotComponent.h"
#include "ProjectR/Inventory/Data/PRItemDataAsset.h"
#include "ProjectR/Inventory/Items/PRItemInstance_Consumable.h"

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
	
	// 초기 아이템 지급
	if (HasAuthority())
	{
		for (FPRItemSaveEntry& StartUpItem : StartUpItems)
		{
			if (IsValid(StartUpItem.ItemData) && StartUpItem.Amount > 0)
			{
				InventoryComponent->AddItem(StartUpItem.ItemData, StartUpItem.Amount);
			}
		}
		
		BindAutoRegisterQuickSlotEvent();
	}
	QuickSlotComponent->InitializeQuickSlots(InventoryComponent);
}

void APRPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
	
	if (APRPlayerState* NewPS = Cast<APRPlayerState>(PlayerState))
	{
		// TODO: ASC 상태 보존, 인벤토리 등 상태 컴포넌트 값 보존
		FPRCharacterSaveData SaveData = MakeSaveData();
		NewPS->InitializePrimaryInfoFromSaveData(SaveData);
	}
}

// =====  IAbilitySystemInterface =====

UAbilitySystemComponent* APRPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// =====  초기화 =====

void APRPlayerState::InitializePrimaryInfoFromSaveData(const FPRCharacterSaveData& InSaveData)
{
	if (!HasAuthority())
	{
		return;
	}

	CurrentSaveData = InSaveData;
	DisplayName    = InSaveData.DisplayName;
}

void APRPlayerState::ApplySaveData(const FPRCharacterSaveData& InSaveData)
{
	
}

FPRCharacterSaveData APRPlayerState::MakeSaveData() const
{
	return FPRCharacterSaveData();
}

void APRPlayerState::BindAutoRegisterQuickSlotEvent()
{
	InventoryComponent->GetOnInventoryChanged().RemoveDynamic(this,&ThisClass::OnInventoryChanged);
	InventoryComponent->GetOnInventoryChanged().AddDynamic(this,&ThisClass::OnInventoryChanged);
}

void APRPlayerState::OnInventoryChanged(UPRInventoryComponent* InInventory,
	const FPRInventoryChangeEventData& EventData)
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (EventData.ChangeReason != EPRInventoryChangeReason::ItemAdded)
	{
		return;
	}

	UPRItemInstance_Consumable* AsConsumable = Cast<UPRItemInstance_Consumable>(EventData.ItemInstance);
	if (!IsValid(AsConsumable))
	{
		return;
	}
	
	if (QuickSlotComponent->IsRegisteredItem(AsConsumable->GetConsumableData()))
	{
		return;
	}
	
	const int32 MaxCount = QuickSlotComponent->GetMaxQuickSlotCount();
	const int32 CurrentCount = QuickSlotComponent->GetUsingQuickSlotCount();

	if (CurrentCount < MaxCount)
	{
		QuickSlotComponent->RequestRegisterQuickSlotItem(CurrentCount,AsConsumable->GetConsumableData());
	}
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
	
	//
	// if (EventTag.MatchesTagExact(PRGameplayTags::Event_Ability_PlayerDeathConfirmed))
	// {
	// 	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwner());
	// 	if (IsValid(PlayerController))
	// 	{
	// 		PlayerController->ClientDispatchSurvivalGameplayEvent(EventTag);
	// 	}
	// }
}
