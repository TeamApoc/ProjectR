// Copyright ProjectR. All Rights Reserved.

#include "PRGroundBoxProjectileBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"
#include "ProjectR/Character/Enemy/Penitent/PRPenitentCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/ProjectR.h"

/*~ 초기화 ~*/

APRGroundBoxProjectileBase::APRGroundBoxProjectileBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	SetReplicateMovement(true);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	
	TraceStartPoint = CreateDefaultSubobject<USceneComponent>(TEXT("TraceStartPoint"));
	TraceStartPoint->SetupAttachment(Root);
	TraceStartPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 220.0f));
	
	DamageDetectionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageDetectionBox"));
	DamageDetectionBox->SetupAttachment(Root);
	DamageDetectionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DamageDetectionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	DamageDetectionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DamageDetectionBox->SetGenerateOverlapEvents(true);

	BreakableDetectionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BreakableDetectionBox"));
	BreakableDetectionBox->SetupAttachment(Root);
	BreakableDetectionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BreakableDetectionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	BreakableDetectionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	BreakableDetectionBox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	BreakableDetectionBox->SetGenerateOverlapEvents(true);

	WallMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMeshComponent"));
	WallMeshComponent->SetupAttachment(Root);
	WallMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WallMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	// 히트스캔 피격 판정
	WallMeshComponent->SetCollisionResponseToChannel(PRCollisionChannels::ECC_Combat, ECR_Block);
	WallMeshComponent->SetCollisionResponseToChannel(PRCollisionChannels::ECC_Projectile, ECR_Block);

	AmbientVFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("AmbientVFXComponent"));
	AmbientVFXComponent->SetupAttachment(Root);

	MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComponent"));
	MovementComponent->UpdatedComponent = Root;
	MovementComponent->InitialSpeed = 0.0f;
	MovementComponent->MaxSpeed = 0.0f;
	MovementComponent->ProjectileGravityScale = 0.0f;
	MovementComponent->bAutoActivate = false;
}

/*~ AActor Interface ~*/

void APRGroundBoxProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	// BP 기본 체력 초기화
	CurrentHealth = FMath::Max(MaxHealth, 0.0f);

	if (IsValid(DamageDetectionBox))
	{
		DamageDetectionBox->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnDamageDetectionBoxBeginOverlap);
	}

	if (IsValid(BreakableDetectionBox))
	{
		BreakableDetectionBox->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnBreakableDetectionBoxBeginOverlap);
	}
}

void APRGroundBoxProjectileBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bLaunched || bDestroyRequested)
	{
		return;
	}

	// 런치 중 지면 보간 부착
	SnapToGround(DeltaSeconds, false);
}

void APRGroundBoxProjectileBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SafetyLifeTimeTimerHandle);
	}

	ResetTargetCooldowns();

	Super::EndPlay(EndPlayReason);
}

/*~ IPRCombatInterface Interface ~*/

bool APRGroundBoxProjectileBase::ReceiveDamageContext(const FPRDamageAppliedContext& Context)
{
	if (!HasAuthority() || bDestroyRequested)
	{
		return false;
	}

	if (Context.FinalDamage <= 0.0f)
	{
		return false;
	}

	const float DamageAmount = Context.FinalDamage;
	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Max(CurrentHealth - DamageAmount, 0.0f);

	FPRDamageAppliedContext AppliedContext = Context;
	AppliedContext.HealthBeforeDamage = PreviousHealth;
	AppliedContext.HealthAfterDamage = CurrentHealth;
	AppliedContext.MaxHealth = FMath::Max(MaxHealth, 0.0f);

	// 체력 차감 이후 후처리
	OnPostDamageApplied(AppliedContext);

	return true;
}

void APRGroundBoxProjectileBase::OnPostDamageApplied(const FPRDamageAppliedContext& Context)
{
	if (!HasAuthority())
	{
		return;
	}

	if (Context.FinalDamage > 0.0f)
	{
		// 서버 피해 결과를 모든 클라이언트 연출 입력으로 전달
		MulticastHandleGroundBoxDamaged(Context.FinalDamage, Context.HealthAfterDamage);
	}

	if (CurrentHealth <= 0.0f || Context.HealthAfterDamage <= 0.0f)
	{
		// 체력 고갈에 따른 단일 소멸 경로
		RequestGroundBoxEnd();
	}
}

/*~ 외부 구동 ~*/

