// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "PRPlaygroundCheatManager.generated.h"

class UPRCheatHandler;

/**
 * 플레이그라운드 디버그용 치트 매니저
 * Exec 진입점만 담당하며 실제 작업은 PRPlayerController에 등록된 PRCheatHandler가 서버 권위로 처리
 */
UCLASS(Abstract)
class PROJECTR_API UPRPlaygroundCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	/*~ UPRPlaygroundCheatManager Interface ~*/
	// 현재 레벨을 동일 맵으로 재시작. 방장(서버 권한)에서만 동작
	UFUNCTION(Exec)
	void PR_Restart();

	// 자신만 다시 스폰. PRCheatHandler의 Server RPC로 라우팅
	UFUNCTION(Exec)
	void PR_Respawn();

	// 주무기와 보조무기 탄창·예비탄을 최대치로 충전. PRCheatHandler의 Server RPC로 라우팅
	UFUNCTION(Exec)
	void PR_FillAmmo();

	// 무한 모드 GE 토글. 1이면 적용, 0이면 회수. PRCheatHandler의 Server RPC로 라우팅
	UFUNCTION(Exec)
	void PR_InfiniteMode(int32 bEnable);

private:
	// 본인 PlayerController에 등록된 PRCheatHandler 조회
	UPRCheatHandler* GetCheatHandler() const;
};
