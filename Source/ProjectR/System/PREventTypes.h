// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PREventTypes.generated.h"

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

