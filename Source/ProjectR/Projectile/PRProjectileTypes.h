// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRProjectileTypes.generated.h"

class APRProjectileBase;

DECLARE_DYNAMIC_DELEGATE_OneParam(FProjectileSpawnedDelegate, APRProjectileBase*, SpawnedProjectile);

// 투사체 이동 동기화 이벤트 종류
UENUM()
enum class EPRRepMovementEvent : uint8
{
	Spawn,       // 최초 스폰 (SimulatedProxy 언히든 + 시뮬 시작)
	Bounce,      // 바운스 발생 시 위치/속도 스냅
	Detonation,  // 폭발/착탄 확정
};

// 이벤트 드리븐 투사체 이동 동기화 페이로드. 서버가 이벤트 발생 시에만 Push
USTRUCT()
struct FPRProjectileRepMovement
{
	GENERATED_BODY()

	// 이벤트 발생 시점의 위치
	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	// 이벤트 발생 시점의 속도
	UPROPERTY()
	FVector Velocity = FVector::ZeroVector;

	// 이벤트 발생 시점의 회전
	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;

	// 이벤트 종류
	UPROPERTY()
	EPRRepMovementEvent Event = EPRRepMovementEvent::Spawn;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FPRProjectileRepMovement> : public TStructOpsTypeTraitsBase2<FPRProjectileRepMovement>
{
	enum { WithNetSerializer = true };
};

// 투사체 스폰 파라미터. Client/Server 양측에서 동일 ID로 매칭하기 위한 정보 묶음
USTRUCT(BlueprintType)
struct FPRProjectileSpawnInfo
{
	GENERATED_BODY()

	// 투사체 식별자. 0은 무효
	uint32 ProjectileId = 0;

	// 스폰할 투사체 클래스
	TSubclassOf<APRProjectileBase> ProjectileClass;

	// 스폰 위치
	FVector SpawnLocation = FVector::ZeroVector;

	// 스폰 회전
	FRotator SpawnRotation = FRotator::ZeroRotator;

	// 추가 스폰 옵션. 기본값은 호출자가 채워서 전달
	FActorSpawnParameters SpawnParameters;
};
