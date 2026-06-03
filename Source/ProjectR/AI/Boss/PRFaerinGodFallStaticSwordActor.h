// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AI/Boss/PRBossPatternActor.h"
#include "UObject/SoftObjectPath.h"
#include "PRFaerinGodFallStaticSwordActor.generated.h"

class UPRFaerinGodFallDataAsset;
class UNiagaraSystem;
class UStaticMeshComponent;

// God Fall Rig의 bone 위치에서 전환된 StaticMesh 검의 현재 상태다.
UENUM(BlueprintType)
enum class EPRFaerinGodFallStaticSwordState : uint8
{
	None					UMETA(DisplayName = "None"),
	SpawnedFromRigBone		UMETA(DisplayName = "Spawned From Rig Bone"),
	EntryDiving				UMETA(DisplayName = "Entry Diving"),
	EntryDiveReturning		UMETA(DisplayName = "Entry Dive Returning"),
	Charging				UMETA(DisplayName = "Charging"),
	Charged					UMETA(DisplayName = "Charged"),
	MovingToTargetOverhead	UMETA(DisplayName = "Moving To Target Overhead"),
	Telegraph				UMETA(DisplayName = "Telegraph"),
	Dropping				UMETA(DisplayName = "Dropping"),
	Impact					UMETA(DisplayName = "Impact"),
	Returning				UMETA(DisplayName = "Returning"),
	Cancelled				UMETA(DisplayName = "Cancelled")
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPRFaerinGodFallSwordChargeFinishedSignature, class APRFaerinGodFallStaticSwordActor*);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRFaerinGodFallSwordAssignedAttackFinishedSignature, class APRFaerinGodFallStaticSwordActor*);

// God Fall 변환 이후 개별 검의 충전, 1회 돌진, 원위치 복귀를 수행하는 StaticMesh 검 Actor다.
UCLASS(Blueprintable)
class PROJECTR_API APRFaerinGodFallStaticSwordActor : public APRBossPatternActor
{
	GENERATED_BODY()

public:
	APRFaerinGodFallStaticSwordActor();

	/*~ AActor Interface ~*/
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ APRBossPatternActor Interface ~*/
	virtual void CancelPatternActor() override;
	virtual bool ShouldCancelOnBossPhaseTransition() const override { return false; }

	// God Fall Rig bone에서 얻은 위치와 전투 데이터를 주입해 StaticSword를 초기화한다.
	void InitializeGodFallStaticSword(APRBossBaseCharacter* InOwnerBoss,
		AActor* InPatternTarget,
		UPRFaerinGodFallDataAsset* InGodFallData,
		int32 InSwordIndex,
		FName InSourceBoneName,
		const FTransform& InInitialTransform);

	// 이 검을 원위치에서 충전 상태로 전환한다.
	void BeginCharging(float ChargeSeconds);

	// God Fall entry 하강에 맞춰 아래로 한 번 돌진한 뒤 자기 위치로 복귀한다.
	bool StartEntryDive(float DiveDistance, float DiveSeconds, float ReturnSeconds, float ChargeSecondsAfterReturn);

	// 충전 완료 상태에서 지정 target에게 1회 공격을 시작한다.
	bool StartAssignedAttack(AActor* InAssignedTarget, float InWarningSeconds, float InOverheadMoveSeconds);

	// 현재 검이 새로운 target 배정을 받을 수 있는지 반환한다.
	bool CanStartAssignedAttack() const { return SwordState == EPRFaerinGodFallStaticSwordState::Charged; }

	// 현재 StaticSword 상태를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	EPRFaerinGodFallStaticSwordState GetSwordState() const { return SwordState; }

	// Rig에서 읽어 온 source bone 이름을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	FName GetSourceBoneName() const { return SourceBoneName; }

	// Rig bone 배열에서 이 검이 차지하는 index를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	int32 GetSwordIndex() const { return SwordIndex; }

	// StaticSword가 기억하는 원위치를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	FVector GetHomeLocation() const { return HomeLocation; }

public:
	FPRFaerinGodFallSwordChargeFinishedSignature OnChargeFinished;
	FPRFaerinGodFallSwordAssignedAttackFinishedSignature OnAssignedAttackFinished;

protected:
	/*~ AActor Interface ~*/
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 상태 복제 시 BP visual에 상태 변경을 알린다.
	UFUNCTION()
	void OnRep_SwordState();

