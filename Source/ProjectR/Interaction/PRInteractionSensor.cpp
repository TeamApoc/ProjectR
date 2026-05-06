// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PRInteractionSensor.h"

#include "PRInteractableActor.h"
#include "PRInteractionInterface.h"
#include "PRInteractorComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ProjectR/ProjectR.h"

UPRInteractionSensor::UPRInteractionSensor()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.1f;
	
	CandidateClassFilter = APRInteractableActor::StaticClass();
	TraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(PRCollisionChannels::ECC_Interactable));
}

void UPRInteractionSensor::BeginPlay()
{
	Super::BeginPlay();

	// 로컬 컨트롤러에서만 동작 (서버/원격 클라에서는 트레이스 무의미)
	if (const APlayerController* PC = GetOwningPlayerController())
	{
		if (!PC->IsLocalController())
		{
			SetComponentTickEnabled(false);
		}
	}
}

void UPRInteractionSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateFocus();
}

void UPRInteractionSensor::SetInteractionBlocked(bool bInBlocked)
{
	if (bInteractionBlocked == bInBlocked)
	{
		return;
	}

	bInteractionBlocked = bInBlocked;

	// 차단 진입 시 즉시 포커스 해제
	if (bInteractionBlocked)
	{
		FocusedActor.Reset();
		if (UPRInteractorComponent* Interactor = GetInteractor())
		{
			Interactor->TryFocus(nullptr);
		}
	}
}

void UPRInteractionSensor::UpdateFocus()
{
	UPRInteractorComponent* Interactor = GetInteractor();
	if (!IsValid(Interactor))
	{
		return;
	}

	// 차단 시 포커스 해제 후 종료
	if (bInteractionBlocked)
	{
		if (FocusedActor.IsValid())
		{
			FocusedActor.Reset();
			Interactor->TryFocus(nullptr);
		}
		return;
	}

	const APawn* Pawn = GetOwningPlayerController() ? GetOwningPlayerController()->GetPawn() : nullptr;
	if (!IsValid(Pawn))
	{
		FocusedActor.Reset();
		Interactor->TryFocus(nullptr);
		return;
	}

	TArray<AActor*> Candidates;
	CollectCandidates(Pawn->GetActorLocation(), FocusableRange, Candidates);

	// 1) 화면 중앙 후보가 있으면 히스테리시스 무시하고 즉시 전환
	if (AActor* ScreenPick = FindScreenCenterCandidate(Candidates))
	{
		FocusedActor = ScreenPick;
		Interactor->TryFocus(ScreenPick);
		return;
	}

	// 2) 화면 중앙 후보 없음 - 기존 포커스가 히스테리시스 범위 안이면 유지
	AActor* Current = FocusedActor.Get();
	if (IsValid(Current) && ShouldRetainFocus(Current))
	{
		Interactor->TryFocus(Current);
		return;
	}

	// 3) 그 외 거리 최단 후보로 폴백
	AActor* Nearest = FindNearestCandidate(Candidates);
	FocusedActor = Nearest;
	Interactor->TryFocus(Nearest);
}

void UPRInteractionSensor::CollectCandidates(const FVector& Origin, float Radius, TArray<AActor*>& OutCandidates) const
{
	OutCandidates.Reset();

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	TArray<AActor*> ActorsToIgnore;
	if (const APlayerController* PC = GetOwningPlayerController())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			ActorsToIgnore.Add(Pawn);
		}
	}

	TArray<AActor*> Found;
	UKismetSystemLibrary::SphereOverlapActors(
		this,
		Origin,
		Radius,
		TraceObjectTypes,
		nullptr,//IsValid(CandidateClassFilter) ? CandidateClassFilter->GetClass() : AActor::StaticClass(),
		ActorsToIgnore,
		Found);

	for (AActor* Actor : Found)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		if (!Actor->Implements<UPRInteractionInterface>())
		{
			continue;
		}

		// 1차 후보 통과 후 Pawn 발/시야 가시성 체크 (장애물 차폐된 후보 제외)
		if (!HasLineOfSight(Actor))
		{
			continue;
		}

		OutCandidates.Add(Actor);
	}
}