void APRGroundBoxProjectileBase::InitGroundBoxProjectile(const FPRGroundBoxLaunchParams& Params)
{
	if (!HasAuthority())
	{
		return;
	}

	SourceActor = Params.SourceActor;
	SourceTeam = UPRCombatStatics::GetActorTeam(SourceActor);
	DamageEffectSpecHandle = Params.DamageEffectSpec.IsValid()
		? Params.DamageEffectSpec
		: BuildDefaultDamageEffectSpec(SourceActor);

	const float InitialHealth = Params.OverrideMaxHealth > 0.0f
		? Params.OverrideMaxHealth
		: MaxHealth;

	// 스폰 입력값이 있으면 BP 기본 체력보다 우선 적용
	MaxHealth = FMath::Max(InitialHealth, 0.0f);
	CurrentHealth = MaxHealth;

	ResetTargetCooldowns();
	bDamageEnabled = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SafetyLifeTimeTimerHandle);
		if (MaxSafetyLifeTime > 0.0f)
		{
			// 명시 종료 누락에 대비한 안전 제거 예약
			World->GetTimerManager().SetTimer(
				SafetyLifeTimeTimerHandle,
				this,
				&ThisClass::HandleSafetyLifeTimeExpired,
				MaxSafetyLifeTime,
				false);
		}
	}

	MulticastHandleSpawned();

	if (!Params.LaunchDirection.IsNearlyZero() && Params.LaunchSpeed > 0.0f)
	{
		LaunchGroundBoxProjectile(Params.LaunchDirection, Params.LaunchSpeed);
	}
}

void APRGroundBoxProjectileBase::InitializeAttachedBarrier(APRPenitentCharacter* OwnerPenitent)
{
	if (!HasAuthority() || !IsValid(OwnerPenitent) || bDestroyRequested)
	{
		return;
	}

	FPRGroundBoxLaunchParams Params;
	Params.SourceActor = OwnerPenitent;
	InitGroundBoxProjectile(Params);

	bLaunched = false;
	SetActorTickEnabled(false);

	if (IsValid(MovementComponent))
	{
		// 부착 상태 이동 정지
		MovementComponent->StopMovementImmediately();
		MovementComponent->Deactivate();
	}
}

void APRGroundBoxProjectileBase::LaunchGroundBoxProjectile(const FVector& LaunchDirection, float LaunchSpeed)
{
	if (!HasAuthority() || bDestroyRequested)
	{
		return;
	}

	const FVector SafeDirection = LaunchDirection.GetSafeNormal();
	if (SafeDirection.IsNearlyZero() || LaunchSpeed <= 0.0f)
	{
		return;
	}

	ResetTargetCooldowns();

	bLaunched = true;
	bDamageEnabled = true;
	SetActorTickEnabled(true);

	if (IsValid(MovementComponent))
	{
		// 런치 입력을 서버 이동 상태로 확정
		MovementComponent->InitialSpeed = LaunchSpeed;
		MovementComponent->MaxSpeed = FMath::Max(MovementComponent->MaxSpeed, LaunchSpeed);
		MovementComponent->Velocity = SafeDirection * LaunchSpeed;
		MovementComponent->Activate(true);
	}

	SetActorRotation(SafeDirection.Rotation());
	// SnapToGround(0.0f, true);
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SafetyLifeTimeTimerHandle);
		if (MaxSafetyLifeTime > 0.0f)
		{
			// 명시 종료 누락에 대비한 안전 제거 예약
			World->GetTimerManager().SetTimer(
				SafetyLifeTimeTimerHandle,
				this,
				&ThisClass::HandleSafetyLifeTimeExpired,
				LaunchLifeTime,
				false);
		}
	}

	MulticastHandleLaunched();
}

void APRGroundBoxProjectileBase::ResetTargetCooldowns()
{
	TargetASCNextAllowedTimes.Reset();
	TargetActorNextAllowedTimes.Reset();
}

void APRGroundBoxProjectileBase::RequestGroundBoxEnd()
{
	if (!HasAuthority() || bDestroyRequested)
	{
		return;
	}

	DestroyGroundBox();
}

void APRGroundBoxProjectileBase::FinishFadeAndDestroy()
{
	if (!HasAuthority() || bDestroyRequested)
	{
		return;
	}

	DestroyGroundBox();
}

void APRGroundBoxProjectileBase::HandleSourceOwnerDead()
{
	if (!HasAuthority() || bDestroyRequested)
	{
		return;
	}

	// 소유자 사망 후 잔존 연출만 허용
	bDamageEnabled = false;
	bLaunched = false;
	SetActorTickEnabled(false);
	ResetTargetCooldowns();

	if (IsValid(MovementComponent))
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->Deactivate();
	}

	MulticastHandleFadeRequested();
}

void APRGroundBoxProjectileBase::HandleSafetyLifeTimeExpired()
{
	if (!HasAuthority() || bDestroyRequested)
	{
		return;
	}

	// 명시 종료 누락에 따른 안전 제거
	DestroyGroundBox();
}

