// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRProjectileBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemInterface.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "PRProjectileMovementComponent.h"
#include "PRProjectileManagerComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Combat/PRCombatInterface.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/Combat/PRDestructableInterface.h"
#include "ProjectR/System/PRRespawnSubsystem.h"
#include "ProjectR/Combat/PRDirectDamageReceiverInterface.h"

APRProjectileBase::APRProjectileBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	bReplicates = true;
	SetReplicatingMovement(false);

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SetRootComponent(SphereComponent);
	SphereComponent->SetSphereRadius(30.f);
	SphereComponent->SetCollisionProfileName(TEXT("Projectile"));
	SphereComponent->SetGenerateOverlapEvents(true);
	SphereComponent->OnComponentHit.AddDynamic(this, &APRProjectileBase::OnSphereHit);
	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &APRProjectileBase::OnSphereBeginOverlap);

	ProjectileMovementComponent = CreateDefaultSubobject<UPRProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = SphereComponent;
}

void APRProjectileBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	constexpr bool bUsePushModel = true;
	
	FDoRepLifetimeParams OwnerOnlyParams{COND_OwnerOnly, REPNOTIFY_Always, bUsePushModel};
	DOREPLIFETIME_WITH_PARAMS_FAST(APRProjectileBase, ProjectileId, OwnerOnlyParams);

	FDoRepLifetimeParams NoneParams{COND_None, REPNOTIFY_Always, bUsePushModel};
	DOREPLIFETIME_WITH_PARAMS_FAST(APRProjectileBase, RepMovement, NoneParams);
}

void APRProjectileBase::InitializeProjectile(EPRProjectileRole InRole, uint32 InProjectileId)
{
	ProjectileRole = InRole;
	ProjectileId = InProjectileId;
}

void APRProjectileBase::InitGameplayEffectSpec(const FGameplayEffectSpecHandle& InEffectSpec)
{
	EffectSpecHandle = InEffectSpec;
	if (EffectSpecHandle.Data)
	{
		InstigatorASC =  EffectSpecHandle.Data->GetEffectContext().GetInstigatorAbilitySystemComponent();	
	}
}

void APRProjectileBase::SetProjectileInitialVelocity(const FVector& Direction, float SpeedOverride)
{
	if (!IsValid(ProjectileMovementComponent))
	{
		return;
	}

	const FVector SafeDirection = Direction.GetSafeNormal();
	if (SafeDirection.IsNearlyZero())
	{
		return;
	}

	const float ResolvedSpeed = SpeedOverride > 0.0f
		? SpeedOverride
		: ProjectileMovementComponent->InitialSpeed;

	ProjectileMovementComponent->InitialSpeed = ResolvedSpeed;
	ProjectileMovementComponent->MaxSpeed = FMath::Max(ProjectileMovementComponent->MaxSpeed, ResolvedSpeed);
	ProjectileMovementComponent->Velocity = SafeDirection * ResolvedSpeed;
	SetActorRotation(SafeDirection.Rotation());
}

float APRProjectileBase::GetProjectileInitialSpeed() const
{
	// BP 기본 투사체 속도 조회
	return IsValid(ProjectileMovementComponent)
		? ProjectileMovementComponent->InitialSpeed
		: 0.0f;
}

void APRProjectileBase::ConfigureProjectileHoming(USceneComponent* HomingTargetComponent, float HomingAcceleration)
{
	if (!IsValid(ProjectileMovementComponent))
	{
		return;
	}

	if (!IsValid(HomingTargetComponent) || HomingAcceleration <= 0.0f)
	{
		ProjectileMovementComponent->bIsHomingProjectile = false;
		ProjectileMovementComponent->HomingTargetComponent = nullptr;
		return;
	}

	ProjectileMovementComponent->bIsHomingProjectile = true;
	ProjectileMovementComponent->HomingTargetComponent = HomingTargetComponent;
	ProjectileMovementComponent->HomingAccelerationMagnitude = HomingAcceleration;
}

