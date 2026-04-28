// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "PRAttributeSet_Player.generated.h"

class UPRWeaponDataAsset;
class UPRWeaponModDataAsset;

// 플레이어 전용 자원과 무기 슬롯 자원 정본을 관리하는 속성 세트
UCLASS()
class PROJECTR_API UPRAttributeSet_Player : public UAttributeSet
{
	GENERATED_BODY()

public:
	/*~ UAttributeSet Interface ~*/
	// 속성 변경 전 최소 클램프 규칙을 적용한다
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	// 속성 변경 후 후처리 규칙을 적용한다
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	/*~ UObject Interface ~*/
	// 복제할 속성을 등록한다
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 슬롯 타입 기준 현재 자원 스냅샷을 만든다
	FPRWeaponSlotResourceState BuildSlotResourceState(EPRWeaponSlotType SlotType) const;

	// 무기 장착 또는 세이브 복원 시 슬롯 자원을 초기화한다
	void InitializeSlotResources(EPRWeaponSlotType SlotType, const UPRWeaponDataAsset* WeaponData, const UPRWeaponModDataAsset* ModData);

	// 발사, 재장전, Mod 소비 결과를 슬롯 단위로 반영한다
	void ApplySlotResourceDelta(EPRWeaponSlotType SlotType, const FPRWeaponSlotResourceDelta& Delta);

public:
	// 스태미너
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData Stamina;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, Stamina)

	// 최대 스태미너
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData MaxStamina;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, MaxStamina)

	// 초당 스태미너 회복량
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_StaminaRegenRate, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData StaminaRegenRate;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, StaminaRegenRate)

	// 주무기 슬롯 탄창 잔탄
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryMagazineAmmo, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData PrimaryMagazineAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, PrimaryMagazineAmmo)

	// 주무기 슬롯 예비 탄약
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryReserveAmmo, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData PrimaryReserveAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, PrimaryReserveAmmo)

	// 주무기 슬롯 Mod 게이지
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryModGauge, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData PrimaryModGauge;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, PrimaryModGauge)

	// 주무기 슬롯 Mod 스택
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryModStack, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData PrimaryModStack;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, PrimaryModStack)

	// 보조무기 슬롯 탄창 잔탄
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryMagazineAmmo, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData SecondaryMagazineAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, SecondaryMagazineAmmo)

	// 보조무기 슬롯 예비 탄약
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryReserveAmmo, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData SecondaryReserveAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, SecondaryReserveAmmo)

	// 보조무기 슬롯 Mod 게이지
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryModGauge, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData SecondaryModGauge;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, SecondaryModGauge)

	// 보조무기 슬롯 Mod 스택
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryModStack, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData SecondaryModStack;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, SecondaryModStack)

protected:
	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PrimaryMagazineAmmo(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PrimaryReserveAmmo(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PrimaryModGauge(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PrimaryModStack(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SecondaryMagazineAmmo(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SecondaryReserveAmmo(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SecondaryModGauge(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SecondaryModStack(const FGameplayAttributeData& OldValue);
};