/*~ 오버랩 처리 ~*/

void APRGroundBoxProjectileBase::OnDamageDetectionBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bDamageEnabled || bDestroyRequested)
	{
		return;
	}

	UAbilitySystemComponent* TargetAbilitySystemComponent =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);

	if (!CanApplyDamageToTarget(OtherActor, TargetAbilitySystemComponent))
	{
		return;
	}

	if (IsTargetOnCooldown(OtherActor, TargetAbilitySystemComponent))
	{
		return;
	}

	ApplyDamageToTarget(OtherActor, TargetAbilitySystemComponent, SweepResult);
}

void APRGroundBoxProjectileBase::OnBreakableDetectionBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bLaunched || bDestroyRequested)
	{
		return;
	}

	if (!bFromSweep)
	{
		// 초기 겹침 오인 방지
		return;
	}

	if (IsBlockingWallHit(SweepResult))
	{
		// 최소 구현의 환경 충돌 결과
		RequestGroundBoxEnd();
	}
}

/*~ 피해 처리 ~*/

bool APRGroundBoxProjectileBase::CanApplyDamageToTarget(AActor* TargetActor, UAbilitySystemComponent* TargetAbilitySystemComponent) const
{
	if (!IsValid(TargetActor) || TargetActor == this || TargetActor == SourceActor)
	{
		return false;
	}

	if (!IsValid(TargetAbilitySystemComponent))
	{
		return false;
	}

	const EPRTeam TargetTeam = UPRCombatStatics::GetActorTeam(TargetActor);
	if (SourceTeam != EPRTeam::Neutral && SourceTeam == TargetTeam)
	{
		return false;
	}

	const IPRCombatInterface* CombatTarget = Cast<IPRCombatInterface>(TargetActor);
	if (CombatTarget != nullptr && CombatTarget->IsDead())
	{
		return false;
	}

	return true;
}

void APRGroundBoxProjectileBase::ApplyDamageToTarget(AActor* TargetActor,
	UAbilitySystemComponent* TargetAbilitySystemComponent, const FHitResult& HitResult)
{
	if (!IsValid(TargetActor) || !IsValid(TargetAbilitySystemComponent) || !DamageEffectSpecHandle.IsValid())
	{
		return;
	}

	FGameplayEffectSpec* EffectSpec = DamageEffectSpecHandle.Data.Get();
	if (EffectSpec == nullptr)
	{
		return;
	}

	// 오버랩 지점을 대상 피해 컨텍스트로 전달
	EffectSpec->GetContext().AddHitResult(HitResult, true);
	TargetAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpec);

	SetTargetCooldown(TargetActor, TargetAbilitySystemComponent);
	MulticastHandleTargetHit(TargetActor, HitResult);
}

FGameplayEffectSpecHandle APRGroundBoxProjectileBase::BuildDefaultDamageEffectSpec(AActor* InSourceActor) const
{
	if (!IsValid(InSourceActor))
	{
		return FGameplayEffectSpecHandle();
	}

	UAbilitySystemComponent* SourceAbilitySystemComponent =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InSourceActor);
	if (!IsValid(SourceAbilitySystemComponent))
	{
		return FGameplayEffectSpecHandle();
	}

	if (!IsValid(DamageEffectClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GroundBoxProjectile] DamageEffectClass 설정 없음. Actor=%s"), *GetNameSafe(this));
		return FGameplayEffectSpecHandle();
	}

	FGameplayEffectContextHandle EffectContext = SourceAbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(InSourceActor);

	const FGameplayEffectSpecHandle SpecHandle = SourceAbilitySystemComponent->MakeOutgoingSpec(
		DamageEffectClass,
		1.0f,
		EffectContext);

	if (SpecHandle.IsValid())
	{
		// 직접 피해 SetByCaller
		SpecHandle.Data->SetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_Damage,
			FMath::Max(DefaultDamageAmount, 0.0f));

		SpecHandle.Data->SetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_GroggyDamage,
			FMath::Max(DefaultGroggyDamageAmount, 0.0f));
	}

	return SpecHandle;
}

bool APRGroundBoxProjectileBase::IsTargetOnCooldown(AActor* TargetActor,
	UAbilitySystemComponent* TargetAbilitySystemComponent) const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const float CurrentTime = World->GetTimeSeconds();
	if (IsValid(TargetAbilitySystemComponent))
	{
		const TWeakObjectPtr<UAbilitySystemComponent> TargetKey(TargetAbilitySystemComponent);
		if (const float* NextAllowedTime = TargetASCNextAllowedTimes.Find(TargetKey))
		{
			return *NextAllowedTime > CurrentTime;
		}
	}

	if (IsValid(TargetActor))
	{
		const TWeakObjectPtr<AActor> TargetKey(TargetActor);
		if (const float* NextAllowedTime = TargetActorNextAllowedTimes.Find(TargetKey))
		{
			return *NextAllowedTime > CurrentTime;
		}
	}

	return false;
}