void APRProjectileBase::ConfigureProjectileHomingSchedule(
	AActor* HomingTargetActor,
	float HomingAcceleration,
	float StartDelay,
	float Duration)
{
	if (!HasAuthority())
	{
		return;
	}

	PendingHomingSchedule.Reset();
	if (!IsValid(HomingTargetActor) || HomingAcceleration <= 0.0f)
	{
		return;
	}

	PendingHomingSchedule.HomingTargetActor = HomingTargetActor;
	PendingHomingSchedule.HomingAcceleration = FMath::Max(HomingAcceleration, 0.0f);
	PendingHomingSchedule.StartDelay = FMath::Max(StartDelay, 0.0f);
	PendingHomingSchedule.Duration = FMath::Max(Duration, 0.0f);
	++PendingHomingSchedule.Revision;
}

void APRProjectileBase::AddProjectileIgnoredActor(AActor* ActorToIgnore)
{
	if (!IsValid(ActorToIgnore))
	{
		return;
	}

	if (IsValid(SphereComponent))
	{
		SphereComponent->IgnoreActorWhenMoving(ActorToIgnore, true);
	}
}

void APRProjectileBase::ApplyFastForward(float ForwardSeconds)
{
	if (!HasAuthority() || !bUseFastForward || ForwardSeconds <= 0.f)
	{
		return;
	}

	if (!IsValid(ProjectileMovementComponent))
	{
		return;
	}

	// PMC를 강제 틱하여 투사체를 ForwardSeconds 만큼 전진. bForceSubStepping=true로 내부 서브스텝 분할 수행
	ProjectileMovementComponent->TickComponent(ForwardSeconds, LEVELTICK_All, nullptr);

	// 수명 보정. Fast-Forward로 소모된 시간만큼 남은 수명에서 차감
	const float RemainingLife = GetLifeSpan();
	if (RemainingLife > 0.f)
	{
		SetLifeSpan(FMath::Max(RemainingLife - ForwardSeconds, 0.1f));
	}
}

void APRProjectileBase::PushRepMovement(EPRRepMovementEvent Event)
{
	if (!HasAuthority())
	{
		return;
	}

	RepMovement.Event    = Event;
	RepMovement.Location = GetActorLocation();
	RepMovement.Rotation = GetActorRotation();
	RepMovement.Velocity = ProjectileMovementComponent->Velocity;
	if (Event == EPRRepMovementEvent::Spawn)
	{
		RepMovement.HomingSchedule = PendingHomingSchedule;
	}
	else
	{
		RepMovement.HomingSchedule.Reset();
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(APRProjectileBase, RepMovement, this);
	ForceNetUpdate();

	if (Event == EPRRepMovementEvent::Spawn)
	{
		StartProjectileHomingSchedule(RepMovement.HomingSchedule);
		if (RepMovement.HomingSchedule.IsEnabled())
		{
			MulticastStartProjectileHomingPresentation(
				RepMovement.HomingSchedule.HomingTargetActor.Get(),
				RepMovement.HomingSchedule.HomingAcceleration,
				RepMovement.HomingSchedule.StartDelay,
				RepMovement.HomingSchedule.Duration,
				RepMovement.HomingSchedule.Revision);
		}
	}
}

void APRProjectileBase::OnRep_RepMovement()
{
	switch (RepMovement.Event)
	{
	case EPRRepMovementEvent::Spawn:
		HandleRepSpawn();
		break;
	case EPRRepMovementEvent::Bounce:
	case EPRRepMovementEvent::Detonation:
		HandleRepCorrection();
		break;
	default:
		break;
	}
}

void APRProjectileBase::StartProjectileHomingSchedule(const FPRProjectileRepHomingSchedule& HomingSchedule)
{
	ClearProjectileHomingScheduleTimers();
	if (!HomingSchedule.IsEnabled())
	{
		return;
	}

	if (HomingSchedule.StartDelay <= 0.0f)
	{
		ApplyProjectileHomingScheduleStart(HomingSchedule);
		return;
	}

	GetWorldTimerManager().SetTimer(
		ProjectileHomingStartTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, HomingSchedule]()
		{
			ApplyProjectileHomingScheduleStart(HomingSchedule);
		}),
		HomingSchedule.StartDelay,
		false);
}

