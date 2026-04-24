// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRFireAbilityTypes.h"
#include "Abilities/GameplayAbility.h"
#include "PRGA_Fire.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFire, Log, All);

/**
 * 플레이어 사격 어빌리티 베이스
 */
UCLASS(Abstract)
class PROJECTR_API UPRGA_Fire : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UPRGA_Fire();
	
public:
	// 총구 위치 반환
	virtual FVector GetMuzzleLocation() const;
	
	// 로컬 화면 Viewpoint 추출
	virtual FPRFireViewpoint GetFireViewpoint() const;
	
	// 히트 판정 수행(클라측).
	virtual FHitResult PerformFireTrace(const FPRFireViewpoint& View, float MaxTraceDistance) const;
	
	// per-shot 서버 보고 (unreliable). 유실 허용.
	UFUNCTION(Server, Unreliable)
	void Server_ReportShot(FPRFireShotPayload Payload);

	// 발사 루프에서 호출. 로컬 처리 로직과 서버 처리 로직 분기
	virtual void FireOneShot();
	
protected:
	// 서버가 샷을 확정 처리. GE 및 데미지 적용.
	virtual void ServerConfirmShot(const FPRFireShotPayload& Payload);
};