void APRGroundBoxProjectileBase::SetTargetCooldown(AActor* TargetActor, UAbilitySystemComponent* TargetAbilitySystemComponent)
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const float NextAllowedTime = World->GetTimeSeconds() + FMath::Max(TargetCooldownTimeSeconds, 0.0f);
	if (IsValid(TargetAbilitySystemComponent))
	{
		TargetASCNextAllowedTimes.Add(TargetAbilitySystemComponent, NextAllowedTime);
		return;
	}

	if (IsValid(TargetActor))
	{
		TargetActorNextAllowedTimes.Add(TargetActor, NextAllowedTime);
	}
}

/*~ 이동 처리 ~*/

void APRGroundBoxProjectileBase::SnapToGround(float DeltaSeconds, bool bInstant)
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || GroundSnapTraceDistance <= 0.0f)
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PRGroundBoxSnapToGround), false, this);
	if (IsValid(SourceActor))
	{
		QueryParams.AddIgnoredActor(SourceActor);
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector TraceStart = TraceStartPoint->GetComponentLocation();
	const FVector TraceEnd = TraceStart - FVector::UpVector * GroundSnapTraceDistance;

	FHitResult GroundHit;
	if (!World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, PRCollisionChannels::ECC_Ground, QueryParams))
	{
		return;
	}

	if (GroundHit.ImpactNormal.Z < GroundNormalThreshold)
	{
		return;
	}

	const float TargetZ = GroundHit.ImpactPoint.Z;
	float NewZ = TargetZ;
	if (!bInstant && GroundSnapInterpSpeed > 0.0f)
	{
		// 지면 높이 보간 추적
		NewZ = FMath::FInterpTo(CurrentLocation.Z, TargetZ, DeltaSeconds, GroundSnapInterpSpeed);
		if (FMath::Abs(NewZ - TargetZ) <= GroundSnapTolerance)
		{
			// 미세 떨림 방지
			NewZ = TargetZ;
		}
	}

	// 수평 이동은 유지하고 높이만 지면에 보정
	const FVector SnappedLocation(CurrentLocation.X, CurrentLocation.Y, NewZ);
	SetActorLocation(SnappedLocation, false);
}

bool APRGroundBoxProjectileBase::IsBlockingWallHit(const FHitResult& HitResult) const
{
	AActor* HitActor = HitResult.GetActor();
	if (!IsValid(HitActor) || HitActor == this || HitActor == SourceActor)
	{
		return false;
	}

	if (HitResult.ImpactNormal.IsNearlyZero())
	{
		// 표면 방향 없는 환경 접촉 제외
		return false;
	}

	if (HitResult.ImpactNormal.Z >= GroundNormalThreshold)
	{
		// 바닥 계열 표면 제외
		return false;
	}

	return true;
}

/*~ 소멸 처리 ~*/

void APRGroundBoxProjectileBase::DestroyGroundBox()
{
	if (bDestroyRequested)
	{
		return;
	}

	bDestroyRequested = true;
	bDamageEnabled = false;
	bLaunched = false;
	SetActorTickEnabled(false);
	ResetTargetCooldowns();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SafetyLifeTimeTimerHandle);
	}

	if (IsValid(MovementComponent))
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->Deactivate();
	}

	MulticastHandleDestroyed();
	Destroy();
}

/*~ 네트워크 연출 ~*/

void APRGroundBoxProjectileBase::MulticastHandleSpawned_Implementation()
{
	OnGroundBoxSpawned.Broadcast(this);
}

void APRGroundBoxProjectileBase::MulticastHandleLaunched_Implementation()
{
	OnGroundBoxLaunched.Broadcast(this);
}

void APRGroundBoxProjectileBase::MulticastHandleTargetHit_Implementation(AActor* TargetActor, const FHitResult& HitResult)
{
	OnGroundBoxTargetHit.Broadcast(this, TargetActor, HitResult);
}

void APRGroundBoxProjectileBase::MulticastHandleGroundBoxDamaged_Implementation(float DamageAmount, float HealthAfterDamage)
{
	OnGroundBoxDamaged.Broadcast(this, DamageAmount, HealthAfterDamage);
}

void APRGroundBoxProjectileBase::MulticastHandleFadeRequested_Implementation()
{
	OnGroundBoxFadeRequested.Broadcast(this);
}

void APRGroundBoxProjectileBase::MulticastHandleDestroyed_Implementation()
{
	OnGroundBoxDestroyed.Broadcast(this);
}