void APRProjectileBase::ApplyProjectileHomingScheduleStart(FPRProjectileRepHomingSchedule HomingSchedule)
{
	AActor* HomingTargetActor = HomingSchedule.HomingTargetActor.Get();
	USceneComponent* HomingTargetComponent = IsValid(HomingTargetActor)
		? HomingTargetActor->GetRootComponent()
		: nullptr;
	if (!IsValid(HomingTargetComponent))
	{
		return;
	}

	ConfigureProjectileHoming(HomingTargetComponent, HomingSchedule.HomingAcceleration);
	if (HomingSchedule.Duration <= 0.0f)
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		ProjectileHomingStopTimerHandle,
		this,
		&APRProjectileBase::StopProjectileHomingSchedule,
		HomingSchedule.Duration,
		false);
}

void APRProjectileBase::StopProjectileHomingSchedule()
{
	ConfigureProjectileHoming(nullptr, 0.0f);
}

void APRProjectileBase::ClearProjectileHomingScheduleTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ProjectileHomingStartTimerHandle);
		World->GetTimerManager().ClearTimer(ProjectileHomingStopTimerHandle);
	}
}

bool APRProjectileBase::IsRemoteAuthProjectilePresentation() const
{
	if (HasAuthority() || ProjectileRole != EPRProjectileRole::Auth)
	{
		return false;
	}

	const APlayerController* NetOwnerPlayerController = Cast<APlayerController>(GetNetOwner());
	return !IsValid(NetOwnerPlayerController) || !NetOwnerPlayerController->IsLocalController();
}

void APRProjectileBase::StartClientProjectileHomingPresentation(const FPRProjectileRepHomingSchedule& HomingSchedule)
{
	bIsRemoteProjectile = IsRemoteAuthProjectilePresentation();
	if (!bIsRemoteProjectile || !HomingSchedule.IsEnabled())
	{
		return;
	}

	if ((bClientProjectilePresentationActive || bHasPendingClientHomingPresentation)
		&& ClientPresentationRevision == HomingSchedule.Revision)
	{
		return;
	}

	ClientPresentationRevision = HomingSchedule.Revision;
	if (!bHasRepSpawnHandled)
	{
		PendingClientHomingPresentationSchedule = HomingSchedule;
		bHasPendingClientHomingPresentation = true;
		return;
	}

	bHasPendingClientHomingPresentation = false;
	PendingClientHomingPresentationSchedule.Reset();
	ClientPresentationHomingTarget = HomingSchedule.HomingTargetActor;
	ClientPresentationHomingAcceleration = FMath::Max(HomingSchedule.HomingAcceleration, 0.0f);
	ClientPresentationHomingStartDelay = FMath::Max(HomingSchedule.StartDelay, 0.0f);
	ClientPresentationHomingDuration = FMath::Max(HomingSchedule.Duration, 0.0f);
	ClientPresentationElapsedSeconds = 0.0f;
	ClientPresentationHomingElapsedSeconds = 0.0f;
	bClientProjectileHomingStarted = ClientPresentationHomingStartDelay <= UE_SMALL_NUMBER;
	bClientProjectilePresentationActive = true;

	ClientPresentationVelocity = RepMovement.Velocity;
	if (ClientPresentationVelocity.IsNearlyZero() && IsValid(ProjectileMovementComponent))
	{
		ClientPresentationVelocity = ProjectileMovementComponent->Velocity;
	}
	if (ClientPresentationVelocity.IsNearlyZero())
	{
		const float ResolvedSpeed = IsValid(ProjectileMovementComponent)
			? ProjectileMovementComponent->InitialSpeed
			: 0.0f;
		ClientPresentationVelocity = GetActorForwardVector() * ResolvedSpeed;
	}

	if (IsValid(ProjectileMovementComponent))
	{
		ProjectileMovementComponent->Deactivate();
		ConfigureProjectileHoming(nullptr, 0.0f);
	}

	SetActorEnableCollision(false);
	SetActorTickEnabled(true);
}

