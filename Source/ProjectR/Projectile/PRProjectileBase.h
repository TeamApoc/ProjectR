// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "PRProjectileTypes.h"
#include "PRProjectileBase.generated.h"

struct FGameplayEffectSpecHandle;
class UPRProjectileMovementComponent;
class USceneComponent;
class USphereComponent;

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
	
	// 스폰 직후 권위 투사체에 대해 GE 초기화
	void InitGameplayEffectSpec(const FGameplayEffectSpecHandle& InEffectSpec);

	// 서버가 직접 생성한 투사체의 초기 진행 방향과 속도를 설정한다.
	void SetProjectileInitialVelocity(const FVector& Direction, float SpeedOverride = 0.0f);

	// 서버가 직접 생성한 투사체의 Homing 타겟을 설정한다.
	void ConfigureProjectileHoming(USceneComponent* HomingTargetComponent, float HomingAcceleration);

	// 서버 권위 투사체를 클라이언트 발사 시점까지 전진. HasAuthority() && bUseFastForward인 경우만 실행
	void ApplyFastForward(float ForwardSeconds);

	// 서버에서 이벤트 발생 시 RepMovement를 Push. HasAuthority()인 경우만 실행
	void PushRepMovement(EPRRepMovementEvent Event);
	
	// 식별자 접근
	uint32 GetProjectileId() const { return ProjectileId; }

	// 역할 접근
	EPRProjectileRole GetProjectileRole() const { return ProjectileRole; }

	// Predicted 여부
	bool IsPredicted() const { return ProjectileRole == EPRProjectileRole::Predicted; }

	// 링크된 카운터파트 조회. Predicted면 Auth, Auth면 Predicted
	APRProjectileBase* GetLinkedCounterpart() const { return LinkedCounterpart.Get(); }

	UFUNCTION(BlueprintCallable)
	void DestroyProjectile();
	
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|Projectile")
	void OnProjectileDestroyed();
	
protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void OnAuthLinked(APRProjectileBase* AuthProjectile);
	
	// ProjectileId 리플리케이션 콜백. 소유 클라이언트에서만 호출됨 (COND_OwnerOnly)
	UFUNCTION()
	void OnRep_ProjectileId();

	// RepMovement OnRep 콜백. SimulatedProxy에서 이벤트별 처리
	UFUNCTION()
	void OnRep_RepMovement();

	// SimulatedProxy: Spawn 이벤트 처리. 언히든 + 위치 설정 + 시뮬 시작
	void HandleRepSpawn();

	// SimulatedProxy: Bounce/Detonation 이벤트 처리. 위치/속도 스냅
	void HandleRepCorrection();

	// 콜리전 히트 이벤트. 서버에서만 판정 처리
	UFUNCTION()
	virtual void OnSphereHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|Projectile")
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	UFUNCTION(BlueprintCallable)
	void ApplyEffectToTarget(AActor* TargetActor);
	
	UFUNCTION(BlueprintCallable)
	void ApplyEffectToTargetWithHit(AActor* TargetActor, const FHitResult& InHitResult);
	
private:
	// Predicted-Auth 양방향 링크
	void LinkCounterpart(APRProjectileBase* InCounterpart);
	
	// 소유 클라이언트에서 Auth 액터 도착 시 매니저의 Predicted와 매칭하여 링크
	void TryLinkToPredictedOnClient();
	
	void DrawDebugs(float DeltaSeconds);
	
protected:
	// Fast-Forward 사용 여부. false면 ApplyFastForward 호출 시 무시
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Prediction")
	bool bUseFastForward = true;

	// Auth가 앞에 있을 때 보간 속도 (따라잡기)
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Prediction", meta = (ClampMin = "0.0"))
	float SyncInterpSpeedCatchUp = 30.f;

	// Auth가 뒤에 있을 때 보간 속도 (감속). 작을수록 천천히 늦춰짐
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Projectile|Prediction", meta = (ClampMin = "0.0"))
	float SyncInterpSpeedSlowDown = 10.f;
	
	// ====== Components =====
	// 루트. ProjectileMovementComponent의 UpdatedComponent
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Projectile")
	TObjectPtr<USphereComponent> SphereComponent;

	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Projectile")
	TObjectPtr<UPRProjectileMovementComponent> ProjectileMovementComponent;
	
	UPROPERTY(BlueprintReadOnly)
	FGameplayEffectSpecHandle EffectSpecHandle;
	
private:
	// 투사체 식별자. Auth 액터에 한해 소유 클라이언트로만 리플리케이트
	UPROPERTY(ReplicatedUsing = OnRep_ProjectileId)
	uint32 ProjectileId = 0;

	// 이벤트 드리븐 이동 동기화.
	UPROPERTY(ReplicatedUsing = OnRep_RepMovement)
	FPRProjectileRepMovement RepMovement;

	// 본 인스턴스의 역할 (네트워크 리플리케이트 대상 아님. 인스턴스 단위 결정)
	EPRProjectileRole ProjectileRole = EPRProjectileRole::Auth; // Auth를 기본값으로 하여야 복제된 투사체 초기화시 Auth확인 가능

	// 카운터파트 약참조 (Predicted-Auth 상호 링크)
	TWeakObjectPtr<APRProjectileBase> LinkedCounterpart;
	
	bool bIsLinked = false;
	bool bIsRemoteProjectile = false;
	bool bShouldSyncToAuth = false;
	bool bHasRepSpawnHandled = false;
	
	TSet<TWeakObjectPtr<AActor>> HitActors;
	
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