	UFUNCTION()
	void OnRep_GodFallData();

	// 모든 클라이언트에서 impact visual을 재생한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastGodFallSwordImpact(FVector_NetQuantize ImpactLocation,
		FRotator ImpactRotation,
		UNiagaraSystem* NiagaraSystem,
		FVector Scale,
		float LifeSeconds);

	// 모든 클라이언트에서 impact 경고 visual 예약을 시작한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastScheduleGodFallSwordImpactWarning(FVector_NetQuantize ImpactLocation,
		FRotator ImpactRotation,
		FSoftObjectPath NiagaraSystemPath,
		FVector Scale,
		float LifeSeconds,
		float SpawnServerWorldTimeSeconds);

	// 모든 클라이언트에서 취소 visual을 재생한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastGodFallSwordCancelled();

	// StaticSword 상태가 바뀔 때 BP visual에 전달한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	void BP_OnGodFallSwordStateChanged(EPRFaerinGodFallStaticSwordState NewState);

	// StaticSword가 impact 판정을 수행한 직후 BP visual에 전달한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	void BP_OnGodFallSwordImpact();

	// StaticSword가 강제 취소될 때 BP visual에 전달한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	void BP_OnGodFallSwordCancelled();

private:
	void SetSwordState(EPRFaerinGodFallStaticSwordState NewState);
	void ClearSwordTimers();
	void FinishCharging();
	void FinishEntryDiveDown();
	void FinishEntryDiveReturn();
	void FinishMoveToTargetOverhead();
	void BeginTelegraph();
	void BeginDropping();
	void FinishDrop();
	void BeginReturning();
	void FinishReturning();
	void UpdateSegmentMovement(float DeltaSeconds);
	void UpdateTargetOverheadMovement(float DeltaSeconds);
	bool IsValidAssignedTarget(AActor* CandidateTarget) const;
	void UpdateClientSwordPresentation(float DeltaSeconds);
	void UpdateClientSegmentMovement(float DeltaSeconds);
	void UpdateClientTargetOverheadMovement(float DeltaSeconds);
	bool RefreshAssignedAttackLocations();
	bool ResolveAssignedAttackLocations(FVector& OutOverheadLocation, FVector& OutGroundLocation) const;
	bool ResolveAssignedImpactLocation(FVector& OutGroundLocation) const;
	bool ProjectTargetLocationToGround(const FVector& TargetLocation, FVector& OutGroundLocation) const;
	void ApplySwordPresentationLocation(const FVector& Location);
	void SpawnNiagaraAtLocationLocal(UNiagaraSystem* NiagaraSystem,
		const FVector& Location,
		const FRotator& Rotation,
		const FVector& Scale,
		float LifeSeconds) const;
	void CaptureMeshBaselineTransform();
	void ResetSwordVisualTransform();
	void UpdateSwordVisualPresentation(float DeltaSeconds);
	bool ShouldTickSwordVisual() const;
	bool ShouldTickSwordMovement() const;
	void RefreshSwordTickEnabled();
	FVector ResolveHoverLocationDelta() const;
	void ResolveChargeShakeDelta(FVector& OutLocationDelta, FRotator& OutRotationDelta) const;
	FRotator ResolveImpactSlantRotationDelta() const;
	float ResolveImpactSlantAlpha() const;
	void ScheduleImpactWarning();
	void ScheduleImpactWarningLocal(const FVector& ImpactLocation,
		float DelaySeconds,
		const FRotator& ImpactRotation,
		const FSoftObjectPath& NiagaraSystemPath,
		const FVector& Scale,
		float LifeSeconds);
	void SpawnImpactWarningLocal(const FVector& ImpactLocation,
		const FRotator& ImpactRotation,
		const FSoftObjectPath& NiagaraSystemPath,
		const FVector& Scale,
		float LifeSeconds);
	float ResolveServerWorldTimeSeconds() const;
	void StartClientSwordPresentationSegment(EPRFaerinGodFallStaticSwordState NewState,
		const FVector& StartLocation,
		const FVector& TargetLocation,
		float DurationSeconds);
	void StartClientSwordTargetOverhead(AActor* AssignedTarget,
		const FVector& InitialOverheadLocation,
		float MoveDurationSeconds,
		float MoveAcceleration);
	void SetClientSwordPresentationLocation(const FVector& Location);
	void ApplyImpactDamage();

	// GodFall 검의 고정 구간 위치를 클라이언트와 맞춘다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetSwordPresentationLocation(FVector Location);

	// GodFall 검의 고정 목표 이동 구간을 클라이언트가 로컬 보간하도록 시작한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartSwordPresentationSegment(EPRFaerinGodFallStaticSwordState NewState,
		FVector StartLocation,
		FVector TargetLocation,
		float DurationSeconds);

	// GodFall 검의 플레이어 머리 위 추적 구간을 클라이언트가 로컬 보간하도록 시작한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartSwordTargetOverhead(AActor* AssignedTarget,
		FVector InitialOverheadLocation,
		float MoveDurationSeconds,
		float MoveAcceleration);