void APRProjectileBase::UpdateClientProjectileHomingPresentation(const float DeltaSeconds)
{
	if (!bClientProjectilePresentationActive || DeltaSeconds <= 0.0f)
	{
		return;
	}

	ClientPresentationElapsedSeconds += DeltaSeconds;
	if (!bClientProjectileHomingStarted
		&& ClientPresentationElapsedSeconds >= ClientPresentationHomingStartDelay)
	{
		bClientProjectileHomingStarted = true;
	}

	const bool bCanApplyHoming = bClientProjectileHomingStarted
		&& (ClientPresentationHomingDuration <= 0.0f
			|| ClientPresentationHomingElapsedSeconds < ClientPresentationHomingDuration);
	if (bCanApplyHoming)
	{
		if (AActor* HomingTargetActor = ClientPresentationHomingTarget.Get())
		{
			const FVector ToTarget = HomingTargetActor->GetActorLocation() - GetActorLocation();
			if (!ToTarget.IsNearlyZero())
			{
				ClientPresentationVelocity += ToTarget.GetSafeNormal()
					* ClientPresentationHomingAcceleration
					* DeltaSeconds;

				const float MaxSpeed = IsValid(ProjectileMovementComponent)
					? FMath::Max(ProjectileMovementComponent->MaxSpeed, ProjectileMovementComponent->InitialSpeed)
					: ClientPresentationVelocity.Size();
				if (MaxSpeed > UE_SMALL_NUMBER)
				{
					ClientPresentationVelocity = ClientPresentationVelocity.GetClampedToMaxSize(MaxSpeed);
				}
			}
		}

		if (ClientPresentationHomingDuration > 0.0f)
		{
			ClientPresentationHomingElapsedSeconds += DeltaSeconds;
		}
	}

	const FVector MoveDelta = ClientPresentationVelocity * DeltaSeconds;
	if (!MoveDelta.IsNearlyZero())
	{
		const FRotator NextRotation = ClientPresentationVelocity.GetSafeNormal().Rotation();
		SetActorLocationAndRotation(
			GetActorLocation() + MoveDelta,
			NextRotation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
	}
}

void APRProjectileBase::StopClientProjectileHomingPresentation()
{
	bClientProjectilePresentationActive = false;
	bClientProjectileHomingStarted = false;
	bHasPendingClientHomingPresentation = false;
	PendingClientHomingPresentationSchedule.Reset();
	ClientPresentationHomingTarget.Reset();
	ClientPresentationVelocity = FVector::ZeroVector;
	ClientPresentationHomingAcceleration = 0.0f;
	ClientPresentationHomingStartDelay = 0.0f;
	ClientPresentationHomingDuration = 0.0f;
	ClientPresentationElapsedSeconds = 0.0f;
	ClientPresentationHomingElapsedSeconds = 0.0f;
}

void APRProjectileBase::MulticastStartProjectileHomingPresentation_Implementation(
	AActor* HomingTargetActor,
	const float HomingAcceleration,
	const float StartDelay,
	const float Duration,
	const uint8 Revision)
{
	if (HasAuthority())
	{
		return;
	}

	FPRProjectileRepHomingSchedule HomingSchedule;
	HomingSchedule.HomingTargetActor = HomingTargetActor;
	HomingSchedule.HomingAcceleration = HomingAcceleration;
	HomingSchedule.StartDelay = StartDelay;
	HomingSchedule.Duration = Duration;
	HomingSchedule.Revision = Revision;
	StartClientProjectileHomingPresentation(HomingSchedule);
}

void APRProjectileBase::HandleRepSpawn()
{
	bIsRemoteProjectile = IsRemoteAuthProjectilePresentation();
	if (!bIsRemoteProjectile)
	{
		return;
	}
	
	SetActorLocation(RepMovement.Location);
	SetActorRotation(RepMovement.Rotation);

	if (IsValid(ProjectileMovementComponent))
	{
		ProjectileMovementComponent->Velocity = RepMovement.Velocity;
		if (RepMovement.HomingSchedule.IsEnabled() || bHasPendingClientHomingPresentation)
		{
			ProjectileMovementComponent->Deactivate();
		}
		else
		{
			ProjectileMovementComponent->Activate();
		}
	}

	// SimulatedProxy에서 숨김 처리 했던 상태를 복구
	SetActorHiddenInGame(false);
	// 원격 클라이언트는 서버 판정 결과만 따르고, 로컬 충돌로 시각용 궤적이 멈추지 않게 한다.
	SetActorEnableCollision(false);
	
	bHasRepSpawnHandled = true;
	if (bHasPendingClientHomingPresentation)
	{
		const FPRProjectileRepHomingSchedule HomingSchedule = PendingClientHomingPresentationSchedule;
		StopClientProjectileHomingPresentation();
		StartClientProjectileHomingPresentation(HomingSchedule);
	}
	else if (RepMovement.HomingSchedule.IsEnabled())
	{
		StartClientProjectileHomingPresentation(RepMovement.HomingSchedule);
	}
}

void APRProjectileBase::HandleRepCorrection()
{
	// SimulatedProxy가 받아야 할 Spawn이벤트가 곧바로 Correction이벤트로 덮어 씌워진 경우 Spawn을 처리
	if (GetLocalRole() == ROLE_SimulatedProxy && !bHasRepSpawnHandled)
	{
		HandleRepSpawn();
	}
	
	// Location, Rotation, Velocity를 서버 기준으로 동기화
	SetActorLocation(RepMovement.Location);
	SetActorRotation(RepMovement.Rotation);

	if (IsValid(ProjectileMovementComponent))
	{
		ProjectileMovementComponent->Velocity = RepMovement.Velocity;
	}

	if (RepMovement.Event == EPRRepMovementEvent::Detonation)
	{
		// 링크된 예측 투사체가 아직 남아 있는 경우
		if (LinkedCounterpart.IsValid())
		{
			// 예측 투사체 파괴
			LinkedCounterpart->Destroy();
		
			// 가시성 복구
			SetActorHiddenInGame(false);
			SetActorEnableCollision(true);
		}
		
		DestroyProjectile();
	}
}


void APRProjectileBase::DestroyProjectile()
{
	ClearProjectileHomingScheduleTimers();
	StopClientProjectileHomingPresentation();
	ConfigureProjectileHoming(nullptr, 0.0f);

	if (HasAuthority())
	{
		PushRepMovement(EPRRepMovementEvent::Detonation);
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);

		// 리플리케이션 채널이 Detonation RepMovement를 클라이언트에 전달할 시간 확보
		// 2.5 net frame: 1프레임 이상 생존 보장, 불필요하게 길지 않은 최솟값
		const UNetDriver* NetDriver = GetWorld()->GetNetDriver();
		const float NetTickRate = (NetDriver && NetDriver->GetNetServerMaxTickRate() > 0.f)
			? NetDriver->GetNetServerMaxTickRate()
			: 30.f;
		SetLifeSpan(2.5f / NetTickRate);
	}
	else
	{
		Destroy();
	}
	
	OnProjectileDestroyed();
}

