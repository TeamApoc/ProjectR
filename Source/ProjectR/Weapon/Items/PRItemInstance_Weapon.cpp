// Copyright ProjectR. All Rights Reserved.

#include "PRItemInstance_Weapon.h"

#include "GameFramework/Actor.h"
#include "Logging/LogMacros.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Character/PRCharacterBase.h"
#include "ProjectR/Inventory/Components/PRInventoryComponent.h"
#include "ProjectR/Weapon/Data/PRWeaponDataAsset.h"
#include "ProjectR/Weapon/Data/PRWeaponModDataAsset.h"
#include "ProjectR/Weapon/Items/PRItemInstance_Mod.h"

namespace
{
	// 소유 캐릭터 기준 프로젝트 ASC를 조회한다
	UPRAbilitySystemComponent* ResolveOwnerAbilitySystem(AActor* OwnerActor)
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(OwnerActor))
		{
			if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
			{
				return Cast<UPRAbilitySystemComponent>(ASC);
			}
		}

		APRCharacterBase* OwnerCharacter = Cast<APRCharacterBase>(OwnerActor);
		if (IsValid(OwnerCharacter))
		{
			return OwnerCharacter->GetPRAbilitySystemComponent();
		}

		return nullptr;
	}
}

void UPRItemInstance_Weapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPRItemInstance_Weapon, ModData);
	DOREPLIFETIME(UPRItemInstance_Weapon, EquippedModItem);
	DOREPLIFETIME(UPRItemInstance_Weapon, bIsEquippedCurrentWeaponSlot);
	DOREPLIFETIME(UPRItemInstance_Weapon, UpgradeLevel);
}

void UPRItemInstance_Weapon::InitializeItem(UPRItemDataAsset* InItemData, int32 InitialStackCount)
{
	Super::InitializeItem(InItemData, InitialStackCount);
}

void UPRItemInstance_Weapon::InitializeMod(UPRWeaponModDataAsset* InModData)
{
	ModData = InModData;
	ClearEquippedModItem();
	bIsEquippedCurrentWeaponSlot = false;
	CachedWeaponAbilitySet = nullptr;
	CachedModAbilitySet = nullptr;
}

UPRWeaponDataAsset* UPRItemInstance_Weapon::GetWeaponData() const
{
	return Cast<UPRWeaponDataAsset>(ItemData);
}

void UPRItemInstance_Weapon::FillSaveEntry(FPRWeaponItemSaveEntry& OutEntry) const
{
	// 저장 엔트리 기본값
	OutEntry = FPRWeaponItemSaveEntry();
	OutEntry.WeaponData = GetWeaponData();
	OutEntry.StackCount = GetStackCount();
}

void UPRItemInstance_Weapon::ApplySaveEntry(const FPRWeaponItemSaveEntry& InEntry)
{
	// v1 인스턴스 상태 복원
	StackCount = FMath::Max(InEntry.StackCount, 0);
}

bool UPRItemInstance_Weapon::MatchesWeaponData(const UPRWeaponDataAsset* InWeaponData) const
{
	return GetWeaponData() == InWeaponData;
}

bool UPRItemInstance_Weapon::HasEquippedModItem() const
{
	return IsValid(EquippedModItem);
}

void UPRItemInstance_Weapon::SetModData(UPRWeaponModDataAsset* NewModData)
{
	ModData = NewModData;
	CachedModAbilitySet = nullptr;
}

void UPRItemInstance_Weapon::SetEquippedModItem(UPRItemInstance_Mod* NewModItem)
{
	EquippedModItem = NewModItem;
}

void UPRItemInstance_Weapon::ClearEquippedModItem()
{
	EquippedModItem = nullptr;
}

void UPRItemInstance_Weapon::OnEquipped(AActor* OwnerActor)
{
	// 장착 생명주기에서는 실제 부여 로직 대신 인터페이스 함수만 통과시킨다
	if (!IsValid(OwnerActor))
	{
		return;
	}

	// 활성 슬롯 상태는 AbilitySet 부여 성공 여부와 분리해 먼저 기록한다
	bIsEquippedCurrentWeaponSlot = true;

	GrantEquippedAbilitySets(OwnerActor);
}

