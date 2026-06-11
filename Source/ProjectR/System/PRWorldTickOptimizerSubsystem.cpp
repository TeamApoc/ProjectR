// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (World Tick Optimizer 서브시스템 구현)
#include "PRWorldTickOptimizerSubsystem.h"

#include "AbilitySystemComponent.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/PRGameplayTags.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRWorldTickOptimizer, Log, All);

namespace
{
	static TAutoConsoleVariable<int32> CVarPRWorldTickOptimizerEnabled(
		TEXT("pr.WorldTickOptimizer.Enabled"),
		1,
		TEXT("월드 Tick 최적화 활성 여부.\n0: 비활성\n1: 활성"),
		ECVF_Default);

	static TAutoConsoleVariable<int32> CVarPRWorldTickOptimizerShowActors(
		TEXT("pr.WorldTickOptimizer.ShowActors"),
		0,
		TEXT("월드 Tick 최적화 대상 액터 상태 박스 표시 여부.\n0: 비활성\n1: 활성"),
		ECVF_Default);

	static TAutoConsoleVariable<int32> CVarPRWorldTickOptimizerVisibilityGateEnabled(
		TEXT("pr.WorldTickOptimizer.VisibilityGate.Enabled"),
		1,
		TEXT("월드 Tick 최적화 Visibility 게이트 활성 여부.\n0: B 반경 내부 Tick 활성\n1: A 반경 내부 Tick 활성, A-B 구간 Visibility 검사"),
		ECVF_Default);

	static TAutoConsoleVariable<int32> CVarPRWorldTickOptimizerVisibilityGateMaxTracesPerEvaluation(
		TEXT("pr.WorldTickOptimizer.VisibilityGate.MaxTracesPerEvaluation"),
		128,
		TEXT("월드 Tick 최적화 Visibility 게이트 평가당 최대 trace 횟수.\n0 이하: 무제한"),
		ECVF_Default);

	// 액터 상태 디버그 박스 표시 여부 확인
	bool IsActorDebugDrawEnabled()
	{
		return CVarPRWorldTickOptimizerShowActors.GetValueOnGameThread() > 0;
	}

	// Tick Visibility 게이트 활성 여부 확인
	bool IsTickVisibilityGateEnabled()
	{
		return CVarPRWorldTickOptimizerVisibilityGateEnabled.GetValueOnGameThread() > 0;
	}

	// Tick Visibility 게이트 trace 예산 반환
	int32 GetTickVisibilityGateTraceBudget()
	{
		return CVarPRWorldTickOptimizerVisibilityGateMaxTracesPerEvaluation.GetValueOnGameThread();
	}

	// Tick Visibility 게이트 반경 설정 유효성 확인
	bool IsTickVisibilityGateConfigValid(const FPRTickOptimizationConfig& Config)
	{
		return Config.TickAlwaysActiveActivationRadius > 0.0f
			&& Config.TickAlwaysActiveDeactivationRadius >= Config.TickAlwaysActiveActivationRadius
			&& Config.TickActivationRadius > Config.TickAlwaysActiveActivationRadius
			&& Config.TickDeactivationRadius > Config.TickAlwaysActiveDeactivationRadius;
	}

	// 최적화 대상 액터의 Tick 활성 상태 시각화
	void DrawActorStateBoxes(const UWorld* World, const TArray<FPRTickOptimizationEntry>& ActiveEntries, float DrawDuration)
	{
		if (!IsValid(World) || !IsActorDebugDrawEnabled())
		{
			return;
		}

		const FVector DebugBoxExtent(50.0f, 50.0f, 50.0f);
		const float ClampedDrawDuration = FMath::Max(DrawDuration, 0.0f);
		for (const FPRTickOptimizationEntry& Entry : ActiveEntries)
		{
			const AActor* TargetActor = Entry.TargetActor.Get();
			if (!IsValid(TargetActor))
			{
				continue;
			}

			FColor DebugColor = FColor::Green;
			if (!Entry.IsTickActive())
			{
				DebugColor = Entry.DebugState == EPRTickOptimizationDebugState::BlockedByVisibility
					? FColor::Yellow
					: FColor::Red;
			}

			DrawDebugBox(
				World,
				TargetActor->GetActorLocation(),
				DebugBoxExtent,
				DebugColor,
				false,
				ClampedDrawDuration,
				SDPG_Foreground,
				2.0f);
		}
	}
}

/*~ UWorldSubsystem Interface ~*/

void UPRWorldTickOptimizerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	StartEvaluationTimer();
}