void APRProjectileBase::OnProjectileDestroyed_Implementation()
{
}

bool APRProjectileBase::HasProjectileAuthority() const
{
	return ProjectileRole == EPRProjectileRole::Auth && HasAuthority();
}

void APRProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	// 투사체 궤적은 RepMovement 이벤트와 로컬 시뮬레이션으로 맞춘다.
	SetReplicatingMovement(false);

	// 발사자 충돌 제외
	AActor* IgnoredInstigator = GetInstigator();
	if (!IsValid(IgnoredInstigator))
	{
		if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
		{
			IgnoredInstigator = OwnerPawn;
		}
		else if (AController* OwnerController = Cast<AController>(GetOwner()))
		{
			IgnoredInstigator = OwnerController->GetPawn();
		}
	}

	if (IsValid(IgnoredInstigator))
	{
		if (IsValid(SphereComponent))
		{
			SphereComponent->IgnoreActorWhenMoving(IgnoredInstigator, true);
		}
	}
	
	// 클라이언트 원격 투사체는 Spawn payload를 받은 뒤 프레젠테이션을 시작한다.
	bIsRemoteProjectile = IsRemoteAuthProjectilePresentation();
	
	if (bIsRemoteProjectile)
	{
		// RepMovement가 BeginPlay 이전에 이미 도착한 경우 즉시 처리
		if (RepMovement.Event == EPRRepMovementEvent::Spawn && !RepMovement.Location.IsZero())
		{
			HandleRepSpawn();
		}
		else
		{
			// 스폰 이벤트 RepMovement 수신 전까지 숨김. OnRep_RepMovement에서 언히든
			SetActorHiddenInGame(true);
			SetActorEnableCollision(false);
			if (IsValid(ProjectileMovementComponent))
			{
				ProjectileMovementComponent->Deactivate();
			}
			SetLifeSpan(0.3f);
		}
		return;
	}

	if (HasProjectileAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (UPRRespawnSubsystem* RespawnSubsystem = World->GetSubsystem<UPRRespawnSubsystem>())
			{
				// 일회성 투사체 등록
				RespawnSubsystem->RegisterDisposableActor(this);
			}
		}
	}

