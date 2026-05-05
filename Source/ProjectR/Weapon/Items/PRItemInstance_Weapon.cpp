// Copyright ProjectR. All Rights Reserved.

#include "PRItemInstance_Weapon.h"

#include "GameFramework/Actor.h"
#include "Logging/LogMacros.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Character/PRCharacterBase.h"
#include "ProjectR/Weapon/Data/PRWeaponDataAsset.h"
#include "ProjectR/Weapon/Data/PRWeaponModDataAsset.h"
#include "ProjectR/Weapon/Items/PRItemInstance_Mod.h"

namespace
{
	// 소유 캐릭터 기준 프로젝트 ASC를 조회한다
	UPRAbilitySystemComponent* ResolveOwnerAbilitySystem(AActor* OwnerActor)
	{
		APRCharacterBase* OwnerCharacter = Cast<APRCharacterBase>(OwnerActor);
		if (!IsValid(OwnerCharacter))
		{
			return nullptr;
		}

		return OwnerCharacter->GetPRAbilitySystemComponent();
	}
}

void UPRItemInstance_Weapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPRItemInstance_Weapon, WeaponData);
	DOREPLIFETIME(UPRItemInstance_Weapon, ModData);
	DOREPLIFETIME(UPRItemInstance_Weapon, EquippedModItem);
	DOREPLIFETIME(UPRItemInstance_Weapon, bIsEquippedCurrentWeaponSlot);
}

void UPRItemInstance_Weapon::InitializeWeaponItem(UPRWeaponDataAsset* InWeaponData, UPRWeaponModDataAsset* InModData)
{
	WeaponData = InWeaponData;
	ModData = InModData;
	ClearEquippedModItem();
	bIsEquippedCurrentWeaponSlot = false;
	CachedWeaponAbilitySet = nullptr;
	CachedModAbilitySet = nullptr;
}

bool UPRItemInstance_Weapon::MatchesWeaponData(const UPRWeaponDataAsset* InWeaponData) const
{
	return WeaponData == InWeaponData;
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
		ASC->GiveAbilitySet(WeaponAbilitySet, WeaponAbilityHandles);
	}

	if (UPRAbilitySet* ModAbilitySet = GetModAbilitySet())
	{
		ASC->GiveAbilitySet(ModAbilitySet, ModAbilityHandles);
	}

	LastWeaponFailReason = EPRWeaponActionFailReason::None;
	LastModFailReason = EPRWeaponModFailReason::None;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("무기 아이템의 어빌리티 셋 부여. Owner=%s Weapon=%s Mod=%s WeaponAbilities=%d WeaponEffects=%d ModAbilities=%d ModEffects=%d"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(WeaponData),
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
		*GetNameSafe(WeaponData),
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
		*GetNameSafe(WeaponData),
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
			ASC->GiveAbilitySet(ModAbilitySet, ModAbilityHandles);
		}
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Weapon item mod rebuild complete. Owner=%s Weapon=%s CurrentMod=%s ModAbilities=%d ModEffects=%d"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(WeaponData),
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

UPRAbilitySet* UPRItemInstance_Weapon::GetWeaponAbilitySet()
{
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

void UPRItemInstance_Weapon::OnRep_WeaponData()
{
	// 클라이언트에서 무기 데이터와 현재 Mod 연결 상태가 함께 복제됐는지 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Client] Weapon item data replicated. Item = %s | Weapon = %s | WeaponId = %s | Mod = %s | ModItem = %s"),
		*GetNameSafe(this),
		*GetNameSafe(WeaponData),
		IsValid(WeaponData) ? *WeaponData->GetDisplayName().ToString() : TEXT("None"),
		*GetNameSafe(ModData),
		*GetNameSafe(EquippedModItem.Get()));
}

void UPRItemInstance_Weapon::OnRep_EquippedModItem()
{
	// 클라이언트에서 장착 Mod Item 참조만 갱신되는 상황을 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Client] Weapon item mod item replicated. Item = %s | Weapon = %s | Mod = %s | ModItem = %s"),
		*GetNameSafe(this),
		*GetNameSafe(WeaponData),
		*GetNameSafe(ModData),
		*GetNameSafe(EquippedModItem.Get()));
}
