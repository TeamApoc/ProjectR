// Copyright ProjectR. All Rights Reserved.

#include "PRPlayerState.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Growth.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Player.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/ItemSystem/Components/PREquipmentManagerComponent.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/FX/PRFXNetworkComponent.h"
#include "ProjectR/ItemSystem/Components/PRQuickSlotComponent.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Consumable.h"
#include "ProjectR/Player/Components/PRCurrencyComponent.h"
#include "ProjectR/Player/Components/PRPlayerGrowthComponent.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"

APRPlayerState::APRPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UPRAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	CommonSet = CreateDefaultSubobject<UPRAttributeSet_Common>(TEXT("CommonSet"));
	PlayerSet = CreateDefaultSubobject<UPRAttributeSet_Player>(TEXT("PlayerSet"));
	WeaponSet = CreateDefaultSubobject<UPRAttributeSet_Weapon>(TEXT("WeaponSet"));
	GrowthSet = CreateDefaultSubobject<UPRAttributeSet_Growth>(TEXT("GrowthSet"));
	InventoryComponent = CreateDefaultSubobject<UPRInventoryComponent>(TEXT("InventoryComponent"));
	EquipmentManagerComponent = CreateDefaultSubobject<UPREquipmentManagerComponent>(TEXT("EquipmentManagerComponent"));
	WeaponManagerComponent = CreateDefaultSubobject<UPRWeaponManagerComponent>(TEXT("WeaponManagerComponent"));
	QuickSlotComponent = CreateDefaultSubobject<UPRQuickSlotComponent>(TEXT("QuickSlotComponent"));
	CurrencyComponent = CreateDefaultSubobject<UPRCurrencyComponent>(TEXT("CurrencyComponent"));
	GrowthComponent = CreateDefaultSubobject<UPRPlayerGrowthComponent>(TEXT("GrowthComponent"));

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

void APRPlayerState::GiveStartUpItems()
{
	if (HasAuthority())
	{
		// 기본 지급 아이템 추가 이벤트 수신 준비
		BindAutoRegisterQuickSlotEvent();

		for (FPRItemSaveEntry& StartUpItem : StartUpItems)
		{
			if (!IsValid(StartUpItem.ItemData) || StartUpItem.Amount <= 0)
			{
				continue;
			}
			
			// 이미 존재하던 아이템에 AddItem이 될 수 있음.
			if (UPRItemInstance* Item = InventoryComponent->FindItemByData(StartUpItem.ItemData))
			{
				Item->SetStack(FMath::Max(StartUpItem.Amount, Item->GetStackCount()));
			}
			else
			{
				InventoryComponent->AddItem(StartUpItem.ItemData, StartUpItem.Amount);
			}
		}

		if (IsValid(QuickSlotComponent))
		{
			// 기본 지급 이후 소비 아이템 캐시 갱신
			QuickSlotComponent->ResetSystem();
		}
	}
}

void APRPlayerState::BeginPlay()
{
	Super::BeginPlay();
	
	// 초기 아이템 지급
	GiveStartUpItems();
	QuickSlotComponent->InitializeQuickSlots(InventoryComponent);
}

void APRPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
	
	if (APRPlayerState* NewPS = Cast<APRPlayerState>(PlayerState))
	{
		FPRCharacterSaveData SaveData = MakeSaveData();
		// SeamlessTravel 이후 새 Pawn 준비 시 전체 런타임 상태 복원 예약
		NewPS->QueueSaveDataApply(SaveData, false);
		if (bCharacterPayloadAccepted)
		{
			// 이동 전 수락된 세션 payload 상태 보존
			NewPS->MarkCharacterPayloadAccepted();
		}
	}
}

// =====  IAbilitySystemInterface =====

UAbilitySystemComponent* APRPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APRPlayerState::GetCachedAmmoRatios(EPRWeaponSlotType SlotType, float& OutMagazineRatio, float& OutReserveRatio) const
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		OutMagazineRatio = CachedPrimaryMagazineAmmoRatio;
		OutReserveRatio = CachedPrimaryReserveAmmoRatio;
		return;
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		OutMagazineRatio = CachedSecondaryMagazineAmmoRatio;
		OutReserveRatio = CachedSecondaryReserveAmmoRatio;
		return;
	}

	OutMagazineRatio = 1.0f;
	OutReserveRatio = 1.0f;
}

