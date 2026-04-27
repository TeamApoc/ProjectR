// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "PREnemyCombatDataAsset.generated.h"

class UEnvQuery;

// EQS 기반 전투 이동 목표 선택 설정이다.
// Strafe, Reposition, Sprint 접근처럼 "어디로 움직일지"를 고르는 공용 설정으로 사용한다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyMoveQueryConfig
{
	GENERATED_BODY()

	// 이동 목표를 고를 EQS Query 자산이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|EQS")
	TObjectPtr<UEnvQuery> QueryTemplate = nullptr;

	// EQS 결과 중 어떤 방식으로 최종 위치를 고를지 정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|EQS")
	TEnumAsByte<EEnvQueryRunMode::Type> QueryRunMode = EEnvQueryRunMode::SingleResult;

	// 이동 중에도 현재 타겟을 계속 바라보는 문맥인지 나타낸다.
	// AIController / AnimBP가 Strafe BlendSpace, AimOffset 문맥을 유지할 때 참고한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bMaintainTargetFocus = true;

	// 이동 중 전투용 AimOffset을 유지할 문맥인지 나타낸다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bUseCombatAimOffset = true;
};

// 일반 적 AI가 공유하는 전투 데이터 베이스 자산이다.
// Soldier_Armored 같은 개별 적은 이 자산을 상속해 고유 공격 데이터를 추가한다.
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPREnemyCombatDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Combat_Strafe용 EQS 이동 설정이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement")
	FPREnemyMoveQueryConfig CombatStrafeConfig;

	// Combat_Reposition용 EQS 이동 설정이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement")
	FPREnemyMoveQueryConfig CombatRepositionConfig;
};