#if WITH_EDITOR
	DrawDebugs(0);
#endif
}

void APRProjectileBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if WITH_EDITOR
	DrawDebugs(DeltaSeconds);
#endif

	if (bIsRemoteProjectile && bClientProjectilePresentationActive)
	{
		UpdateClientProjectileHomingPresentation(DeltaSeconds);
		return;
	}
	
	if (bShouldSyncToAuth && LinkedCounterpart.IsValid())
	{
		const FVector AuthLocation = LinkedCounterpart->GetActorLocation();
		const FVector Delta = AuthLocation - GetActorLocation();

		// 현재 이동 방향 기준으로 Auth가 앞/뒤 판별
		// Dot > 0: Auth가 진행 방향 앞: 빠르게 따라잡음
		// Dot < 0: Auth가 진행 방향 뒤: 천천히 늦춤
		const FVector TravelDir = IsValid(ProjectileMovementComponent)
			? ProjectileMovementComponent->Velocity.GetSafeNormal()
			: GetActorForwardVector();
		const float Dot = FVector::DotProduct(Delta, TravelDir);
		const float InterpSpeed = Dot >= 0.f ? SyncInterpSpeedCatchUp : SyncInterpSpeedSlowDown;

		const FVector NewLocation = FMath::VInterpTo(GetActorLocation(), AuthLocation, DeltaSeconds, InterpSpeed);
		const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), LinkedCounterpart->GetActorRotation(), DeltaSeconds, InterpSpeed);
		SetActorLocationAndRotation(NewLocation, NewRotation);
	}
}

void APRProjectileBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasProjectileAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (UPRRespawnSubsystem* RespawnSubsystem = World->GetSubsystem<UPRRespawnSubsystem>())
			{
				// 일회성 투사체 등록 해제
				RespawnSubsystem->UnregisterDisposableActor(this);
			}
		}
	}

	ClearProjectileHomingScheduleTimers();
	StopClientProjectileHomingPresentation();
	Super::EndPlay(EndPlayReason);

	if (ProjectileRole == EPRProjectileRole::Predicted)
	{
		UPRProjectileManagerComponent* Manager = UPRProjectileManagerComponent::FindForActor(GetWorld()->GetFirstPlayerController());
		if (!Manager)
		{
			return;
		}
		Manager->ClearProjectile(ProjectileId);	
	}
}

void APRProjectileBase::OnAuthLinked(APRProjectileBase* AuthProjectile)
{
	// 이 투사체를 Auth 위치로 보간 시작
	if (!IsValid(AuthProjectile))
	{
		return;
	}
	bShouldSyncToAuth = true;
}

