// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PRStatRows.generated.h"

// DT_EnemyStats / DT_BossStats 공용 Row. 프로퍼티 이름이 Registry의 리플렉션 키
USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyStatRow : public FTableRowBase
{
	GENERATED_BODY()

	// 최대 체력. 초기화 경로에서 MaxHealth로 주입
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxHealth = 100.0f;

	// 이동 속도 배율 초기값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MovementSpeedMultiplier = 1.0f;

	// 최대 그로기 게이지
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxGroggyGauge = 100.0f;

	// 장갑 피해 배율 초기값 (1.0 = 감산 없음)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ArmorMultiplier = 1.0f;

	// 약점 피해 배율 초기값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float WeakpointMultiplier = 2.0f;
};

// DT_PlayerStats Row. 프로퍼티 이름이 Registry의 리플렉션 키
USTRUCT(BlueprintType)
struct PROJECTR_API FPRPlayerStatRow : public FTableRowBase
{
	GENERATED_BODY()

	// 최대 체력 초기값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxHealth = 100.0f;

	// 이동 속도 배율 초기값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MovementSpeedMultiplier = 1.0f;

	// 최대 스태미너 초기값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxStamina = 100.0f;

	// 초당 스태미너 회복량 초기값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float StaminaRegenRate = 25.0f;

	// 최대 Mod 게이지 초기값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxModGauge = 100.0f;

	// 주무기 탄창 기본 용량
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxPrimaryAmmo = 30.0f;

	// 보조무기 탄창 기본 용량
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxSecondaryAmmo = 6.0f;
};
