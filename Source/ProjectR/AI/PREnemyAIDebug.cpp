// Copyright ProjectR. All Rights Reserved.

#include "PREnemyAIDebug.h"

#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY(LogPREnemyAI);

namespace
{
	static TAutoConsoleVariable<int32> CVarPREnemyAIDebugPresentation(
		TEXT("pr.EnemyAI.DebugPresentation"),
		0,
		TEXT("전투 이동 표현 적용/해제 로그를 출력한다.\n0: 비활성\n1: 활성"),
		ECVF_Default);

	static TAutoConsoleVariable<int32> CVarPREnemyAIDebugAttackPressure(
		TEXT("pr.EnemyAI.DebugAttackPressure"),
		0,
		TEXT("attack_pressure 증가/리셋 로그를 출력한다.\n0: 비활성\n1: 활성"),
		ECVF_Default);

	static TAutoConsoleVariable<int32> CVarPREnemyAIDebugPattern(
		TEXT("pr.EnemyAI.DebugPattern"),
		0,
		TEXT("패턴 선택/전투 이동 의도 로그를 출력한다.\n0: 비활성\n1: 활성"),
		ECVF_Default);
}

bool PREnemyAIDebug::IsPresentationLogEnabled()
{
	return CVarPREnemyAIDebugPresentation.GetValueOnAnyThread() > 0;
}

bool PREnemyAIDebug::IsAttackPressureLogEnabled()
{
	return CVarPREnemyAIDebugAttackPressure.GetValueOnAnyThread() > 0;
}

bool PREnemyAIDebug::IsPatternLogEnabled()
{
	return CVarPREnemyAIDebugPattern.GetValueOnAnyThread() > 0;
}