void APRPlayerState::SetCachedAmmoRatios(EPRWeaponSlotType SlotType, float MagazineRatio, float ReserveRatio)
{
	const float ClampedMagazineRatio = FMath::Clamp(MagazineRatio, 0.0f, 1.0f);
	const float ClampedReserveRatio = FMath::Clamp(ReserveRatio, 0.0f, 1.0f);

	if (SlotType == EPRWeaponSlotType::Primary)
	{
		CachedPrimaryMagazineAmmoRatio = ClampedMagazineRatio;
		CachedPrimaryReserveAmmoRatio = ClampedReserveRatio;
		return;
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		CachedSecondaryMagazineAmmoRatio = ClampedMagazineRatio;
		CachedSecondaryReserveAmmoRatio = ClampedReserveRatio;
	}
}
void APRPlayerState::InitializePrimaryInfoFromSaveData(const FPRCharacterSaveData& InSaveData)
{
	if (!HasAuthority())
	{
		return;
	}
	CurrentSaveData = InSaveData;
	DisplayName    = InSaveData.DisplayName;
	CharacterLevel = InSaveData.Level;
	Experience = InSaveData.Experience;
	StatUpgradeInfo = InSaveData.Stats;
	if (IsValid(GrowthComponent))
	{
		GrowthComponent->ApplyGrowthSaveData(InSaveData.Experience, InSaveData.Level, InSaveData.Stats);
	}
	bPendingSaveDataApply = true;
	bPendingStartUpItemsMerge = false;

	ForceNetUpdate();
}

void APRPlayerState::QueueSaveDataApply(const FPRCharacterSaveData& InSaveData, bool bMergeStartUpItems)
{
	if (!HasAuthority())
	{
		return;
	}

	InitializePrimaryInfoFromSaveData(InSaveData);
	bPendingStartUpItemsMerge = bMergeStartUpItems;
}

void APRPlayerState::MarkCharacterPayloadAccepted()
{
	if (!HasAuthority())
	{
		return;
	}

	bCharacterPayloadAccepted = true;
}

void APRPlayerState::ApplyPendingSaveData()
{
	if (!HasAuthority() || !bPendingSaveDataApply)
	{
		return;
	}

	ApplySaveDataInternal(CurrentSaveData, bPendingStartUpItemsMerge);
}

void APRPlayerState::ApplySaveData(const FPRCharacterSaveData& InSaveData)
{
	ApplySaveDataInternal(InSaveData, false);
}

void APRPlayerState::ApplySaveDataInternal(const FPRCharacterSaveData& InSaveData, bool bMergeStartUpItems)
{
	if (!HasAuthority())
	{
		return;
	}
	if (IsValid(AbilitySystemComponent))
	{
		// Attribute Base 복원
		AbilitySystemComponent->ApplyAttributeBaseSnapshot(InSaveData.AttributeBaseSnapshot);
	}
	if (IsValid(InventoryComponent))
	{
		// 인벤토리 복원
		InventoryComponent->ApplySaveData(InSaveData.Inventory);
	}
	if (IsValid(WeaponManagerComponent))
	{
		// 무기 장착 복원
		WeaponManagerComponent->ApplySaveData(InSaveData.WeaponManager);	
	}
	if (IsValid(EquipmentManagerComponent))
	{
		// 비무기 장비 복원
		EquipmentManagerComponent->ApplySaveData(InSaveData.Equipment);
	}
	if (IsValid(QuickSlotComponent))
	{
		// 퀵슬롯 복원
		QuickSlotComponent->ApplySaveData(InSaveData.QuickSlot);
	}
	if (IsValid(CurrencyComponent))
	{
		// 재화 복원
		CurrencyComponent->ApplySaveData(InSaveData.Currency);
	}
	CurrentSaveData = InSaveData;
	DisplayName = InSaveData.DisplayName;
	CharacterLevel = InSaveData.Level;
	Experience = InSaveData.Experience;
	StatUpgradeInfo = InSaveData.Stats;
	if (IsValid(GrowthComponent))
	{
		GrowthComponent->ApplyGrowthSaveData(InSaveData.Experience, InSaveData.Level, InSaveData.Stats);
	}

	if (bMergeStartUpItems)
	{
		// 신규 또는 빈 세이브의 기본 지급 아이템 보강
		GiveStartUpItems();
	}

	bPendingSaveDataApply = false;
	bPendingStartUpItemsMerge = false;

	ForceNetUpdate();
}

