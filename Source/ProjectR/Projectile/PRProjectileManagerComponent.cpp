// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRProjectileManagerComponent.h"

#include "PRProjectileBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "Engine/World.h"

UPRProjectileManagerComponent::UPRProjectileManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/*~ Lookup ~*/

UPRProjectileManagerComponent* UPRProjectileManagerComponent::FindForActor(AActor* InActor)
{
	if (!IsValid(InActor))
	{
		return nullptr;
	}

	if (UPRProjectileManagerComponent* Found = InActor->FindComponentByClass<UPRProjectileManagerComponent>())
	{
		return Found;
	}

	// Pawn이면 Controller로 탐색
	if (APawn* AsPawn = Cast<APawn>(InActor))
	{
		if (AController* Ctrl = AsPawn->GetController())
		{
			if (UPRProjectileManagerComponent* Found = Ctrl->FindComponentByClass<UPRProjectileManagerComponent>())
			{
				return Found;
			}
		}
	}

	return nullptr;
}

/*~ Prediction Tunables ~*/

float UPRProjectileManagerComponent::GetForwardPredictionTime() const
{
	APlayerController* PC = ResolveOwningController();
	if (!IsValid(PC) || !IsValid(PC->PlayerState))
	{
		return 0.f;
	}

	const float ExactPingMs = PC->PlayerState->ExactPing;
	const float EffectivePingMs = FMath::Max(0.f, ExactPingMs - PredictionLatencyReduction);
	const float ClampedPingMs = FMath::Min(EffectivePingMs, MaxPredictionPing);
	return 0.001f * ClientBiasPct * ClampedPingMs;
}

float UPRProjectileManagerComponent::GetProjectileSpawnDelay() const
{
	APlayerController* PC = ResolveOwningController();
	if (!IsValid(PC) || !IsValid(PC->PlayerState))
	{
		return 0.f;
	}

	const float ExactPingMs = PC->PlayerState->ExactPing;
	const float OverflowMs = ExactPingMs - PredictionLatencyReduction - MaxPredictionPing;
	return 0.001f * FMath::Max(0.f, OverflowMs);
}

/*~ Id Allocation ~*/

uint32 UPRProjectileManagerComponent::GenerateNewProjectileId()
{
	const uint32 NewId = NextSpawnedProjectileId++;
	if (NextSpawnedProjectileId == NULL_PROJECTILE_ID)
	{
		// 0은 무효 ID이므로 건너뜀
		NextSpawnedProjectileId = 1;
	}
	return NewId;
}

/*~ Map Registration ~*/

void UPRProjectileManagerComponent::RegisterPredictedProjectile(uint32 Id, APRProjectileBase* ProjectileActor)
{
	if (Id == NULL_PROJECTILE_ID || !IsValid(ProjectileActor))
	{
		return;
	}

	FPRLinkedProjectile& Entry = SpawnedProjectilesMap.FindOrAdd(Id);
	Entry.PredictedProjectile = ProjectileActor;
}

void UPRProjectileManagerComponent::UnregisterPredictedProjectile(uint32 Id)
{
	SpawnedProjectilesMap.Remove(Id);
}

APRProjectileBase* UPRProjectileManagerComponent::FindPredictedProjectile(uint32 Id) const
{
	if (const FPRLinkedProjectile* Entry = SpawnedProjectilesMap.Find(Id))
	{
		return Entry->PredictedProjectile.Get();
	}
	return nullptr;
}

void UPRProjectileManagerComponent::NotifyAuthArrived(uint32 Id, APRProjectileBase* AuthProjectile)
{
	if (Id == NULL_PROJECTILE_ID || !IsValid(AuthProjectile))
	{
		return;
	}

	FPRLinkedProjectile& Entry = SpawnedProjectilesMap.FindOrAdd(Id);
	Entry.AuthProjectile = AuthProjectile;
}

/*~ Spawn ~*/

APRProjectileBase* UPRProjectileManagerComponent::SpawnPredictedProjectile(FPRProjectileSpawnInfo& SpawnInfo)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(SpawnInfo.ProjectileClass))
	{
		return nullptr;
	}
	
	check(!SpawnInfo.ProjectileId == NULL_PROJECTILE_ID);
	if (SpawnInfo.ProjectileId == NULL_PROJECTILE_ID)
	{
		return nullptr;
	}

	// Owner/Instigator 자동 설정 (호출자가 SpawnParameters에 미지정한 경우만)
	if (SpawnInfo.SpawnParameters.Owner == nullptr)
	{
		SpawnInfo.SpawnParameters.Owner = GetOwner();
	}
	if (SpawnInfo.SpawnParameters.Instigator == nullptr)
	{
		if (APawn* Pawn = Cast<APawn>(GetOwner()))
		{
			SpawnInfo.SpawnParameters.Instigator = Pawn;
		}
		else if (AController* Ctrl = Cast<AController>(GetOwner()))
		{
			SpawnInfo.SpawnParameters.Instigator = Ctrl->GetPawn();
		}
	}
	
	SpawnInfo.SpawnParameters.CustomPreSpawnInitalization = [this, SpawnInfo](AActor* Actor)
	{
		if (APRProjectileBase* ProjectileBase = Cast<APRProjectileBase>(Actor))
		{
			ProjectileBase->InitializeProjectile(EPRProjectileRole::Predicted, SpawnInfo.ProjectileId);
		}
	};

	APRProjectileBase* Predicted = World->SpawnActor<APRProjectileBase>(
		SpawnInfo.ProjectileClass,
		SpawnInfo.SpawnLocation,
		SpawnInfo.SpawnRotation,
		SpawnInfo.SpawnParameters);

	if (!IsValid(Predicted))
	{
		return nullptr;
	}

	RegisterPredictedProjectile(SpawnInfo.ProjectileId, Predicted);
	return Predicted;
}

