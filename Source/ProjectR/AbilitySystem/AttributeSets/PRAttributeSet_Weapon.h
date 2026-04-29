// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "PRAttributeSet_Weapon.generated.h"

class UPRWeaponDataAsset;
class UPRWeaponModDataAsset;

// 플레이어 무기 슬롯 자원 정보를 관리하는 속성 세트
UCLASS()
class PROJECTR_API UPRAttributeSet_Weapon : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPRAttributeSet_Weapon();
	
	/*~ UObject Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ UAttributeSet Interface ~*/
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
public:
	// 슬롯 타입 기준 현재 자원 스냅샷을 만든다
	FPRWeaponSlotResourceState BuildSlotResourceState(EPRWeaponSlotType SlotType) const;

	// 무기 장착 또는 세이브 복원 시 슬롯 자원을 초기화한다
	void InitializeSlotResources(EPRWeaponSlotType SlotType, const UPRWeaponDataAsset* WeaponData, const UPRWeaponModDataAsset* ModData);

	// 발사, 재장전, Mod 소비 결과를 슬롯 단위로 반영한다
	void ApplySlotResourceDelta(EPRWeaponSlotType SlotType, const FPRWeaponSlotResourceDelta& Delta);
	
protected:
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

	UFUNCTION() 
	void OnRep_BaseDamage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_DamageMultiplier(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_ArmorPenetration(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_WeakpointMultiplier(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_GroggyDamageMultiplier(const FGameplayAttributeData& OldValue);

public:
	// 주무기 슬롯 탄창 잔탄
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryMagazineAmmo, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData PrimaryMagazineAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, PrimaryMagazineAmmo)

	// 주무기 슬롯 예비 탄약
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryReserveAmmo, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData PrimaryReserveAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, PrimaryReserveAmmo)

	// 주무기 슬롯 Mod 게이지
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryModGauge, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData PrimaryModGauge;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, PrimaryModGauge)

	// 주무기 슬롯 Mod 스택
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryModStack, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData PrimaryModStack;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, PrimaryModStack)

	// 보조무기 슬롯 탄창 잔탄
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryMagazineAmmo, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData SecondaryMagazineAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, SecondaryMagazineAmmo)

	// 보조무기 슬롯 예비 탄약
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryReserveAmmo, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData SecondaryReserveAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, SecondaryReserveAmmo)

	// 보조무기 슬롯 Mod 게이지
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryModGauge, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData SecondaryModGauge;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, SecondaryModGauge)

	// 보조무기 슬롯 Mod 스택
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryModStack, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData SecondaryModStack;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, SecondaryModStack)

	// 무기 기본 데미지
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseDamage, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData BaseDamage;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, BaseDamage)

	// 데미지 배율
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DamageMultiplier, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData DamageMultiplier;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, DamageMultiplier)

	// 장갑 관통
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ArmorPenetration, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData ArmorPenetration;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, ArmorPenetration)

	// 약점 피해 배율
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WeakpointMultiplier, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData WeakpointMultiplier;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, WeakpointMultiplier)

	// 그로기 데미지 배율
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_GroggyDamageMultiplier, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData GroggyDamageMultiplier;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, GroggyDamageMultiplier)

};