FPRCharacterSaveData APRPlayerState::MakeSaveData() const
{
	FPRCharacterSaveData SaveData;
	SaveData.Version = EPRSaveVersion::V1;
	SaveData.DisplayName = DisplayName;
	SaveData.Level = IsValid(AbilitySystemComponent) && IsValid(GrowthSet)
		? FMath::Max(FMath::RoundToInt(AbilitySystemComponent->GetNumericAttribute(UPRAttributeSet_Growth::GetLevelAttribute())), 1)
		: CharacterLevel;
	SaveData.Experience = IsValid(AbilitySystemComponent) && IsValid(GrowthSet)
		? FMath::Max(static_cast<int64>(AbilitySystemComponent->GetNumericAttribute(UPRAttributeSet_Growth::GetExperienceAttribute())), static_cast<int64>(0))
		: Experience;
	SaveData.Stats = IsValid(GrowthComponent) ? GrowthComponent->MakeGrowthSaveData() : StatUpgradeInfo;
	if (IsValid(InventoryComponent))
	{
		SaveData.Inventory = InventoryComponent->MakeSaveData();
	}
	if (IsValid(WeaponManagerComponent))
	{
		SaveData.WeaponManager = WeaponManagerComponent->MakeSaveData();
	}
	if (IsValid(EquipmentManagerComponent))
	{
		SaveData.Equipment = EquipmentManagerComponent->MakeSaveData();
	}
	if (IsValid(QuickSlotComponent))
	{
		SaveData.QuickSlot = QuickSlotComponent->MakeSaveData();
	}
	if (IsValid(CurrencyComponent))
	{
		SaveData.Currency = CurrencyComponent->MakeSaveData();
	}
	const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
	if (IsValid(AbilitySystemComponent) && IsValid(Registry))
	{
		SaveData.AttributeBaseSnapshot = AbilitySystemComponent->MakeAttributeBaseSnapshot(
			Registry->GetPersistentBaseAttributes(EPRCharacterRole::Player));
	}
	return SaveData;
}

void APRPlayerState::SyncGrowthCache(int64 NewExperience, int32 NewLevel, const FPRCharacterStatUpgradeInfo& NewStats)
{
	if (!HasAuthority())
	{
		return;
	}

	Experience = FMath::Max(NewExperience, static_cast<int64>(0));
	CharacterLevel = FMath::Max(NewLevel, 1);
	StatUpgradeInfo = NewStats;
}

void APRPlayerState::SetCameraSensitivity(float Sensitivity)
{
	float NewSensetivity = FMath::Clamp(Sensitivity, 0.05f, 1.0f);
	CameraSensitivity = NewSensetivity;
	UE_LOG(LogTemp, Log, TEXT("MouseSensitivity = %f"), CameraSensitivity);
	OnMouseSensitivityChanged.Broadcast(CameraSensitivity);
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
}

void APRPlayerState::ResetState()
{
	// 서버 권위 전용
	if (!HasAuthority() || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	// ASC 런타임 상태 정리
	AbilitySystemComponent->ResetSystem();

	if (IsValid(GrowthComponent))
	{
		// 성장 보너스 효과 재동기화
		GrowthComponent->ResetSystem();
	}

	if (IsValid(InventoryComponent))
	{
		// 인벤토리 런타임 캐시 정리
		InventoryComponent->ResetSystem();
	}

	if (IsValid(EquipmentManagerComponent))
	{
		// 장비 외형 알림 재동기화
		EquipmentManagerComponent->ResetSystem();
	}

	if (IsValid(WeaponManagerComponent))
	{
		// 기존 Pawn 무기 부착 상태 정리
		WeaponManagerComponent->ResetSystem();
	}

	if (IsValid(QuickSlotComponent))
	{
		// 퀵슬롯 소비 아이템 캐시 재조회
		QuickSlotComponent->ResetSystem();
	}

	if (IsValid(CurrencyComponent))
	{
		// 재화 시스템 리스폰 정리
		CurrencyComponent->ResetSystem();
	}
}

void APRPlayerState::PrepareStateForRespawn()
{
	if (!HasAuthority())
	{
		return;
	}

	// 새 Pawn PossessedBy 이후 ApplySaveData가 장비, 무기, 인벤토리를 같은 경로로 복원하도록 리스폰 직전 상태 보관
	CurrentSaveData = MakeSaveData();
	bPendingSaveDataApply = true;
	bPendingRespawnRecovery = true;
}

bool APRPlayerState::ConsumePendingRespawnRecovery()
{
	if (!HasAuthority() || !bPendingRespawnRecovery)
	{
		return false;
	}

	bPendingRespawnRecovery = false;
	return true;
}

void APRPlayerState::GrantCharacterAbilitySet(const UPRAbilitySet* InAbilitySet, UObject* InSourceObject)
{
	if (!HasAuthority() || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	// 이전 Pawn AbilitySet 회수
	AbilitySystemComponent->ClearAbilitySetByHandles(CharacterAbilitySetHandles);

	if (IsValid(InAbilitySet))
	{
		// 새 Pawn AbilitySet 부여
		AbilitySystemComponent->GiveAbilitySet(InAbilitySet, CharacterAbilitySetHandles, InSourceObject);
	}
}
