// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once
#include "NativeGameplayTags.h"

// 프로젝트 전용 Native GameplayTag 선언 모음. 태그 추가 시 이 네임스페이스 안에 선언.
namespace PRGameplayTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(System_Test); // 시스템 테스트용 태그

	// Player Tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player); // 플레이어 관련 태그
}
