// Copyright ProjectR. All Rights Reserved.

#include "PRWorldTickOptimizerSubsystem.h"

#include "AbilitySystemComponent.h"
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
		CachedSources.Reset();
		return;
	}

	BuildSources();
	EvaluateTargets();
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

		if (!Entry.bIsVisibilityActive)
		{
			Entry.bIsVisibilityActive = true;
			TargetInterface->SetVisibilityActive(true);
		}
	}

	NextEvaluationIndex = 0;
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

	const FVector TargetLocation = TargetInterface->GetTickLocation();
	const bool bShouldTickBeActive = Entry.bIsTickActive
		? HasSourceInsideRadius(TargetLocation, Entry.Config.TickDeactivationRadius)
		: HasSourceInsideRadius(TargetLocation, Entry.Config.TickActivationRadius);

	if (Entry.bIsTickActive != bShouldTickBeActive)
	{
		Entry.bIsTickActive = bShouldTickBeActive;
		TargetInterface->SetTickActive(bShouldTickBeActive);
	}

	const bool bShouldVisibilityBeActive = Entry.bIsVisibilityActive
		? HasSourceInsideRadius(TargetLocation, Entry.Config.VisibilityDeactivationRadius)
		: HasSourceInsideRadius(TargetLocation, Entry.Config.VisibilityActivationRadius);

	if (Entry.bIsVisibilityActive != bShouldVisibilityBeActive)
	{
		Entry.bIsVisibilityActive = bShouldVisibilityBeActive;
		TargetInterface->SetVisibilityActive(bShouldVisibilityBeActive);
	}
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
