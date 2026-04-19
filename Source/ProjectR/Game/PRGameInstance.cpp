// Copyright ProjectR. All Rights Reserved.

#include "PRGameInstance.h"
#include "PRSessionSubsystem.h"

// ===== 초기화 =====

void UPRGameInstance::Init()
{
	Super::Init();
}

void UPRGameInstance::Shutdown()
{
	Super::Shutdown();
}

// ===== 세션 진입점 =====

void UPRGameInstance::HostSession(const FPRHostSessionParams& Params)
{
	UPRSessionSubsystem* Session = GetSubsystem<UPRSessionSubsystem>();
	if (!IsValid(Session))
	{
		return;
	}

	Session->StartHost(Params);
}

void UPRGameInstance::JoinSession(const FPRJoinSessionParams& Params)
{
	UPRSessionSubsystem* Session = GetSubsystem<UPRSessionSubsystem>();
	if (!IsValid(Session))
	{
		return;
	}

	Session->StartJoin(Params);
}

void UPRGameInstance::LeaveSession()
{
	UPRSessionSubsystem* Session = GetSubsystem<UPRSessionSubsystem>();
	if (!IsValid(Session))
	{
		return;
	}

	Session->EndSession();
}

// ===== 세이브 연동 (스켈레톤) =====

bool UPRGameInstance::LoadLocalCharacter(FName SlotName)
{
	// TODO: 세이브 시스템 구현 시 UGameplayStatics::LoadGameFromSlot 연동
	return false;
}

bool UPRGameInstance::SaveLocalCharacter(FName SlotName)
{
	// TODO: 세이브 시스템 구현 시 UGameplayStatics::SaveGameToSlot 연동
	return false;
}

void UPRGameInstance::ApplyRewardGrant(const FPRRewardGrant& Grant)
{
	// 즉시 지급. 경험치는 로컬 캐릭터에 바로 반영
	LocalCharacter.Experience += Grant.Experience;
	// 재화는 인벤토리 시스템 구현 후 반영 (현재는 스텁)
}
