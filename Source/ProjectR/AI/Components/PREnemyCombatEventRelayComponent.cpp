// Copyright ProjectR. All Rights Reserved.

#include "PREnemyCombatEventRelayComponent.h"

UPREnemyCombatEventRelayComponent::UPREnemyCombatEventRelayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UPREnemyCombatEventRelayComponent::BeginPlay()
{
	Super::BeginPlay();

	// 새 적 AI 구조에서는 상태 이벤트를 직접 중계하지 않는다.
	// 기존 BP 서브오브젝트 참조가 끊기지 않도록 호환용 껍데기만 유지한다.
	bEventsBound = true;
}
