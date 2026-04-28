// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "PRWeaponModDataAsset.generated.h"

// 무기 Mod 1종의 초기 게이지, 스택, 어빌리티 연결 정보를 담는다
UCLASS(BlueprintType)
class PROJECTR_API UPRWeaponModDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPRWeaponModDataAsset();

public:
	// Mod 분류와 호환성 검증에 사용할 태그 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	FGameplayTagContainer ModTags;

	// 슬롯 초기화 시 사용할 최대 게이지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon", meta = (ClampMin = "0.0"))
	float MaxGauge = 100.0f;

	// 슬롯 초기화 시 사용할 시작 스택
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon", meta = (ClampMin = "0"))
	int32 InitialStack = 1;

	// 활성 슬롯 장착 시 부여할 Mod 어빌리티 목록. 순서대로 GiveAbility
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon")
	TArray<FPRAbilityEntry> EquippedAbilities;

	// 활성 슬롯 장착 시 부여 직후 자동 적용할 Mod Startup GE 목록
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon")
	TArray<FPREffectEntry> EquippedEffects;
};
