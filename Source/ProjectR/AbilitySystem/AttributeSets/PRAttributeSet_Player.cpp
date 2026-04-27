// Copyright ProjectR. All Rights Reserved.

#include "PRAttributeSet_Player.h"

#include "GameplayEffectExtension.h"
#include "Logging/LogMacros.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Weapon/Data/PRWeaponDataAsset.h"
#include "ProjectR/Weapon/Data/PRWeaponModDataAsset.h"

// 26.04.27, 이건주, 무기 자원 어트리뷰트 셋 추가

// PreAttributeChange에서 탄약/Mod 관련 속성들을 개별 비교문 없이 그룹 처리하기 위해 사용
namespace
{
	// 주무기 슬롯 자원 속성 판별
	bool IsPrimarySlotAttribute(const FGameplayAttribute& Attribute)
	{
		return Attribute == UPRAttributeSet_Player::GetPrimaryMagazineAmmoAttribute()
			|| Attribute == UPRAttributeSet_Player::GetPrimaryReserveAmmoAttribute()
			|| Attribute == UPRAttributeSet_Player::GetPrimaryModGaugeAttribute()
			|| Attribute == UPRAttributeSet_Player::GetPrimaryModStackAttribute();
	}
	//보조무기 슬롯 자원 속성 판별
	bool IsSecondarySlotAttribute(const FGameplayAttribute& Attribute)
	{
		return Attribute == UPRAttributeSet_Player::GetSecondaryMagazineAmmoAttribute()
			|| Attribute == UPRAttributeSet_Player::GetSecondaryReserveAmmoAttribute()
			|| Attribute == UPRAttributeSet_Player::GetSecondaryModGaugeAttribute()
			|| Attribute == UPRAttributeSet_Player::GetSecondaryModStackAttribute();
	}
}


// =====  UAttributeSet Interface =====
void UPRAttributeSet_Player::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	else if (Attribute == GetMaxStaminaAttribute() || Attribute == GetStaminaRegenRateAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (IsPrimarySlotAttribute(Attribute) || IsSecondarySlotAttribute(Attribute))
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UPRAttributeSet_Player::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Stamina 고갈 시 State.StaminaDepleted 태그 부여, 회복 시 제거
	if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
		if (IsValid(ASC))
		{
			const FGameplayTag DepletedTag = PRGameplayTags::State_StaminaDepleted;
			const bool bHasTag = ASC->HasMatchingGameplayTag(DepletedTag);
			if (GetStamina() <= 0.0f && !bHasTag)
			{
				ASC->AddLooseGameplayTag(DepletedTag);
			}
			else if (GetStamina() > 0.0f && bHasTag)
			{
				ASC->RemoveLooseGameplayTag(DepletedTag);
			}
		}
	}
}

void UPRAttributeSet_Player::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, StaminaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, PrimaryMagazineAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, PrimaryReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, PrimaryModGauge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, PrimaryModStack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, SecondaryMagazineAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, SecondaryReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, SecondaryModGauge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, SecondaryModStack, COND_None, REPNOTIFY_Always);
}

FPRWeaponSlotResourceState UPRAttributeSet_Player::BuildSlotResourceState(EPRWeaponSlotType SlotType) const
{
	FPRWeaponSlotResourceState ResourceState;
	ResourceState.SlotType = SlotType;

	if (SlotType == EPRWeaponSlotType::Primary)
	{
		ResourceState.MagazineAmmo = FMath::RoundToInt(GetPrimaryMagazineAmmo());
		ResourceState.ReserveAmmo = FMath::RoundToInt(GetPrimaryReserveAmmo());
		ResourceState.ModGauge = GetPrimaryModGauge();
		ResourceState.ModStack = FMath::RoundToInt(GetPrimaryModStack());
	}
	else if (SlotType == EPRWeaponSlotType::Secondary)
	{
		ResourceState.MagazineAmmo = FMath::RoundToInt(GetSecondaryMagazineAmmo());
		ResourceState.ReserveAmmo = FMath::RoundToInt(GetSecondaryReserveAmmo());
		ResourceState.ModGauge = GetSecondaryModGauge();
		ResourceState.ModStack = FMath::RoundToInt(GetSecondaryModStack());
	}

	return ResourceState;
}

