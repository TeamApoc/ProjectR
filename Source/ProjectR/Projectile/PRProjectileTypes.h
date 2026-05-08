// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
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

// 투사체 예측 경로 시뮬레이션 종료 사유
UENUM(BlueprintType)
enum class EPRPreviewEndReason : uint8
{
	// 정상 진행 중 또는 미산출 상태
	None,
	// 첫 착탄으로 막힘. 이 지점에서 표시 종료
	HitBlocking,
	// MaxSimTime 도달
	TimeExceeded,
	// MaxDistance 도달
	DistanceExceeded,
};

// 투사체 예측 경로 산출에 필요한 발사 파라미터 묶음. 무기/어빌리티가 채워서 컴포넌트에 주입
USTRUCT(BlueprintType)
struct FPRProjectilePreviewParams
{
	GENERATED_BODY()

	// 초기 속력(cm/s). 기준 컴포넌트의 ForwardVector에 곱해져 초기 속도 벡터 구성
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview", meta = (ClampMin = "0.0"))
	float InitialSpeed = 3000.f;

	// 중력 배율. 1.0이면 월드 기본 중력, 0.0이면 직선 비행. UProjectileMovementComponent의 ProjectileGravityScale와 1:1 대응
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview")
	float GravityScale = 1.f;

	// 최대 시뮬레이션 시간(초). 이 시간을 넘으면 산출 종료. 첫 착탄이 먼저 발생하면 그 시점에 종료
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview", meta = (ClampMin = "0.0"))
	float MaxSimTime = 3.f;

	// 최대 비행 거리(cm). 누적 거리가 이 값을 초과하면 산출 종료. 0이면 거리 제한 없음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview", meta = (ClampMin = "0.0"))
	float MaxDistance = 5000.f;

	// 시뮬레이션 스텝 주기(초). 작을수록 정밀도 상승, 비용 증가. 권장 0.016 ~ 0.033
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview", meta = (ClampMin = "0.001"))
	float SimFrequency = 0.0166f;

	// 충돌 검사 반지름(cm). 0이면 라인 트레이스, 양수면 스피어 트레이스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview", meta = (ClampMin = "0.0"))
	float CollisionRadius = 5.f;

	// 샘플 포인트 사이의 최소 거리(cm). 다운샘플링 기준. 너무 작으면 포인트 과다, 너무 크면 굴곡 표현 부족
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview", meta = (ClampMin = "1.0"))
	float SampleSpacing = 50.f;

	// 표시할 최대 포인트 수. 출력 배열 상한 보호. 0이면 무제한
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview", meta = (ClampMin = "0"))
	int32 MaxSamplePoints = 64;

	// 충돌 채널. 무기 투사체 채널과 일치시켜 실제 비행과 동일한 대상에 충돌 처리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Projectile|Preview")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};

// 매 틱 산출되는 궤적 결과 캐시. 외부에서 조회 가능
USTRUCT(BlueprintType)
struct FPRProjectilePreviewResult
{
	GENERATED_BODY()

	// 다운샘플링 후 최종 표시에 사용된 월드 좌표 배열. 시작점에서 종료점까지
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Projectile|Preview")
	TArray<FVector> SamplePoints;

	// 시뮬레이션 종료 사유
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Projectile|Preview")
	EPRPreviewEndReason EndReason = EPRPreviewEndReason::None;

	// 종료 지점의 충돌 정보. EndReason이 HitBlocking일 때만 유효
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Projectile|Preview")
	FHitResult EndHit;

	// 산출에 걸린 총 시뮬레이션 시간(초)
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Projectile|Preview")
	float ElapsedSimTime = 0.f;
};