void UPRWorldTickOptimizerSubsystem::Deinitialize()
{
	StopEvaluationTimer();
	ActiveEntries.Empty();
	PendingRegisterTargets.Empty();
	PendingUnregisterTargets.Empty();
	CachedSources.Empty();

	Super::Deinitialize();
}

/*~ 등록 API ~*/

void UPRWorldTickOptimizerSubsystem::RegisterTarget(AActor* TargetActor)
{
	const UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_Client || !IsValid(TargetActor))
	{
		return;
	}

	const TWeakObjectPtr<AActor> TargetKey(TargetActor);
	PendingUnregisterTargets.Remove(TargetKey);
	PendingRegisterTargets.Add(TargetKey);
}

void UPRWorldTickOptimizerSubsystem::UnregisterTarget(AActor* TargetActor)
{
	const UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_Client || !IsValid(TargetActor))
	{
		return;
	}

	const TWeakObjectPtr<AActor> TargetKey(TargetActor);
	PendingRegisterTargets.Remove(TargetKey);
	PendingUnregisterTargets.Add(TargetKey);
}

// 대상 액터의 강제 활성 고정
void UPRWorldTickOptimizerSubsystem::ForceActivateTarget(AActor* TargetActor)
{
	const UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_Client || !IsValid(TargetActor))
	{
		return;
	}

	FlushPendingChanges();

	FPRTickOptimizationEntry* Entry = FindEntry(TargetActor);
	if (Entry == nullptr)
	{
		FPRTickOptimizationEntry NewEntry;
		if (!BuildEntry(TargetActor, NewEntry))
		{
			return;
		}

		ActiveEntries.Add(MoveTemp(NewEntry));
		Entry = &ActiveEntries.Last();
	}

	Entry->bForceActive = true;
	Entry->DebugState = EPRTickOptimizationDebugState::Active;
	ApplyEntryActiveState(*Entry, true, true);
}

// 대상 액터의 강제 활성 고정 해제
void UPRWorldTickOptimizerSubsystem::ClearForceActivateTarget(AActor* TargetActor)
{
	const UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_Client || !IsValid(TargetActor))
	{
		return;
	}

	FlushPendingChanges();

	FPRTickOptimizationEntry* Entry = FindEntry(TargetActor);
	if (Entry == nullptr)
	{
		return;
	}

	Entry->bForceActive = false;
}

void UPRWorldTickOptimizerSubsystem::ForceEvaluate()
{
	const UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_Client)
	{
		return;
	}

	FlushPendingChanges();
	if (!IsOptimizationEnabled())
	{
		RestoreAllTargetsActive();
		DrawActorStateBoxes(World, ActiveEntries, EvaluationInterval);
		CachedSources.Reset();
		return;
	}

	BuildSources();
	const int32 TraceBudget = GetTickVisibilityGateTraceBudget();
	RemainingVisibilityGateTraceBudget = TraceBudget <= 0 ? -1 : TraceBudget;
	EvaluateTargets();
	DrawActorStateBoxes(World, ActiveEntries, EvaluationInterval);
}

bool UPRWorldTickOptimizerSubsystem::IsOptimizationEnabled()
{
	return CVarPRWorldTickOptimizerEnabled.GetValueOnAnyThread() > 0;
}

/*~ 타이머 관리 ~*/

void UPRWorldTickOptimizerSubsystem::StartEvaluationTimer()
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_Client)
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		EvaluationTimerHandle,
		this,
		&UPRWorldTickOptimizerSubsystem::ForceEvaluate,
		EvaluationInterval,
		true,
		EvaluationInterval);
}

void UPRWorldTickOptimizerSubsystem::StopEvaluationTimer()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	World->GetTimerManager().ClearTimer(EvaluationTimerHandle);
}

/*~ 등록 반영 ~*/

void UPRWorldTickOptimizerSubsystem::FlushPendingChanges()
{
	CompactEntries();

	for (const TWeakObjectPtr<AActor>& WeakTarget : PendingRegisterTargets)
	{
		AActor* TargetActor = WeakTarget.Get();
		if (!IsValid(TargetActor))
		{
			continue;
		}

		const bool bAlreadyRegistered = ActiveEntries.ContainsByPredicate(
			[TargetActor](const FPRTickOptimizationEntry& Entry)
			{
				return Entry.TargetActor.Get() == TargetActor;
			});
		if (bAlreadyRegistered)
		{
			continue;
		}

		FPRTickOptimizationEntry NewEntry;
		if (BuildEntry(TargetActor, NewEntry))
		{
			ActiveEntries.Add(MoveTemp(NewEntry));
		}
	}

	PendingRegisterTargets.Empty();
	PendingUnregisterTargets.Empty();
	NextEvaluationIndex = ActiveEntries.IsValidIndex(NextEvaluationIndex) ? NextEvaluationIndex : 0;
}

