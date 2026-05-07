// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "PRMMC_AmmoCost.generated.h"

// 사격 GE에서 raw 탄약 차감량을 산출하는 MMC
// 결과 = SetByCaller.AmmoCost(displayed 발수, 기본 1.0) × 슬롯 AmmoScale
// GE 측 Modifier Op은 Subtract로 사용하거나 Coefficient를 -1로 두어 차감 방향을 결정
UCLASS()
class PROJECTR_API UPRMMC_AmmoCost : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UPRMMC_AmmoCost();

	/*~ UGameplayModMagnitudeCalculation Interface ~*/
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

protected:
	// 어느 슬롯의 AmmoScale을 단가로 사용할지 결정. 디자이너가 BP에서 Primary/Secondary용으로 분기 생성
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Ammo")
	EPRAmmoType TargetAmmoType = EPRAmmoType::Primary;
};
