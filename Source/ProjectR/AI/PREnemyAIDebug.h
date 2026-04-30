// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPREnemyAI, Log, All);

namespace PREnemyAIDebug
{
	// 전투 이동 표현 적용/해제 로그 활성 여부
	bool IsPresentationLogEnabled();

	// attack_pressure 증가/리셋 로그 활성 여부
	bool IsAttackPressureLogEnabled();

	// 패턴 선택/이동 의도 판단 로그 활성 여부
	bool IsPatternLogEnabled();
}