void UPRAttributeSet_Player::InitializeSlotResources(EPRWeaponSlotType SlotType, const UPRWeaponDataAsset* WeaponData, const UPRWeaponModDataAsset* ModData)
{
	const int32 MagazineAmmo = IsValid(WeaponData) ? FMath::Max(WeaponData->DefaultMagazineAmmo, 0) : 0;
	const int32 ReserveAmmo = IsValid(WeaponData) ? FMath::Max(WeaponData->DefaultReserveAmmo, 0) : 0;
	const float ModGauge = IsValid(ModData) ? FMath::Max(ModData->MaxGauge, 0.0f) : 0.0f;
	const int32 ModStack = IsValid(ModData) ? FMath::Max(ModData->InitialStack, 0) : 0;

	if (SlotType == EPRWeaponSlotType::Primary)
	{
		SetPrimaryMagazineAmmo(MagazineAmmo);
		SetPrimaryReserveAmmo(ReserveAmmo);
		SetPrimaryModGauge(ModGauge);
		SetPrimaryModStack(ModStack);
	}
	else if (SlotType == EPRWeaponSlotType::Secondary)
	{
		SetSecondaryMagazineAmmo(MagazineAmmo);
		SetSecondaryReserveAmmo(ReserveAmmo);
		SetSecondaryModGauge(ModGauge);
		SetSecondaryModStack(ModStack);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("PlayerSet InitializeSlotResources Slot=%d Weapon=%s Mod=%s Magazine=%d Reserve=%d ModGauge=%.2f ModStack=%d"),
		static_cast<int32>(SlotType),
		*GetNameSafe(WeaponData),
		*GetNameSafe(ModData),
		MagazineAmmo,
		ReserveAmmo,
		ModGauge,
		ModStack);
}

void UPRAttributeSet_Player::ApplySlotResourceDelta(EPRWeaponSlotType SlotType, const FPRWeaponSlotResourceDelta& Delta)
{
	FPRWeaponSlotResourceState ResourceState = BuildSlotResourceState(SlotType);
	const FPRWeaponSlotResourceState PreviousState = ResourceState;
	ResourceState.MagazineAmmo = FMath::Max(ResourceState.MagazineAmmo + Delta.MagazineDelta, 0);
	ResourceState.ReserveAmmo = FMath::Max(ResourceState.ReserveAmmo + Delta.ReserveDelta, 0);
	ResourceState.ModGauge = FMath::Max(ResourceState.ModGauge + Delta.ModGaugeDelta, 0.0f);
	ResourceState.ModStack = FMath::Max(ResourceState.ModStack + Delta.ModStackDelta, 0);

	if (SlotType == EPRWeaponSlotType::Primary)
	{
		SetPrimaryMagazineAmmo(ResourceState.MagazineAmmo);
		SetPrimaryReserveAmmo(ResourceState.ReserveAmmo);
		SetPrimaryModGauge(ResourceState.ModGauge);
		SetPrimaryModStack(ResourceState.ModStack);
	}
	else if (SlotType == EPRWeaponSlotType::Secondary)
	{
		SetSecondaryMagazineAmmo(ResourceState.MagazineAmmo);
		SetSecondaryReserveAmmo(ResourceState.ReserveAmmo);
		SetSecondaryModGauge(ResourceState.ModGauge);
		SetSecondaryModStack(ResourceState.ModStack);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("PlayerSet ApplySlotResourceDelta Slot=%d Prev(Mag=%d Res=%d Gauge=%.2f Stack=%d) Delta(Mag=%d Res=%d Gauge=%.2f Stack=%d) New(Mag=%d Res=%d Gauge=%.2f Stack=%d)"),
		static_cast<int32>(SlotType),
		PreviousState.MagazineAmmo,
		PreviousState.ReserveAmmo,
		PreviousState.ModGauge,
		PreviousState.ModStack,
		Delta.MagazineDelta,
		Delta.ReserveDelta,
		Delta.ModGaugeDelta,
		Delta.ModStackDelta,
		ResourceState.MagazineAmmo,
		ResourceState.ReserveAmmo,
		ResourceState.ModGauge,
		ResourceState.ModStack);
}

void UPRAttributeSet_Player::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, Stamina, OldValue);
}

void UPRAttributeSet_Player::OnRep_MaxStamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, MaxStamina, OldValue);
}

void UPRAttributeSet_Player::OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, StaminaRegenRate, OldValue);
}

void UPRAttributeSet_Player::OnRep_PrimaryMagazineAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, PrimaryMagazineAmmo, OldValue);
}

void UPRAttributeSet_Player::OnRep_PrimaryReserveAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, PrimaryReserveAmmo, OldValue);
}

void UPRAttributeSet_Player::OnRep_PrimaryModGauge(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, PrimaryModGauge, OldValue);
}

void UPRAttributeSet_Player::OnRep_PrimaryModStack(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, PrimaryModStack, OldValue);
}

void UPRAttributeSet_Player::OnRep_SecondaryMagazineAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, SecondaryMagazineAmmo, OldValue);
}

void UPRAttributeSet_Player::OnRep_SecondaryReserveAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, SecondaryReserveAmmo, OldValue);
}

void UPRAttributeSet_Player::OnRep_SecondaryModGauge(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, SecondaryModGauge, OldValue);
}

void UPRAttributeSet_Player::OnRep_SecondaryModStack(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, SecondaryModStack, OldValue);
}