void UPRWorldTickOptimizerSubsystem::CompactEntries()
{
	ActiveEntries.RemoveAllSwap(
		[this](const FPRTickOptimizationEntry& Entry)
		{
			AActor* TargetActor = Entry.TargetActor.Get();
			return !IsValid(TargetActor) || PendingUnregisterTargets.Contains(TWeakObjectPtr<AActor>(TargetActor));
		},
		EAllowShrinking::No);
}

bool UPRWorldTickOptimizerSubsystem::BuildEntry(AActor* TargetActor, FPRTickOptimizationEntry& OutEntry) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	IPRTickOptimizable* TargetInterface = Cast<IPRTickOptimizable>(TargetActor);
	if (TargetInterface == nullptr)
	{
		UE_LOG(
			LogPRWorldTickOptimizer,
			Warning,
			TEXT("[WorldTickOptimizer] TickOptimizable 미구현 Target=%s"),
			*GetNameSafe(TargetActor));
		return false;
	}

	const FPRTickOptimizationConfig Config = TargetInterface->GetTickConfig();
	if (Config.TickActivationRadius <= 0.0f || Config.TickDeactivationRadius < Config.TickActivationRadius)
	{
		UE_LOG(
			LogPRWorldTickOptimizer,
			Warning,
			TEXT("[WorldTickOptimizer] Tick 반경 설정 오류 Target=%s Activation=%.1f Deactivation=%.1f"),
			*GetNameSafe(TargetActor),
			Config.TickActivationRadius,
			Config.TickDeactivationRadius);
		return false;
	}

	if (!IsTickVisibilityGateConfigValid(Config))
	{
		UE_LOG(
			LogPRWorldTickOptimizer,
			Warning,
			TEXT("[WorldTickOptimizer] Tick 내부 반경 설정 오류 Target=%s InnerActivation=%.1f InnerDeactivation=%.1f OuterActivation=%.1f OuterDeactivation=%.1f"),
			*GetNameSafe(TargetActor),
			Config.TickAlwaysActiveActivationRadius,
			Config.TickAlwaysActiveDeactivationRadius,
			Config.TickActivationRadius,
			Config.TickDeactivationRadius);
	}

	if (Config.VisibilityActivationRadius <= 0.0f || Config.VisibilityDeactivationRadius < Config.VisibilityActivationRadius)
	{
		UE_LOG(
			LogPRWorldTickOptimizer,
			Warning,
			TEXT("[WorldTickOptimizer] Visibility 반경 설정 오류 Target=%s Activation=%.1f Deactivation=%.1f"),
			*GetNameSafe(TargetActor),
			Config.VisibilityActivationRadius,
			Config.VisibilityDeactivationRadius);
		return false;
	}

	OutEntry.TargetActor = TargetActor;
	OutEntry.TargetInterface.SetObject(TargetActor);
	OutEntry.TargetInterface.SetInterface(TargetInterface);
	OutEntry.Config = Config;
	OutEntry.bIsTickActive = Config.bStartTickActive || TargetInterface->IsTickActive();
	OutEntry.bIsVisibilityActive = Config.bStartVisibilityActive || TargetInterface->IsVisibilityActive();
	return true;
}

/*~ 거리 평가 ~*/

void UPRWorldTickOptimizerSubsystem::BuildSources()
{
	CachedSources.Reset();

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		const APlayerController* PlayerController = Iterator->Get();
		if (!IsValid(PlayerController))
		{
			continue;
		}

		APawn* PlayerPawn = PlayerController->GetPawn();
		if (!IsValid(PlayerPawn) || UPRCombatStatics::GetActorTeam(PlayerPawn) != EPRTeam::Player)
		{
			continue;
		}

		const UAbilitySystemComponent* AbilitySystemComponent = UPRCombatStatics::FindAbilitySystemComponent(PlayerPawn);
		if (IsValid(AbilitySystemComponent) && AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Dead))
		{
			continue;
		}

		FPRTickOptimizationSource Source;
		Source.Location = PlayerPawn->GetActorLocation();
		Source.SourceActor = PlayerPawn;
		CachedSources.Add(Source);
	}
}

