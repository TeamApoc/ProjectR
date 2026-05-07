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
// 탄약 자원은 모두 raw 단위로 저장하며, 표시·소비 시점에 AmmoScale로 환산한다
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

	/*~ UPRAttributeSet_Weapon Interface ~*/
	// 탄약 타입에 대응하는 raw 탄창 어트리뷰트 핸들 반환
	static FGameplayAttribute GetMagazineAmmoAttribute(EPRAmmoType AmmoType);

	// 탄약 타입에 대응하는 raw 예비탄 어트리뷰트 핸들 반환
	static FGameplayAttribute GetReserveAmmoAttribute(EPRAmmoType AmmoType);

	// 탄약 타입에 대응하는 효율 단가 어트리뷰트 핸들 반환
	static FGameplayAttribute GetAmmoScaleAttribute(EPRAmmoType AmmoType);

	// 탄약 타입에 대응하는 보유 한도 비율 어트리뷰트 핸들 반환
	static FGameplayAttribute GetReserveAmmoRatioAttribute(EPRAmmoType AmmoType);

	// 탄약 타입에 대응하는 raw 탄창 현재 값 반환
	float GetMagazineAmmoByType(EPRAmmoType AmmoType) const;

	// 탄약 타입에 대응하는 raw 예비탄 현재 값 반환
	float GetReserveAmmoByType(EPRAmmoType AmmoType) const;

	// 탄약 타입에 대응하는 효율 단가 현재 값 반환
	float GetAmmoScaleByType(EPRAmmoType AmmoType) const;

	// 탄약 타입에 대응하는 보유 한도 비율 현재 값 반환
	float GetReserveAmmoRatioByType(EPRAmmoType AmmoType) const;

	// HUD 표시용 탄창 발수 (raw / scale, scale<=0이면 0 반환)
	int32 GetDisplayedMagazineAmmo(EPRAmmoType AmmoType) const;

	// HUD 표시용 예비탄 발수 (raw / scale, scale<=0이면 0 반환)
	int32 GetDisplayedReserveAmmo(EPRAmmoType AmmoType) const;

	// raw 단위 예비탄 보유 한도 (Ratio × Scale × Baseline)
	float GetMaxReserveAmmoRaw(EPRAmmoType AmmoType) const;

protected:
	UFUNCTION()
	void OnRep_PrimaryMagazineAmmo(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PrimaryReserveAmmo(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PrimaryAmmoScale(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PrimaryReserveAmmoRatio(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PrimaryModGauge(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PrimaryModStack(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SecondaryMagazineAmmo(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SecondaryReserveAmmo(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SecondaryAmmoScale(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SecondaryReserveAmmoRatio(const FGameplayAttributeData& OldValue);

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
	// 모든 무기 공통 raw 탄창 baseline (1.0 효율 단가 기준 100발 탄창)
	static constexpr float MagazineRawBaseline = 100.f;

	// 주무기 슬롯 raw 탄창 잔탄
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryMagazineAmmo, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData PrimaryMagazineAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, PrimaryMagazineAmmo)

	// 주무기 슬롯 raw 예비 탄약
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryReserveAmmo, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData PrimaryReserveAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, PrimaryReserveAmmo)

	// 주무기 슬롯 효율 단가 (낮을수록 같은 raw로 더 많이 발사)
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryAmmoScale, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData PrimaryAmmoScale;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, PrimaryAmmoScale)

	// 주무기 슬롯 보유 한도 비율 (탄창 baseline 대비 예비탄 한도 배수)
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryReserveAmmoRatio, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData PrimaryReserveAmmoRatio;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, PrimaryReserveAmmoRatio)

	// 주무기 슬롯 Mod 게이지
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryModGauge, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData PrimaryModGauge;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, PrimaryModGauge)

	// 주무기 슬롯 Mod 스택
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryModStack, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData PrimaryModStack;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, PrimaryModStack)

	// 보조무기 슬롯 raw 탄창 잔탄
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryMagazineAmmo, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData SecondaryMagazineAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, SecondaryMagazineAmmo)

	// 보조무기 슬롯 raw 예비 탄약
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryReserveAmmo, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData SecondaryReserveAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, SecondaryReserveAmmo)

	// 보조무기 슬롯 효율 단가
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryAmmoScale, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData SecondaryAmmoScale;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, SecondaryAmmoScale)

	// 보조무기 슬롯 보유 한도 비율
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryReserveAmmoRatio, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData SecondaryReserveAmmoRatio;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, SecondaryReserveAmmoRatio)

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
