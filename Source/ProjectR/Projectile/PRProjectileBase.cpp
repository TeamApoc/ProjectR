// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRProjectileBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemInterface.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "PRProjectileMovementComponent.h"
#include "PRProjectileManagerComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

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
	SphereComponent->OnComponentHit.AddDynamic(this, &APRProjectileBase::OnSphereHit);

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

	MARK_PROPERTY_DIRTY_FROM_NAME(APRProjectileBase, RepMovement, this);
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
	}
}

void APRProjectileBase::HandleRepSpawn()
{
	bIsRemoteProjectile = GetNetOwner() == nullptr;
	if (!bIsRemoteProjectile)
	{
		return;
	}
	
	SetActorLocation(RepMovement.Location);
	SetActorRotation(RepMovement.Rotation);

	if (IsValid(ProjectileMovementComponent))
	{
		ProjectileMovementComponent->Velocity = RepMovement.Velocity;
		ProjectileMovementComponent->Activate();
	}

	// SimulatedProxy에서 숨김 처리 했던 상태를 복구
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	
	bHasRepSpawnHandled = true;
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
		DestroyProjectile();
	}
}


void APRProjectileBase::DestroyProjectile()
{
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

void APRProjectileBase::BeginPlay()
{
	Super::BeginPlay();
	
	// 투사체의 NetOwner는 PC로 설정되므로, 클라이언트 remote에서는 nullptr이다.
	// AI/보스가 서버에서 직접 생성한 권위 투사체는 NetOwner가 없을 수 있으므로 서버에서는 remote 판정을 하지 않는다.
	bIsRemoteProjectile = !HasAuthority() && GetNetOwner() == nullptr;
	
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
		}
		return;
	}
	else if (!HasAuthority() && GetProjectileRole() == EPRProjectileRole::Auth)
	{
		// 예측 클라측은 감춤
		SetActorHiddenInGame(true);
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
	
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor);
	if (!ASI)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}
	
	if (FGameplayEffectSpec* EffectSpec = EffectSpecHandle.Data.Get())
	{
		TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpec);	
	}
}

void APRProjectileBase::ApplyEffectToTargetWithHit(AActor* TargetActor, const FHitResult& InHitResult)
{
	if (GetLocalRole() != ROLE_Authority || ProjectileRole == EPRProjectileRole::Predicted)
	{
		return;
	}
	
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor);
	if (!ASI)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}
	
	if (FGameplayEffectSpec* EffectSpec = EffectSpecHandle.Data.Get())
	{
		EffectSpec->GetContext().AddHitResult(InHitResult,true);
		TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpec);	
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
	
	if (HitActors.Contains(OtherActor))
	{
		return;
	}
	HitActors.Add(OtherActor);
	
	if (GetProjectileRole() == EPRProjectileRole::Predicted)
	{
		// 예측 투사체가 먼저 Hit에 성공한 경우
		if (LinkedCounterpart.IsValid())
		{
			// 권위 투사체로 가시성을 전환
			LinkedCounterpart->SetActorHiddenInGame(false);
			Destroy();
			return;
		}
	}
	
	if (GetProjectileRole() == EPRProjectileRole::Auth)
	{
		// 권위 투사체가 먼저 Hit에 성공한 경우
		if (LinkedCounterpart.IsValid())
		{
			// 예측 투사체를 즉시 파괴
			LinkedCounterpart->Destroy();
		}
	}
	
	HandleHit(HitComponent,OtherActor, OtherComp, NormalImpulse, Hit);
	
	// TODO: 권위 투사체의 첫 복제 전 바로 파괴되어 버린 경우 Remote의 파괴 이펙트 보장 필요
}

void APRProjectileBase::HandleHit_Implementation(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	DestroyProjectile();
}

void APRProjectileBase::DrawDebugs(float DeltaSeconds)
{
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
}