protected:
	// StaticSword의 실제 표시와 위치 복제 기준이 되는 mesh component다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	TObjectPtr<UStaticMeshComponent> StaticSwordMeshComponent;

	// 현재 StaticSword 상태다.
	UPROPERTY(ReplicatedUsing = OnRep_SwordState, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	EPRFaerinGodFallStaticSwordState SwordState = EPRFaerinGodFallStaticSwordState::None;

	// Rig bone 배열에서 이 검이 차지하는 index다.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	int32 SwordIndex = INDEX_NONE;

	// 이 검의 스폰 기준이 된 rig bone 이름이다.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	FName SourceBoneName = NAME_None;

	// StaticSword로 변환된 시점의 원위치다.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	FVector HomeLocation = FVector::ZeroVector;

private:
	UPROPERTY(ReplicatedUsing = OnRep_GodFallData)
	TObjectPtr<UPRFaerinGodFallDataAsset> GodFallData;

	FTimerHandle ChargeTimerHandle;
	FTimerHandle TelegraphTimerHandle;
	FTimerHandle ImpactHoldTimerHandle;
	FTimerHandle ImpactWarningTimerHandle;

	FVector SegmentStartLocation = FVector::ZeroVector;
	FVector SegmentTargetLocation = FVector::ZeroVector;
	FVector AssignedOverheadLocation = FVector::ZeroVector;
	FVector AssignedImpactLocation = FVector::ZeroVector;
	FTransform MeshBaselineRelativeTransform = FTransform::Identity;
	FVector ClientSegmentStartLocation = FVector::ZeroVector;
	FVector ClientSegmentTargetLocation = FVector::ZeroVector;
	FVector ClientAssignedOverheadLocation = FVector::ZeroVector;
	float SegmentElapsedSeconds = 0.0f;
	float SegmentDurationSeconds = 0.0f;
	float EntryDiveReturnSeconds = 0.0f;
	float EntryDiveChargeSecondsAfterReturn = 0.0f;
	float OverheadMoveElapsedSeconds = 0.0f;
	float OverheadMoveDurationSeconds = 0.0f;
	float OverheadMoveSpeed = 0.0f;
	float OverheadMoveAcceleration = 0.0f;
	float ClientSegmentElapsedSeconds = 0.0f;
	float ClientSegmentDurationSeconds = 0.0f;
	float ClientOverheadMoveElapsedSeconds = 0.0f;
	float ClientOverheadMoveDurationSeconds = 0.0f;
	float ClientOverheadMoveSpeed = 0.0f;
	float ClientOverheadMoveAcceleration = 0.0f;
	float AssignedWarningSeconds = 0.0f;
	float VisualElapsedSeconds = 0.0f;
	float VisualStateElapsedSeconds = 0.0f;
	TWeakObjectPtr<AActor> ClientAssignedTarget;
	bool bHasAssignedAttackLocation = false;
	bool bClientSwordPresentationActive = false;
	bool bMeshBaselineCaptured = false;
	bool bImpactWarningSpawned = false;
	EPRFaerinGodFallStaticSwordState ClientPresentationState = EPRFaerinGodFallStaticSwordState::None;
};