void APRProjectileBase::OnRep_ProjectileId()
{
	// COND_OwnerOnly이므로 소유 클라이언트에서만 도달.
	ProjectileRole = EPRProjectileRole::Auth;
	TryLinkToPredictedOnClient();
	// HasActorBegunPlay()가 false면 BeginPlay에서 처리
}

void APRProjectileBase::ApplyEffectToTarget(AActor* TargetActor)
{
	if (GetLocalRole() != ROLE_Authority || ProjectileRole == EPRProjectileRole::Predicted)
	{
		return;
	}

	FGameplayEffectSpec* EffectSpec = EffectSpecHandle.Data.Get();
	if (EffectSpec == nullptr)
	{
		return;
	}

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor);
	if (ASI)
	{
		UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent();
		if (TargetASC)
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpec);
		}
		return;
	}

	if (IPRDirectDamageReceiverInterface* DirectDamageReceiver = Cast<IPRDirectDamageReceiverInterface>(TargetActor))
	{
		DirectDamageReceiver->ApplyDirectDamageFromSpec(*EffectSpec, FHitResult());
	}
}

void APRProjectileBase::ApplyEffectToTargetWithHit(AActor* TargetActor, const FHitResult& InHitResult)
{
	if (GetLocalRole() != ROLE_Authority || ProjectileRole == EPRProjectileRole::Predicted)
	{
		return;
	}
	
	IAbilitySystemInterface* InitialASI = Cast<IAbilitySystemInterface>(TargetActor);
	if (InitialASI)
	{
		UAbilitySystemComponent* TargetASC = InitialASI->GetAbilitySystemComponent();
		FGameplayEffectSpec* EffectSpec = EffectSpecHandle.Data.Get();
		if (TargetASC && EffectSpec)
		{
			EffectSpec->GetContext().AddHitResult(InHitResult,true);
			TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpec);	
		}
		return;
	}
	
	// 2026.05.31 이건주_ ASC 미보유 대상 대미지 적용 브릿지
	IPRDestructableInterface* DestructableTarget = Cast<IPRDestructableInterface>(TargetActor);
	if (DestructableTarget)
	{
		// Instigator, DamageAmount만 전달
		FGameplayEffectSpec* EffectSpec = EffectSpecHandle.Data.Get();
		if (EffectSpec == nullptr)
		{
			return;
		}

		const FGameplayEffectContextHandle& EffectContext = EffectSpec->GetEffectContext();
	
		const float BaseDamage = EffectSpec->GetSetByCallerMagnitude(
		PRCombatGameplayTags::SetByCaller_CurrentWeapon_BaseDamage,
		false,
		0.0f);
	
		// 파괴 가능 대상 전용 컨텍스트
		FPRDestructableDamageReceiveContext DestructableDamageContext;
		DestructableDamageContext.Instigator = EffectContext.GetInstigator();
		DestructableDamageContext.DamageAmount = BaseDamage;
		DestructableDamageContext.HitResult = InHitResult;

		DestructableTarget->ReceiveDamageContext(DestructableDamageContext);
		return;
	}

	FGameplayEffectSpec* EffectSpec = EffectSpecHandle.Data.Get();
	if (EffectSpec == nullptr)
	{
		return;
	}

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor);
	if (ASI)
	{
		UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent();
		if (TargetASC)
		{
			EffectSpec->GetContext().AddHitResult(InHitResult, true);
			TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpec);
		}
		return;
	}

	if (IPRDirectDamageReceiverInterface* DirectDamageReceiver = Cast<IPRDirectDamageReceiverInterface>(TargetActor))
	{
		DirectDamageReceiver->ApplyDirectDamageFromSpec(*EffectSpec, InHitResult);
	}
}

void APRProjectileBase::LinkCounterpart(APRProjectileBase* InCounterpart)
{
	if (!IsValid(InCounterpart))
	{
		return;
	}

	LinkedCounterpart = InCounterpart;

	// 양방향 링크 보장
	if (InCounterpart->LinkedCounterpart.Get() != this)
	{
		InCounterpart->LinkCounterpart(this);
	}
	bIsLinked = true;
	
	if (GetProjectileRole() == EPRProjectileRole::Auth && LinkedCounterpart.IsValid())
	{
		// 예측 클라측은 감춤
		SetActorHiddenInGame(true);	
	}
}