void UPRWorldTickOptimizerSubsystem::EvaluateTargets()
{
	if (ActiveEntries.IsEmpty())
	{
		NextEvaluationIndex = 0;
		return;
	}

	if (MaxTargetsPerEvaluation <= 0 || MaxTargetsPerEvaluation >= ActiveEntries.Num())
	{
		for (FPRTickOptimizationEntry& Entry : ActiveEntries)
		{
			EvaluateTarget(Entry);
		}

		NextEvaluationIndex = 0;
		return;
	}

	const int32 EvaluationCount = FMath::Min(MaxTargetsPerEvaluation, ActiveEntries.Num());
	for (int32 Count = 0; Count < EvaluationCount; ++Count)
	{
		if (!ActiveEntries.IsValidIndex(NextEvaluationIndex))
		{
			NextEvaluationIndex = 0;
		}

		EvaluateTarget(ActiveEntries[NextEvaluationIndex]);
		NextEvaluationIndex = (NextEvaluationIndex + 1) % ActiveEntries.Num();
	}
}

void UPRWorldTickOptimizerSubsystem::RestoreAllTargetsActive()
{
	for (FPRTickOptimizationEntry& Entry : ActiveEntries)
	{
		AActor* TargetActor = Entry.TargetActor.Get();
		IPRTickOptimizable* TargetInterface = Entry.TargetInterface.GetInterface();
		if (!IsValid(TargetActor) || TargetInterface == nullptr)
		{
			continue;
		}

		if (!Entry.bIsTickActive)
		{
			Entry.bIsTickActive = true;
			TargetInterface->SetTickActive(true);
		}

		Entry.DebugState = EPRTickOptimizationDebugState::Active;

		if (!Entry.bIsVisibilityActive)
		{
			Entry.bIsVisibilityActive = true;
			TargetInterface->SetVisibilityActive(true);
		}
	}

	NextEvaluationIndex = 0;
}

// 대상 항목의 Tick과 Visibility 활성 상태 적용
void UPRWorldTickOptimizerSubsystem::ApplyEntryActiveState(FPRTickOptimizationEntry& Entry, bool bTickActive, bool bVisibilityActive)
{
	AActor* TargetActor = Entry.TargetActor.Get();
	IPRTickOptimizable* TargetInterface = Entry.TargetInterface.GetInterface();
	if (!IsValid(TargetActor) || TargetInterface == nullptr)
	{
		return;
	}

	if (Entry.bIsTickActive != bTickActive)
	{
		Entry.bIsTickActive = bTickActive;
		TargetInterface->SetTickActive(bTickActive);
	}

	if (Entry.bIsVisibilityActive != bVisibilityActive)
	{
		Entry.bIsVisibilityActive = bVisibilityActive;
		TargetInterface->SetVisibilityActive(bVisibilityActive);
	}
}

void UPRWorldTickOptimizerSubsystem::EvaluateTarget(FPRTickOptimizationEntry& Entry)
{
	AActor* TargetActor = Entry.TargetActor.Get();
	IPRTickOptimizable* TargetInterface = Entry.TargetInterface.GetInterface();
	if (!IsValid(TargetActor) || TargetInterface == nullptr)
	{
		return;
	}

	if (Entry.Config.bAllowTargetRuntimeEvaluationGate && !TargetInterface->CanOptimizeTick())
	{
		return;
	}

	if (Entry.bForceActive)
	{
		// 강제 활성 대상의 거리 기반 비활성화 차단
		Entry.DebugState = EPRTickOptimizationDebugState::Active;
		ApplyEntryActiveState(Entry, true, true);
		return;
	}

	const FVector TargetLocation = TargetInterface->GetTickLocation();
	bool bShouldTickBeActive = false;
	if (IsTickVisibilityGateEnabled() && IsTickVisibilityGateConfigValid(Entry.Config))
	{
		bShouldTickBeActive = EvaluateTickVisibilityGate(Entry, TargetLocation);
	}
	else
	{
		bShouldTickBeActive = Entry.bIsTickActive
			? HasSourceInsideRadius(TargetLocation, Entry.Config.TickDeactivationRadius)
			: HasSourceInsideRadius(TargetLocation, Entry.Config.TickActivationRadius);
		Entry.DebugState = bShouldTickBeActive
			? EPRTickOptimizationDebugState::Active
			: EPRTickOptimizationDebugState::OutsideRadius;
	}

	const bool bShouldVisibilityBeActive = Entry.bIsVisibilityActive
		? HasSourceInsideRadius(TargetLocation, Entry.Config.VisibilityDeactivationRadius)
		: HasSourceInsideRadius(TargetLocation, Entry.Config.VisibilityActivationRadius);

	ApplyEntryActiveState(Entry, bShouldTickBeActive, bShouldVisibilityBeActive);
}