void UPRProjectileManagerComponent::SpawnPredictedProjectileDelayed(FPRProjectileSpawnInfo& SpawnInfo, FProjectileSpawnedDelegate& OnSpawned)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	check(!SpawnInfo.ProjectileId == NULL_PROJECTILE_ID);
	if (SpawnInfo.ProjectileId == NULL_PROJECTILE_ID)
	{
		return;
	}

	const float Delay = GetProjectileSpawnDelay();
	if (Delay <= 0.f)
	{
		// 지연이 없으면 즉시 스폰 후 콜백
		APRProjectileBase* Spawned = SpawnPredictedProjectile(SpawnInfo);
		if (OnSpawned.IsBound())
		{
			OnSpawned.Execute(Spawned);
		}
		return;
	}

	const uint32 Id = SpawnInfo.ProjectileId;
	FPendingDelayedSpawn& Pending = PendingDelayedSpawns.Add(Id);
	Pending.Params = SpawnInfo;
	Pending.Callback = OnSpawned;

	FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &UPRProjectileManagerComponent::HandleDelayedSpawnElapsed, Id);
	World->GetTimerManager().SetTimer(Pending.Timer, TimerDelegate, Delay, false);
}

void UPRProjectileManagerComponent::CancelDelayedSpawn(uint32 Id)
{
	FPendingDelayedSpawn* Pending = PendingDelayedSpawns.Find(Id);
	if (!Pending)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Pending->Timer);
	}
	PendingDelayedSpawns.Remove(Id);
}

APRProjectileBase* UPRProjectileManagerComponent::SpawnAuthProjectile(FPRProjectileSpawnInfo& SpawnInfo)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(SpawnInfo.ProjectileClass) || SpawnInfo.ProjectileId == NULL_PROJECTILE_ID)
	{
		return nullptr;
	}

	if (SpawnInfo.SpawnParameters.Owner == nullptr)
	{
		SpawnInfo.SpawnParameters.Owner = GetOwner();
	}
	if (SpawnInfo.SpawnParameters.Instigator == nullptr)
	{
		if (APawn* Pawn = Cast<APawn>(GetOwner()))
		{
			SpawnInfo.SpawnParameters.Instigator = Pawn;
		}
		else if (AController* Ctrl = Cast<AController>(GetOwner()))
		{
			SpawnInfo.SpawnParameters.Instigator = Ctrl->GetPawn();
		}
	}

	SpawnInfo.SpawnParameters.CustomPreSpawnInitalization = [this, SpawnInfo](AActor* Actor)
	{
		if (APRProjectileBase* ProjectileBase = Cast<APRProjectileBase>(Actor))
		{
			ProjectileBase->InitializeProjectile(EPRProjectileRole::Auth, SpawnInfo.ProjectileId);
		}
	};

	APRProjectileBase* Auth = World->SpawnActor<APRProjectileBase>(
		SpawnInfo.ProjectileClass,
		SpawnInfo.SpawnLocation,
		SpawnInfo.SpawnRotation,
		SpawnInfo.SpawnParameters);

	if (!IsValid(Auth))
	{
		return nullptr;
	}

	NotifyAuthArrived(SpawnInfo.ProjectileId, Auth);
	return Auth;
}

void UPRProjectileManagerComponent::ClearProjectile(uint32 Id)
{
	if (SpawnedProjectilesMap.Contains(Id))
	{
		SpawnedProjectilesMap.Remove(Id);
	}
}

/*~ Internal ~*/

APlayerController* UPRProjectileManagerComponent::ResolveOwningController() const
{
	AActor* Owner = GetOwner();
	if (APlayerController* PC = Cast<APlayerController>(Owner))
	{
		return PC;
	}
	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		return Cast<APlayerController>(Pawn->GetController());
	}
	return nullptr;
}

void UPRProjectileManagerComponent::HandleDelayedSpawnElapsed(uint32 Id)
{
	FPendingDelayedSpawn* Pending = PendingDelayedSpawns.Find(Id);
	if (!Pending)
	{
		return;
	}

	// 로컬 복사 후 맵에서 제거 (콜백 도중 재진입 방지)
	FPRProjectileSpawnInfo Params = Pending->Params;
	FProjectileSpawnedDelegate Callback = Pending->Callback;
	PendingDelayedSpawns.Remove(Id);

	APRProjectileBase* Spawned = SpawnPredictedProjectile(Params);
	Callback.ExecuteIfBound(Spawned);
}