void APRProjectileBase::TryLinkToPredictedOnClient()
{
	if (ProjectileId == 0 || LinkedCounterpart.IsValid())
	{
		return;
	}
	
	UPRProjectileManagerComponent* Manager = UPRProjectileManagerComponent::FindForActor(GetWorld()->GetFirstPlayerController());
	if (!Manager)
	{
		return;
	}
	
	// Auth 측을 매니저 맵의 Auth 슬롯에 등록
	Manager->NotifyAuthArrived(ProjectileId, this);

	APRProjectileBase* Predicted = Manager->FindPredictedProjectile(ProjectileId);
	if (IsValid(Predicted))
	{
		LinkCounterpart(Predicted);
		Predicted->OnAuthLinked(this);
	}
}

void APRProjectileBase::OnSphereHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 자기 자신이나 발사자에 의한 히트 무시
	if (!IsValid(OtherActor) || OtherActor == this || OtherActor == GetInstigator())
	{
		return;
	}
	
	if (GetProjectileRole() == EPRProjectileRole::Predicted)
	{
		if (!ProjectileMovementComponent->ShouldBounce(Hit))
		{
			// 예측 투사체가 먼저 Hit에 성공한 경우
			if (LinkedCounterpart.IsValid())
			{
				//LinkedCounterpart->SetActorHiddenInGame(false);
				Destroy();
			}
		}
	}
	else if (GetProjectileRole() == EPRProjectileRole::Auth)
	{
		if (!ProjectileMovementComponent->ShouldBounce(Hit))
		{
			// 예측 투사체가 존재하고, 권위 투사체가 먼저 Hit에 성공한 경우
			if (LinkedCounterpart.IsValid())
			{
				// 가시성 복구
				SetActorHiddenInGame(false);
				SetActorEnableCollision(true);
			
				// 예측 투사체를 즉시 파괴
				LinkedCounterpart->Destroy();
			}
		}
		
		if (HasAuthority())
		{
			if (!HitActors.Contains(OtherActor))
			{
				HandleHit(HitComponent,OtherActor, OtherComp, NormalImpulse, Hit);
			}
			HitActors.Add(OtherActor);
			
			if (!ProjectileMovementComponent->ShouldBounce(Hit))
			{
				DestroyProjectile();	
			}
		}
		// Replicate된 투사체인 경우
		else
		{
			SetActorHiddenInGame(true);
			SetActorEnableCollision(false);
			ProjectileMovementComponent->SetComponentTickEnabled(false);
		}
		
		
		// TODO: 권위 투사체의 첫 복제 전 바로 파괴되어 버린 경우 Remote의 파괴 이펙트 보장 필요
	}
}

void APRProjectileBase::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	OnSphereHit(OverlappedComponent, OtherActor, OtherComp, FVector::ZeroVector, SweepResult);
}

void APRProjectileBase::HandleHit_Implementation(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	
}

void APRProjectileBase::DrawDebugs(float DeltaSeconds)
{
#if WITH_EDITORONLY_DATA
	if (HasAuthority() && ProjectileRole == EPRProjectileRole::Auth)
	{
		return;
	}

	if (!bDrawDebugSphere)
	{
		return;
	}

	DebugDrawAccumulator += DeltaSeconds;
	if (DebugDrawInterval > 0.f && DebugDrawAccumulator < DebugDrawInterval)
	{
		return;
	}
	DebugDrawAccumulator = 0.f;

	const FColor& DrawColor = (ProjectileRole == EPRProjectileRole::Predicted) ? DebugColorPredicted : DebugColorAuth;
	DrawDebugSphere(GetWorld(), GetActorLocation(), DebugSphereRadius, 12, DrawColor, false, DebugDrawLifetime);
#endif
}
