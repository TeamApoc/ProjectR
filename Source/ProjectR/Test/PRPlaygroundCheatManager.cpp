// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRPlaygroundCheatManager.h"

#include "GameFramework/PlayerController.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Test/PRCheatHandler.h"


/*~ 맵 제어 ~*/

void UPRPlaygroundCheatManager::PR_Restart()
{
#if !UE_BUILD_SHIPPING
	APlayerController* PC = GetOuterAPlayerController();
	if (!IsValid(PC) || !PC->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("PR_Restart: 방장(서버 권한)에서만 실행 가능"));
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	// ServerTravel로 모든 연결된 클라이언트를 같은 맵으로 함께 이동
	World->ServerTravel(TEXT("?Restart"), false);
#endif
}


/*~ 핸들러 위임 ~*/

void UPRPlaygroundCheatManager::PR_Respawn()
{
#if !UE_BUILD_SHIPPING
	UPRCheatHandler* Handler = GetCheatHandler();
	if (!IsValid(Handler))
	{
		UE_LOG(LogTemp, Warning, TEXT("PR_Respawn: CheatHandler 없음. PC의 CheatHandlerClass 확인 필요"));
		return;
	}

	Handler->ServerCheatRespawn();
#endif
}

void UPRPlaygroundCheatManager::PR_FillAmmo()
{
#if !UE_BUILD_SHIPPING
	UPRCheatHandler* Handler = GetCheatHandler();
	if (!IsValid(Handler))
	{
		UE_LOG(LogTemp, Warning, TEXT("PR_FillAmmo: CheatHandler 없음. PC의 CheatHandlerClass 확인 필요"));
		return;
	}

	Handler->ServerCheatFillAmmo();
#endif
}

void UPRPlaygroundCheatManager::PR_InfiniteMode(int32 bEnable)
{
#if !UE_BUILD_SHIPPING
	UPRCheatHandler* Handler = GetCheatHandler();
	if (!IsValid(Handler))
	{
		UE_LOG(LogTemp, Warning, TEXT("PR_InfiniteMode: CheatHandler 없음. PC의 CheatHandlerClass 확인 필요"));
		return;
	}

	Handler->ServerCheatInfiniteMode(bEnable != 0);
#endif
}

void UPRPlaygroundCheatManager::PR_AddAttackPower(float Amount)
{
#if !UE_BUILD_SHIPPING
	UPRCheatHandler* Handler = GetCheatHandler();
	if (!IsValid(Handler))
	{
		UE_LOG(LogTemp, Warning, TEXT("PR_AddAttackPower: CheatHandler 없음. PC의 CheatHandlerClass 확인 필요"));
		return;
	}

	Handler->ServerCheatAddAttackPower(Amount);
#endif
}

void UPRPlaygroundCheatManager::PR_ResetAttackPower()
{
#if !UE_BUILD_SHIPPING
	UPRCheatHandler* Handler = GetCheatHandler();
	if (!IsValid(Handler))
	{
		UE_LOG(LogTemp, Warning, TEXT("PR_ResetAttackPower: CheatHandler 없음. PC의 CheatHandlerClass 확인 필요"));
		return;
	}

	Handler->ServerCheatResetAttackPower();
#endif
}

void UPRPlaygroundCheatManager::PR_Fly(int32 bEnable)
{
#if !UE_BUILD_SHIPPING
	UPRCheatHandler* Handler = GetCheatHandler();
	if (!IsValid(Handler))
	{
		UE_LOG(LogTemp, Warning, TEXT("PR_Fly: CheatHandler 없음. PC의 CheatHandlerClass 확인 필요"));
		return;
	}

	Handler->ServerCheatFly(bEnable != 0);
#endif
}


/*~ 내부 헬퍼 ~*/

UPRCheatHandler* UPRPlaygroundCheatManager::GetCheatHandler() const
{
	APRPlayerController* PC = Cast<APRPlayerController>(GetOuterAPlayerController());
	if (!IsValid(PC))
	{
		return nullptr;
	}

	return PC->GetCheatHandler();
}
