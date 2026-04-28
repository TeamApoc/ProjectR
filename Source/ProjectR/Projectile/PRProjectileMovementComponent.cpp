// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRProjectileMovementComponent.h"


UPRProjectileMovementComponent::UPRProjectileMovementComponent()
{
	// 초기 속도 (cm/s). 100m/s = 유탄/로켓 수준. 히트스캔 대체 고속탄은 파생 BP에서 상향 조정
	InitialSpeed = 10000.f;

	// 최고 속도. 중력·공기저항 가속 시 속도 상한
	MaxSpeed = 10000.f;

	// 투사체 수명. ProjectileBase의 MaxLifetime과 별개로 PMC 이동을 제한하지 않음 (수명 관리는 Actor에서)
	ProjectileGravityScale = 0.3f;

	// 바운스 미사용 (기본). 유탄 등 파생 BP에서 활성화
	bShouldBounce = false;
	Bounciness = 0.3f;
	Friction = 0.3f;

	// Fast-Forward 시 큰 델타타임을 잘게 분할하여 중간 충돌 누락 방지
	// bForceSubStepping이 false면 TickComponent(ForwardPredictionTime) 단일 스텝으로 처리되어
	// 스폰 위치~전진 위치 사이 충돌을 모두 건너뜀
	bForceSubStepping = true;
	MaxSimulationTimeStep = 0.005f;
	MaxSimulationIterations = 16;

	// 이동 리플리케이션 비활성. 폭발 시점에만 위치 동기화
	SetIsReplicated(false);
}