void UPRItemInstance_Weapon::OnUnequipped(AActor* OwnerActor)
{
	// 해제 생명주기에서는 실제 회수 로직 대신 인터페이스 함수만 통과시킨다
	bIsEquippedCurrentWeaponSlot = false;

	if (!IsValid(OwnerActor))
	{
		return;
	}

	ClearEquippedAbilitySets(OwnerActor);
	ResetTransientRuntimeOnDeactivate();
}

void UPRItemInstance_Weapon::OnModChanged(AActor* OwnerActor, UPRWeaponModDataAsset* NewModData)
{
	// Mod 교체 생명주기에서는 기존 핸들을 정리한 뒤 새 Mod 기준으로 재구성한다
	if (!IsValid(OwnerActor))
	{
		return;
	}

	RebuildModAbility(OwnerActor, NewModData);
}

void UPRItemInstance_Weapon::GrantEquippedAbilitySets(AActor* OwnerActor)
{
	if (!IsValid(OwnerActor))
	{
		return;
	}

	UPRAbilitySystemComponent* ASC = ResolveOwnerAbilitySystem(OwnerActor);
	if (!IsValid(ASC) || !ASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	if (UPRAbilitySet* WeaponAbilitySet = GetWeaponAbilitySet())
	{
		ASC->GiveAbilitySet(WeaponAbilitySet, WeaponAbilityHandles, this);
	}

	if (UPRAbilitySet* ModAbilitySet = GetModAbilitySet())
	{
		ASC->GiveAbilitySet(ModAbilitySet, ModAbilityHandles, this);
	}

	LastWeaponFailReason = EPRWeaponActionFailReason::None;
	LastModFailReason = EPRWeaponModFailReason::None;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("무기 아이템의 어빌리티 셋 부여. Owner=%s Weapon=%s Mod=%s WeaponAbilities=%d WeaponEffects=%d ModAbilities=%d ModEffects=%d"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(GetWeaponData()),
		*GetNameSafe(ModData),
		WeaponAbilityHandles.AbilityHandles.Num(),
		WeaponAbilityHandles.EffectHandles.Num(),
		ModAbilityHandles.AbilityHandles.Num(),
		ModAbilityHandles.EffectHandles.Num());
}

void UPRItemInstance_Weapon::ClearEquippedAbilitySets(AActor* OwnerActor)
{
	if (!IsValid(OwnerActor))
	{
		return;
	}

	UPRAbilitySystemComponent* ASC = ResolveOwnerAbilitySystem(OwnerActor);
	if (!IsValid(ASC) || !ASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	ASC->ClearAbilitySetByHandles(WeaponAbilityHandles);
	ASC->ClearAbilitySetByHandles(ModAbilityHandles);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("무기 아이템의 어빌리티 셋 회수. Owner=%s Weapon=%s Mod=%s"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(GetWeaponData()),
		*GetNameSafe(ModData));
}

void UPRItemInstance_Weapon::RebuildModAbility(AActor* OwnerActor, UPRWeaponModDataAsset* NewModData)
{
	if (!IsValid(OwnerActor))
	{
		return;
	}

	UPRAbilitySystemComponent* ASC = ResolveOwnerAbilitySystem(OwnerActor);
	if (!IsValid(ASC) || !ASC->IsOwnerActorAuthoritative())
	{
		SetModData(NewModData);
		return;
	}

	const UPRWeaponModDataAsset* PreviousModData = ModData;
	const bool bWasEquipped = IsEquippedCurrentWeaponSlot();

	ASC->ClearAbilitySetByHandles(ModAbilityHandles);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Weapon item mod rebuild start. Owner=%s Weapon=%s PrevMod=%s NewMod=%s WasEquipped=%d"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(GetWeaponData()),
		*GetNameSafe(PreviousModData),
		*GetNameSafe(NewModData),
		bWasEquipped ? 1 : 0);

	SetModData(NewModData);

	if (!IsValid(ModData))
	{
		FireModeState = EPRWeaponFireModeState::BaseFire;
		LastModFailReason = EPRWeaponModFailReason::MissingMod;
		ModEffectEndServerWorldTimeSeconds = 0.0f;
		return;
	}

	LastModFailReason = EPRWeaponModFailReason::None;

	if (bWasEquipped)
	{
		if (UPRAbilitySet* ModAbilitySet = GetModAbilitySet())
		{
			ASC->GiveAbilitySet(ModAbilitySet, ModAbilityHandles, this);
		}
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Weapon item mod rebuild complete. Owner=%s Weapon=%s CurrentMod=%s ModAbilities=%d ModEffects=%d"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(GetWeaponData()),
		*GetNameSafe(ModData),
		ModAbilityHandles.AbilityHandles.Num(),
		ModAbilityHandles.EffectHandles.Num());
}

void UPRItemInstance_Weapon::ResetTransientRuntimeOnDeactivate()
{
	FireModeState = EPRWeaponFireModeState::BaseFire;
	ModEffectEndServerWorldTimeSeconds = 0.0f;

	LastWeaponFailReason = EPRWeaponActionFailReason::None;
	LastModFailReason = EPRWeaponModFailReason::None;
}

float UPRItemInstance_Weapon::GetRemainingModDurationSeconds(float ServerWorldTimeSeconds) const
{
	return FMath::Max(ModEffectEndServerWorldTimeSeconds - ServerWorldTimeSeconds, 0.0f);
}

bool UPRItemInstance_Weapon::SetUpgradeLevel(int32 NewLevel)
{
	if (NewLevel < 0)
	{
		return false;
	}

	if (UpgradeLevel == NewLevel)
	{
		return false;
	}

	UpgradeLevel = NewLevel;
	NotifyInventoryChanged(EPRInventoryChangeReason::WeaponUpgradeChanged);
	return true;
}

bool UPRItemInstance_Weapon::IncreaseUpgradeLevel()
{
	return SetUpgradeLevel(UpgradeLevel + 1);
}

UPRAbilitySet* UPRItemInstance_Weapon::GetWeaponAbilitySet()
{
	auto WeaponData = GetWeaponData();
	if (!IsValid(WeaponData))
	{
		return nullptr;
	}

	if (!IsValid(CachedWeaponAbilitySet))
	{
		CachedWeaponAbilitySet = CreateRuntimeAbilitySet(
			this,
			WeaponData->EquippedAbilities,
			WeaponData->EquippedEffects);
	}

	return CachedWeaponAbilitySet;
}

UPRAbilitySet* UPRItemInstance_Weapon::GetModAbilitySet()
{
	if (!IsValid(ModData))
	{
		return nullptr;
	}

	if (!IsValid(CachedModAbilitySet))
	{
		CachedModAbilitySet = CreateRuntimeAbilitySet(
			this,
			ModData->EquippedAbilities,
			ModData->EquippedEffects);
	}

	return CachedModAbilitySet;
}

void UPRItemInstance_Weapon::OnRep_ModData()
{
	NotifyInventoryChanged(EPRInventoryChangeReason::ModEquipChanged);
}

void UPRItemInstance_Weapon::OnRep_EquippedModItem()
{
	// 클라이언트에서 장착 Mod Item 참조만 갱신되는 상황을 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Client] Weapon item mod item replicated. Item = %s | Weapon = %s | Mod = %s | ModItem = %s"),
		*GetNameSafe(this),
		*GetNameSafe(GetWeaponData()),
		*GetNameSafe(ModData),
		*GetNameSafe(EquippedModItem.Get()));

	NotifyInventoryChanged(EPRInventoryChangeReason::ModEquipChanged);
}

void UPRItemInstance_Weapon::OnRep_UpgradeLevel()
{
	NotifyInventoryChanged(EPRInventoryChangeReason::WeaponUpgradeChanged);
}