bool UPRInteractionSensor::HasLineOfSight(const AActor* Target) const
{
	if (!IsValid(Target))
	{
		return false;
	}

	const APlayerController* PC = GetOwningPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!IsValid(Pawn))
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PRInteractionLOS), false);
	Params.AddIgnoredActor(Pawn);

	const FVector TargetLoc = Target->GetActorLocation();

	auto TraceVisible = [&](const FVector& Origin) -> bool
	{
		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(Hit, Origin, TargetLoc, ECC_Visibility, Params);
		if (!bHit)
		{
			return true;
		}
		return Hit.GetActor() == Target;
	};

	// 발 위치 또는 시야 위치 중 한 곳이라도 가시성 통과면 OK
	return TraceVisible(Pawn->GetActorLocation()) || TraceVisible(Pawn->GetPawnViewLocation());
}

AActor* UPRInteractionSensor::FindScreenCenterCandidate(const TArray<AActor*>& Candidates) const
{
	if (Candidates.Num() == 0)
	{
		return nullptr;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!IsValid(PC))
	{
		return nullptr;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);

	const FVector2D ScreenCenter(ViewportSizeX * 0.5f, ViewportSizeY * 0.5f);
	const float MaxScreenDist = FMath::Min(ViewportSizeX, ViewportSizeY) * ScreenCenterRadius;

	AActor* BestOnScreen = nullptr;
	float BestScreenDist = TNumericLimits<float>::Max();

	for (AActor* Actor : Candidates)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		FVector2D Screen;
		const bool bOnScreen = PC->ProjectWorldLocationToScreen(Actor->GetActorLocation(), Screen);
		if (!bOnScreen)
		{
			continue;
		}

		const float ScreenDist = FVector2D::Distance(Screen, ScreenCenter);
		if (ScreenDist <= MaxScreenDist && ScreenDist < BestScreenDist)
		{
			BestScreenDist = ScreenDist;
			BestOnScreen = Actor;
		}
	}

	return BestOnScreen;
}

AActor* UPRInteractionSensor::FindNearestCandidate(const TArray<AActor*>& Candidates) const
{
	if (Candidates.Num() == 0)
	{
		return nullptr;
	}

	const APlayerController* PC = GetOwningPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!IsValid(Pawn))
	{
		return nullptr;
	}

	const FVector PawnLoc = Pawn->GetActorLocation();

	AActor* BestNearest = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (AActor* Actor : Candidates)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		const float DistSq = (Actor->GetActorLocation() - PawnLoc).SizeSquared();
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestNearest = Actor;
		}
	}

	return BestNearest;
}

bool UPRInteractionSensor::ShouldRetainFocus(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	if (!Actor->Implements<UPRInteractionInterface>())
	{
		return false;
	}

	const APlayerController* PC = GetOwningPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!IsValid(Pawn))
	{
		return false;
	}

	const float DistSq = (Actor->GetActorLocation() - Pawn->GetActorLocation()).SizeSquared();
	const float ReleaseRange = FocusableRange * HysteresisFactor;
	return DistSq <= FMath::Square(ReleaseRange);
}

UPRInteractorComponent* UPRInteractionSensor::GetInteractor() const
{
	if (CachedInteractor.IsValid())
	{
		return CachedInteractor.Get();
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!IsValid(PC))
	{
		return nullptr;
	}

	UPRInteractorComponent* Interactor = PC->FindComponentByClass<UPRInteractorComponent>();
	if (!IsValid(Interactor))
	{
		// 빙의 Pawn에 부착된 경우도 허용
		if (APawn* Pawn = PC->GetPawn())
		{
			Interactor = Pawn->FindComponentByClass<UPRInteractorComponent>();
		}
	}

	CachedInteractor = Interactor;
	return Interactor;
}

APlayerController* UPRInteractionSensor::GetOwningPlayerController() const
{
	return Cast<APlayerController>(GetOwner());
}
