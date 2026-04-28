// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PRProjectileBase.generated.h"

class UPRProjectileMovementComponent;

UENUM()
enum class EPRProjectileRole : uint8
{
	Predicted,
	Auth,
};

UCLASS()
class PROJECTR_API APRProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	APRProjectileBase();

	/*~ AActor Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 스폰 직후 1회 호출. 역할/ID 부여 (FinishSpawning 이전 호출 권장)
	void InitializeProjectile(EPRProjectileRole InRole, uint32 InProjectileId);

	// 서버 권위 투사체를 클라이언트 발사 시점까지 전진. HasAuthority() && bUseFastForward인 경우만 실행
	void ApplyFastForward(float ForwardSeconds);

	// Predicted-Auth 양방향 링크
	void LinkCounterpart(APRProjectileBase* InCounterpart);

	// 식별자 접근
	uint32 GetProjectileId() const { return ProjectileId; }

	// 역할 접근
	EPRProjectileRole GetProjectileRole() const { return ProjectileRole; }

	// Predicted 여부
	bool IsPredicted() const { return ProjectileRole == EPRProjectileRole::Predicted; }

	// 링크된 카운터파트 조회. Predicted면 Auth, Auth면 Predicted
	APRProjectileBase* GetLinkedCounterpart() const { return LinkedCounterpart.Get(); }

protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	// ProjectileId 리플리케이션 콜백. 소유 클라이언트에서만 호출됨 (COND_OwnerOnly)
	UFUNCTION()
	void OnRep_ProjectileId();

	// 소유 클라이언트에서 Auth 액터 도착 시 매니저의 Predicted와 매칭하여 링크
	void TryLinkToPredictedOnClient();

private:
	void DrawDebugs(float DeltaSeconds);
	
protected:
	// Fast-Forward 사용 여부. false면 ApplyFastForward 호출 시 무시
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Prediction")
	bool bUseFastForward = true;
	
	// ====== Components =====
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPRProjectileMovementComponent> ProjectileMovementComponent;
	
private:
	// 투사체 식별자. Auth 액터에 한해 소유 클라이언트로만 리플리케이트
	UPROPERTY(ReplicatedUsing = OnRep_ProjectileId)
	uint32 ProjectileId = 0;

	// 본 인스턴스의 역할 (네트워크 리플리케이트 대상 아님. 인스턴스 단위 결정)
	EPRProjectileRole ProjectileRole = EPRProjectileRole::Predicted;

	// 카운터파트 약참조 (Predicted-Auth 상호 링크)
	TWeakObjectPtr<APRProjectileBase> LinkedCounterpart;
	
#if WITH_EDITORONLY_DATA
	// 디버그 스피어 드로우 활성화 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Debug")
	bool bDrawDebugSphere = false;

	// 디버그 스피어 반지름 (cm)
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Debug", meta = (EditCondition = "bDrawDebugSphere", ClampMin = "1.0"))
	float DebugSphereRadius = 10.f;

	// 드로우 주기 (초). 0이면 매 틱
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Debug", meta = (EditCondition = "bDrawDebugSphere", ClampMin = "0.0"))
	float DebugDrawInterval = 0.f;

	// 드로우 지속 시간
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Debug", meta = (EditCondition = "bDrawDebugSphere", ClampMin = "0.0"))
	float DebugDrawLifetime = 1.f;
	
	// Predicted 역할 색상
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Debug", meta = (EditCondition = "bDrawDebugSphere"))
	FColor DebugColorPredicted = FColor::Yellow;

	// Auth 역할 색상
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Debug", meta = (EditCondition = "bDrawDebugSphere"))
	FColor DebugColorAuth = FColor::Green;

	// 마지막 드로우 이후 경과 시간 누적.
	float DebugDrawAccumulator = 0.f;
#endif
	
};