// 활성 목록의 대상 항목 검색
FPRTickOptimizationEntry* UPRWorldTickOptimizerSubsystem::FindEntry(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return nullptr;
	}

	return ActiveEntries.FindByPredicate(
		[TargetActor](const FPRTickOptimizationEntry& Entry)
		{
			return Entry.TargetActor.Get() == TargetActor;
		});
}

bool UPRWorldTickOptimizerSubsystem::HasSourceInsideRadius(const FVector& TargetLocation, float Radius) const
{
	const float RadiusSquared = FMath::Square(Radius);
	for (const FPRTickOptimizationSource& Source : CachedSources)
	{
		if (FVector::DistSquared(TargetLocation, Source.Location) <= RadiusSquared)
		{
			return true;
		}
	}

	return false;
}

bool UPRWorldTickOptimizerSubsystem::EvaluateTickVisibilityGate(FPRTickOptimizationEntry& Entry, const FVector& TargetLocation)
{
	struct FPRVisibilityGateSourceCandidate
	{
		// Visibility trace 대상 플레이어 기준점
		const FPRTickOptimizationSource* Source = nullptr;

		// 가까운 기준점 우선 검사용 거리 제곱
		float DistanceSquared = 0.0f;
	};

	AActor* TargetActor = Entry.TargetActor.Get();
	const UWorld* World = GetWorld();
	if (!IsValid(TargetActor) || !IsValid(World))
	{
		return Entry.bIsTickActive;
	}

	const float InnerRadius = Entry.bIsTickActive
		? Entry.Config.TickAlwaysActiveDeactivationRadius
		: Entry.Config.TickAlwaysActiveActivationRadius;
	const float OuterRadius = Entry.bIsTickActive
		? Entry.Config.TickDeactivationRadius
		: Entry.Config.TickActivationRadius;
	const float InnerRadiusSquared = FMath::Square(InnerRadius);
	const float OuterRadiusSquared = FMath::Square(OuterRadius);

	TArray<FPRVisibilityGateSourceCandidate, TInlineAllocator<4>> TraceCandidates;
	for (const FPRTickOptimizationSource& Source : CachedSources)
	{
		const float DistanceSquared = FVector::DistSquared(TargetLocation, Source.Location);
		if (DistanceSquared <= InnerRadiusSquared)
		{
			Entry.DebugState = EPRTickOptimizationDebugState::Active;
			return true;
		}

		if (DistanceSquared <= OuterRadiusSquared)
		{
			FPRVisibilityGateSourceCandidate Candidate;
			Candidate.Source = &Source;
			Candidate.DistanceSquared = DistanceSquared;
			TraceCandidates.Add(Candidate);
		}
	}

	if (TraceCandidates.IsEmpty())
	{
		Entry.DebugState = EPRTickOptimizationDebugState::OutsideRadius;
		return false;
	}

	TraceCandidates.Sort(
		[](const FPRVisibilityGateSourceCandidate& Left, const FPRVisibilityGateSourceCandidate& Right)
		{
			return Left.DistanceSquared < Right.DistanceSquared;
		});

	for (const FPRVisibilityGateSourceCandidate& Candidate : TraceCandidates)
	{
		if (Candidate.Source == nullptr)
		{
			continue;
		}

		if (!TryConsumeVisibilityGateTraceBudget())
		{
			// Trace 예산 소진 시 비활성 전환 보류
			return Entry.bIsTickActive;
		}

		// 플레이어와 대상 자체를 제외한 월드 차폐물 검사
		FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(PRWorldTickOptimizerVisibilityGate), false);
		TraceParams.AddIgnoredActor(TargetActor);
		AActor* SourceActor = Candidate.Source->SourceActor.Get();
		if (IsValid(SourceActor))
		{
			TraceParams.AddIgnoredActor(SourceActor);
		}

		FHitResult HitResult;
		const bool bBlocked = World->LineTraceSingleByChannel(
			HitResult,
			Candidate.Source->Location,
			TargetLocation,
			ECC_Visibility,
			TraceParams);
		if (!bBlocked)
		{
			Entry.DebugState = EPRTickOptimizationDebugState::Active;
			return true;
		}
	}

	Entry.DebugState = EPRTickOptimizationDebugState::BlockedByVisibility;
	return false;
}

bool UPRWorldTickOptimizerSubsystem::TryConsumeVisibilityGateTraceBudget()
{
	if (RemainingVisibilityGateTraceBudget < 0)
	{
		return true;
	}

	if (RemainingVisibilityGateTraceBudget <= 0)
	{
		return false;
	}

	--RemainingVisibilityGateTraceBudget;
	return true;
}
