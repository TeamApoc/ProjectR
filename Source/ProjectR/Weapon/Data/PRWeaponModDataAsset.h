// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PRWeaponModDataAsset.generated.h"

class UPRAbilitySet;

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
	float MaxGauge = 0.0f;

	// 슬롯 초기화 시 사용할 시작 스택
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon", meta = (ClampMin = "0"))
	int32 InitialStack = 0;

	// 무기 전환 뒤에도 효과를 유지할지 결정한다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	bool bPersistEffectAfterWeaponSwitch = false;

	// 장착 시 함께 부여할 Mod 어빌리티 세트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TObjectPtr<UPRAbilitySet> EquippedModAbilitySet = nullptr;

	// 특수 사격 상태에서 기본 발사를 대체할 어빌리티 세트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TObjectPtr<UPRAbilitySet> OverrideFireAbilitySet = nullptr;
};
