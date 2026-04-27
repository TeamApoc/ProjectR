// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRProjectileBase.h"

#include "PRProjectileMovementComponent.h"
#include "PRProjectileManagerComponent.h"
#include "Net/UnrealNetwork.h"

APRProjectileBase::APRProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(false);
	bNetUseOwnerRelevancy = true;

	ProjectileMovementComponent = CreateDefaultSubobject<UPRProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
}

void APRProjectileBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(APRProjectileBase, ProjectileId, COND_OwnerOnly);
}

void APRProjectileBase::InitializeProjectile(EPRProjectileRole InRole, uint32 InProjectileId)
{
	ProjectileRole = InRole;
	ProjectileId = InProjectileId;
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
	// COND_OwnerOnly이므로 소유 클라이언트에서만 도달. 이 경로는 항상 케이스 (3)
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
