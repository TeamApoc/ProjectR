// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRProjectileBase.h"

#include "DrawDebugHelpers.h"
#include "PRProjectileMovementComponent.h"
#include "PRProjectileManagerComponent.h"
#include "Net/UnrealNetwork.h"

APRProjectileBase::APRProjectileBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	bReplicates = true;
	SetReplicateMovement(false);
	bNetUseOwnerRelevancy = true;

	ProjectileMovementComponent = CreateDefaultSubobject<UPRProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
}

void APRProjectileBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	constexpr bool bUsePushModel = true;
	
	FDoRepLifetimeParams OwnerOnlyParams{COND_OwnerOnly, REPNOTIFY_Always, bUsePushModel};
	DOREPLIFETIME_WITH_PARAMS_FAST(APRProjectileBase, ProjectileId, OwnerOnlyParams);
}

void APRProjectileBase::InitializeProjectile(EPRProjectileRole InRole, uint32 InProjectileId)
{
	ProjectileRole = InRole;
	ProjectileId = InProjectileId;
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
}

void APRProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority() && ProjectileRole == EPRProjectileRole::Auth)
	{
		TryLinkToPredictedOnClient();
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

void APRProjectileBase::OnRep_ProjectileId()
{
	// COND_OwnerOnly이므로 소유 클라이언트에서만 도달.
	ProjectileRole = EPRProjectileRole::Auth;

	if (HasActorBegunPlay())
	{
		TryLinkToPredictedOnClient();
	}
	// HasActorBegunPlay()가 false면 BeginPlay에서 처리
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
	}
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