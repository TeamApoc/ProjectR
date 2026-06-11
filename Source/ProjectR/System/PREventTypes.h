// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "PREventTypes.generated.h"

class AActor;

enum class EPRWeaponSlotType : uint8;
/**
 * 이벤트 페이로드 베이스 구조체.
 * 실제 페이로드는 이 구조체를 상속받아 정의한 뒤 FInstancedStruct로 래핑해 발송한다.
 * 가상함수는 두지 않는다 (UHT 호환 및 단순 데이터 컨테이너 용도).
 */
USTRUCT(BlueprintType)
struct PROJECTR_API FPREventPayload
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRModActivationPayload : public FPREventPayload
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	bool bActivated = false;
	
	UPROPERTY(BlueprintReadWrite)
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRHUDMessageEventPayload : public FPREventPayload
{
	GENERATED_BODY()

	// HUD 메시지 표시 여부
	UPROPERTY(BlueprintReadWrite)
	bool bShow = false;

	// HUD에 표시할 안내 문구
	UPROPERTY(BlueprintReadWrite)
	FText Message;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRPlayerAttackTargetPayload : public FPREventPayload
{
	GENERATED_BODY()

	// 공격 주체 플레이어
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> Attacker = nullptr;

	// 실제 피해를 받은 대상
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> Target = nullptr;

	// 서버 피해 적용 시점
	UPROPERTY(BlueprintReadWrite)
	float ServerWorldTimeSeconds = 0.0f;

	// 피격 결과
	UPROPERTY(BlueprintReadWrite)
	FHitResult HitResult;
};
