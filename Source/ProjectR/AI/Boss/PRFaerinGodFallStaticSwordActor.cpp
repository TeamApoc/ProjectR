// Copyright ProjectR. All Rights Reserved.

#include "PRFaerinGodFallStaticSwordActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AI/Data/PRFaerinGodFallDataAsset.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PRAssetManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRFaerinGodFallSword, Log, All);

// ===== 생성 =====

APRFaerinGodFallStaticSwordActor::APRFaerinGodFallStaticSwordActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PatternLifeSpan = 0.0f;
	SetReplicateMovement(false);

	StaticSwordMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticSwordMeshComponent"));
	StaticSwordMeshComponent->SetupAttachment(SceneRoot);
	StaticSwordMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// ===== AActor Interface =====

void APRFaerinGodFallStaticSwordActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateSwordVisualPresentation(DeltaSeconds);

	if (!HasAuthority())
	{
		UpdateClientSwordPresentation(DeltaSeconds);
		RefreshSwordTickEnabled();
		return;
	}

	if (SwordState == EPRFaerinGodFallStaticSwordState::MovingToTargetOverhead)
	{
		UpdateTargetOverheadMovement(DeltaSeconds);
		RefreshSwordTickEnabled();
		return;
	}

	if (SwordState == EPRFaerinGodFallStaticSwordState::EntryDiving
		|| SwordState == EPRFaerinGodFallStaticSwordState::EntryDiveReturning
		|| SwordState == EPRFaerinGodFallStaticSwordState::Dropping
		|| SwordState == EPRFaerinGodFallStaticSwordState::Returning)
	{
		UpdateSegmentMovement(DeltaSeconds);
	}

	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRFaerinGodFallStaticSwordActor, SwordState);
	DOREPLIFETIME(APRFaerinGodFallStaticSwordActor, SwordIndex);
	DOREPLIFETIME(APRFaerinGodFallStaticSwordActor, SourceBoneName);
	DOREPLIFETIME(APRFaerinGodFallStaticSwordActor, HomeLocation);
	DOREPLIFETIME(APRFaerinGodFallStaticSwordActor, GodFallData);
}

void APRFaerinGodFallStaticSwordActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearSwordTimers();
	ResetSwordVisualTransform();
	Super::EndPlay(EndPlayReason);
}

// ===== APRBossPatternActor Interface =====

void APRFaerinGodFallStaticSwordActor::CancelPatternActor()
{
	if (!HasAuthority())
	{
		return;
	}

	ClearSwordTimers();
	bImpactWarningSpawned = false;
	ResetSwordVisualTransform();
	SetSwordState(EPRFaerinGodFallStaticSwordState::Cancelled);
	MulticastGodFallSwordCancelled();
	ExpirePatternActor();
}

// ===== 초기화 / 충전 =====

void APRFaerinGodFallStaticSwordActor::InitializeGodFallStaticSword(APRBossBaseCharacter* InOwnerBoss,
	AActor* InPatternTarget,
	UPRFaerinGodFallDataAsset* InGodFallData,
	const int32 InSwordIndex,
	const FName InSourceBoneName,
	const FTransform& InInitialTransform)
{
	if (!HasAuthority())
	{
		return;
	}

	GodFallData = InGodFallData;
	SwordIndex = InSwordIndex;
	SourceBoneName = InSourceBoneName;
	HomeLocation = InInitialTransform.GetLocation();

	SetActorTransform(InInitialTransform, false, nullptr, ETeleportType::TeleportPhysics);
	CaptureMeshBaselineTransform();
	ResetSwordVisualTransform();
	MulticastSetSwordPresentationLocation(InInitialTransform.GetLocation());
	InitializeBossPatternActor(InOwnerBoss, InPatternTarget);
	SetSwordState(EPRFaerinGodFallStaticSwordState::SpawnedFromRigBone);
}

void APRFaerinGodFallStaticSwordActor::BeginCharging(const float ChargeSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	ClearSwordTimers();
	PatternTarget = nullptr;
	bHasAssignedAttackLocation = false;
	bImpactWarningSpawned = false;
	OverheadMoveSpeed = 0.0f;
	ResetSwordVisualTransform();
	ApplySwordPresentationLocation(HomeLocation);
	MulticastSetSwordPresentationLocation(HomeLocation);
	SetSwordState(EPRFaerinGodFallStaticSwordState::Charging);

	if (ChargeSeconds <= 0.0f)
	{
		FinishCharging();
		return;
	}

	GetWorldTimerManager().SetTimer(
		ChargeTimerHandle,
		this,
		&APRFaerinGodFallStaticSwordActor::FinishCharging,
		ChargeSeconds,
		false);
}

void APRFaerinGodFallStaticSwordActor::FinishCharging()
{
	if (!HasAuthority())
	{
		return;
	}

	ApplySwordPresentationLocation(HomeLocation);
	MulticastSetSwordPresentationLocation(HomeLocation);
	ResetSwordVisualTransform();
	SetSwordState(EPRFaerinGodFallStaticSwordState::Charged);
	OnChargeFinished.Broadcast(this);
}

bool APRFaerinGodFallStaticSwordActor::StartEntryDive(const float DiveDistance,
	const float DiveSeconds,
	const float ReturnSeconds,
	const float ChargeSecondsAfterReturn)
{
	if (!HasAuthority()
		|| SwordState == EPRFaerinGodFallStaticSwordState::MovingToTargetOverhead
		|| SwordState == EPRFaerinGodFallStaticSwordState::Telegraph
		|| SwordState == EPRFaerinGodFallStaticSwordState::Dropping
		|| SwordState == EPRFaerinGodFallStaticSwordState::Impact
		|| SwordState == EPRFaerinGodFallStaticSwordState::Returning)
	{
		return false;
	}

	ClearSwordTimers();
	PatternTarget = nullptr;
	bHasAssignedAttackLocation = false;
	bImpactWarningSpawned = false;
	EntryDiveReturnSeconds = FMath::Max(ReturnSeconds, 0.0f);
	EntryDiveChargeSecondsAfterReturn = FMath::Max(ChargeSecondsAfterReturn, 0.0f);

	SegmentStartLocation = GetActorLocation();
	SegmentTargetLocation = HomeLocation - FVector(0.0f, 0.0f, FMath::Max(DiveDistance, 0.0f));
	SegmentElapsedSeconds = 0.0f;
	SegmentDurationSeconds = FMath::Max(DiveSeconds, 0.0f);

	SetSwordState(EPRFaerinGodFallStaticSwordState::EntryDiving);
	MulticastStartSwordPresentationSegment(
		EPRFaerinGodFallStaticSwordState::EntryDiving,
		SegmentStartLocation,
		SegmentTargetLocation,
		SegmentDurationSeconds);
	if (SegmentDurationSeconds <= UE_SMALL_NUMBER)
	{
		FinishEntryDiveDown();
		return true;
	}

	RefreshSwordTickEnabled();
	return true;
}

bool APRFaerinGodFallStaticSwordActor::StartAssignedAttack(AActor* InAssignedTarget,
	const float InWarningSeconds,
	const float InOverheadMoveSeconds)
{
	if (!HasAuthority() || !CanStartAssignedAttack() || !IsValidAssignedTarget(InAssignedTarget))
	{
		return false;
	}

	PatternTarget = InAssignedTarget;

	FVector ResolvedOverheadLocation = FVector::ZeroVector;
	FVector ResolvedImpactLocation = FVector::ZeroVector;
	if (!ResolveAssignedAttackLocations(ResolvedOverheadLocation, ResolvedImpactLocation))
	{
		UE_LOG(LogPRFaerinGodFallSword, Warning,
			TEXT("God Fall sword failed to resolve assigned attack locations. Sword=%s, Target=%s"),
			*GetNameSafe(this),
			*GetNameSafe(InAssignedTarget));
		PatternTarget = nullptr;
		return false;
	}

	ClearSwordTimers();
	bImpactWarningSpawned = false;
	AssignedOverheadLocation = ResolvedOverheadLocation;
	AssignedImpactLocation = ResolvedImpactLocation;
	AssignedWarningSeconds = FMath::Max(InWarningSeconds, 0.0f);
	bHasAssignedAttackLocation = true;

	if (IsValid(GodFallData) && GodFallData->bDrawDebug)
	{
		DrawDebugLine(GetWorld(), HomeLocation, AssignedOverheadLocation, FColor::Cyan, false, FMath::Max(InOverheadMoveSeconds, 0.05f));
		DrawDebugSphere(GetWorld(), AssignedOverheadLocation, 56.0f, 12, FColor::Cyan, false, FMath::Max(InWarningSeconds, 0.05f));
	}

	SegmentStartLocation = GetActorLocation();
	SegmentElapsedSeconds = 0.0f;
	SegmentDurationSeconds = 0.0f;
	OverheadMoveElapsedSeconds = 0.0f;
	OverheadMoveDurationSeconds = FMath::Max(InOverheadMoveSeconds, 0.0f);
	OverheadMoveSpeed = 0.0f;
	if (IsValid(GodFallData))
	{
		const float DurationScale = GodFallData->TargetOverheadMoveSeconds > UE_SMALL_NUMBER
			? OverheadMoveDurationSeconds / GodFallData->TargetOverheadMoveSeconds
			: 1.0f;
		OverheadMoveAcceleration = FMath::Max(GodFallData->TargetOverheadMoveAcceleration, 0.0f)
			/ FMath::Max(DurationScale, 0.01f);
	}
	else
	{
		OverheadMoveAcceleration = 0.0f;
	}

	SetSwordState(EPRFaerinGodFallStaticSwordState::MovingToTargetOverhead);
	MulticastStartSwordTargetOverhead(
		InAssignedTarget,
		AssignedOverheadLocation,
		OverheadMoveDurationSeconds,
		OverheadMoveAcceleration);
	if (OverheadMoveDurationSeconds <= UE_SMALL_NUMBER)
	{
		FinishMoveToTargetOverhead();
		return true;
	}

	RefreshSwordTickEnabled();
	return true;
}

// ===== 복제 / BP 이벤트 =====

void APRFaerinGodFallStaticSwordActor::OnRep_SwordState()
{
	VisualStateElapsedSeconds = 0.0f;
	if (SwordState == EPRFaerinGodFallStaticSwordState::Charging
		|| SwordState == EPRFaerinGodFallStaticSwordState::Charged
		|| SwordState == EPRFaerinGodFallStaticSwordState::Returning
		|| SwordState == EPRFaerinGodFallStaticSwordState::Cancelled
		|| SwordState == EPRFaerinGodFallStaticSwordState::None)
	{
		ResetSwordVisualTransform();
	}
	BP_OnGodFallSwordStateChanged(SwordState);
	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::OnRep_GodFallData()
{
	if (!bMeshBaselineCaptured)
	{
		CaptureMeshBaselineTransform();
	}
	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::MulticastGodFallSwordImpact_Implementation(
	FVector_NetQuantize ImpactLocation,
	FRotator ImpactRotation,
	UNiagaraSystem* NiagaraSystem,
	FVector Scale,
	float LifeSeconds)
{
	SpawnNiagaraAtLocationLocal(NiagaraSystem, ImpactLocation, ImpactRotation, Scale, LifeSeconds);
	BP_OnGodFallSwordImpact();
}

void APRFaerinGodFallStaticSwordActor::MulticastGodFallSwordImpactWarning_Implementation(
	FVector_NetQuantize ImpactLocation,
	FRotator ImpactRotation,
	UNiagaraSystem* NiagaraSystem,
	FVector Scale,
	float LifeSeconds)
{
	SpawnNiagaraAtLocationLocal(NiagaraSystem, ImpactLocation, ImpactRotation, Scale, LifeSeconds);
}

void APRFaerinGodFallStaticSwordActor::MulticastGodFallSwordCancelled_Implementation()
{
	ResetSwordVisualTransform();
	BP_OnGodFallSwordCancelled();
}

// ===== 상태 전이 =====

void APRFaerinGodFallStaticSwordActor::SetSwordState(const EPRFaerinGodFallStaticSwordState NewState)
{
	if (SwordState == NewState)
	{
		return;
	}

	SwordState = NewState;
	VisualStateElapsedSeconds = 0.0f;
	if (SwordState == EPRFaerinGodFallStaticSwordState::Charging
		|| SwordState == EPRFaerinGodFallStaticSwordState::Charged
		|| SwordState == EPRFaerinGodFallStaticSwordState::Returning
		|| SwordState == EPRFaerinGodFallStaticSwordState::Cancelled
		|| SwordState == EPRFaerinGodFallStaticSwordState::None)
	{
		ResetSwordVisualTransform();
	}
	BP_OnGodFallSwordStateChanged(SwordState);
	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::ClearSwordTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChargeTimerHandle);
		World->GetTimerManager().ClearTimer(TelegraphTimerHandle);
		World->GetTimerManager().ClearTimer(ImpactHoldTimerHandle);
		World->GetTimerManager().ClearTimer(ImpactWarningTimerHandle);
	}
}

void APRFaerinGodFallStaticSwordActor::FinishEntryDiveDown()
{
	if (!HasAuthority())
	{
		return;
	}

	ApplySwordPresentationLocation(SegmentTargetLocation);
	ApplyImpactDamage();
	const FRotator ImpactRotation = IsValid(GodFallData)
		? GodFallData->SwordImpactNiagaraRotationOffset
		: FRotator::ZeroRotator;
	UNiagaraSystem* ImpactNiagaraSystem = IsValid(GodFallData) ? GodFallData->SwordImpactNiagaraSystem : nullptr;
	const FVector ImpactNiagaraScale = IsValid(GodFallData) ? GodFallData->SwordImpactNiagaraScale : FVector::OneVector;
	const float ImpactNiagaraLifeSeconds = IsValid(GodFallData) ? GodFallData->SwordImpactNiagaraLifeSeconds : 0.0f;
	MulticastGodFallSwordImpact(SegmentTargetLocation, ImpactRotation, ImpactNiagaraSystem, ImpactNiagaraScale, ImpactNiagaraLifeSeconds);

	SegmentStartLocation = GetActorLocation();
	SegmentTargetLocation = HomeLocation;
	SegmentElapsedSeconds = 0.0f;
	SegmentDurationSeconds = EntryDiveReturnSeconds;

	SetSwordState(EPRFaerinGodFallStaticSwordState::EntryDiveReturning);
	MulticastStartSwordPresentationSegment(
		EPRFaerinGodFallStaticSwordState::EntryDiveReturning,
		SegmentStartLocation,
		SegmentTargetLocation,
		SegmentDurationSeconds);
	if (SegmentDurationSeconds <= UE_SMALL_NUMBER)
	{
		FinishEntryDiveReturn();
		return;
	}

	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::FinishEntryDiveReturn()
{
	if (!HasAuthority())
	{
		return;
	}

	ApplySwordPresentationLocation(HomeLocation);
	MulticastSetSwordPresentationLocation(HomeLocation);
	ResetSwordVisualTransform();
	BeginCharging(EntryDiveChargeSecondsAfterReturn);
}

void APRFaerinGodFallStaticSwordActor::BeginDropping()
{
	if (!HasAuthority() || !IsValid(GodFallData))
	{
		return;
	}

	if (!IsValidAssignedTarget(PatternTarget.Get()))
	{
		BeginReturning();
		return;
	}

	if (!bHasAssignedAttackLocation)
	{
		UE_LOG(LogPRFaerinGodFallSword, Warning,
			TEXT("God Fall sword drop skipped because assigned impact location is missing. Sword=%s, Target=%s"),
			*GetNameSafe(this),
			*GetNameSafe(PatternTarget.Get()));
		return;
	}

	SegmentStartLocation = GetActorLocation();
	SegmentTargetLocation = AssignedImpactLocation;
	SegmentElapsedSeconds = 0.0f;
	SegmentDurationSeconds = FMath::Max(GodFallData->DropSeconds, UE_SMALL_NUMBER);

	SetSwordState(EPRFaerinGodFallStaticSwordState::Dropping);
	MulticastStartSwordPresentationSegment(
		EPRFaerinGodFallStaticSwordState::Dropping,
		SegmentStartLocation,
		SegmentTargetLocation,
		SegmentDurationSeconds);
	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::FinishDrop()
{
	if (!HasAuthority())
	{
		return;
	}

	ApplySwordPresentationLocation(SegmentTargetLocation);
	MulticastSetSwordPresentationLocation(SegmentTargetLocation);
	SetSwordState(EPRFaerinGodFallStaticSwordState::Impact);

	ApplyImpactDamage();
	const FRotator ImpactRotation = IsValid(GodFallData)
		? GodFallData->SwordImpactNiagaraRotationOffset
		: FRotator::ZeroRotator;
	UNiagaraSystem* ImpactNiagaraSystem = IsValid(GodFallData) ? GodFallData->SwordImpactNiagaraSystem : nullptr;
	const FVector ImpactNiagaraScale = IsValid(GodFallData) ? GodFallData->SwordImpactNiagaraScale : FVector::OneVector;
	const float ImpactNiagaraLifeSeconds = IsValid(GodFallData) ? GodFallData->SwordImpactNiagaraLifeSeconds : 0.0f;
	MulticastGodFallSwordImpact(SegmentTargetLocation, ImpactRotation, ImpactNiagaraSystem, ImpactNiagaraScale, ImpactNiagaraLifeSeconds);

	const float HoldSeconds = IsValid(GodFallData) ? GodFallData->ImpactHoldSeconds : 0.0f;
	if (HoldSeconds <= 0.0f)
	{
		BeginReturning();
		return;
	}

	GetWorldTimerManager().SetTimer(
		ImpactHoldTimerHandle,
		this,
		&APRFaerinGodFallStaticSwordActor::BeginReturning,
		HoldSeconds,
		false);
}

void APRFaerinGodFallStaticSwordActor::BeginReturning()
{
	if (!HasAuthority() || !IsValid(GodFallData))
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ImpactWarningTimerHandle);
	SegmentStartLocation = GetActorLocation();
	SegmentTargetLocation = HomeLocation;
	SegmentElapsedSeconds = 0.0f;
	SegmentDurationSeconds = FMath::Max(GodFallData->RiseSeconds, UE_SMALL_NUMBER);

	SetSwordState(EPRFaerinGodFallStaticSwordState::Returning);
	MulticastStartSwordPresentationSegment(
		EPRFaerinGodFallStaticSwordState::Returning,
		SegmentStartLocation,
		SegmentTargetLocation,
		SegmentDurationSeconds);
	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::FinishReturning()
{
	if (!HasAuthority())
	{
		return;
	}

	ApplySwordPresentationLocation(HomeLocation);
	MulticastSetSwordPresentationLocation(HomeLocation);
	ResetSwordVisualTransform();
	RefreshSwordTickEnabled();
	PatternTarget = nullptr;
	OnAssignedAttackFinished.Broadcast(this);
}

// ===== 이동 / 타겟팅 =====

void APRFaerinGodFallStaticSwordActor::UpdateSegmentMovement(const float DeltaSeconds)
{
	if (!IsValid(GodFallData))
	{
		return;
	}

	SegmentElapsedSeconds += DeltaSeconds;
	const float Alpha = FMath::Clamp(SegmentElapsedSeconds / FMath::Max(SegmentDurationSeconds, UE_SMALL_NUMBER), 0.0f, 1.0f);
	ApplySwordPresentationLocation(FMath::Lerp(SegmentStartLocation, SegmentTargetLocation, Alpha));

	if (Alpha >= 1.0f)
	{
		if (SwordState == EPRFaerinGodFallStaticSwordState::EntryDiving)
		{
			FinishEntryDiveDown();
		}
		else if (SwordState == EPRFaerinGodFallStaticSwordState::EntryDiveReturning)
		{
			FinishEntryDiveReturn();
		}
		else if (SwordState == EPRFaerinGodFallStaticSwordState::Dropping)
		{
			FinishDrop();
		}
		else if (SwordState == EPRFaerinGodFallStaticSwordState::Returning)
		{
			FinishReturning();
		}
	}
}

void APRFaerinGodFallStaticSwordActor::UpdateTargetOverheadMovement(const float DeltaSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!IsValidAssignedTarget(PatternTarget.Get()))
	{
		BeginReturning();
		return;
	}

	RefreshAssignedAttackLocations();
	OverheadMoveElapsedSeconds += DeltaSeconds;
	OverheadMoveSpeed += OverheadMoveAcceleration * DeltaSeconds;

	const FVector CurrentLocation = GetActorLocation();
	const FVector ToTarget = AssignedOverheadLocation - CurrentLocation;
	const float Distance = ToTarget.Size();
	if (Distance > UE_SMALL_NUMBER && OverheadMoveSpeed > 0.0f)
	{
		const float MoveDistance = FMath::Min(OverheadMoveSpeed * DeltaSeconds, Distance);
		ApplySwordPresentationLocation(CurrentLocation + ToTarget.GetSafeNormal() * MoveDistance);
	}

	if (OverheadMoveElapsedSeconds >= OverheadMoveDurationSeconds)
	{
		RefreshAssignedAttackLocations();
		FinishMoveToTargetOverhead();
	}
}

bool APRFaerinGodFallStaticSwordActor::RefreshAssignedAttackLocations()
{
	FVector ResolvedOverheadLocation = FVector::ZeroVector;
	FVector ResolvedImpactLocation = FVector::ZeroVector;
	if (!ResolveAssignedAttackLocations(ResolvedOverheadLocation, ResolvedImpactLocation))
	{
		return false;
	}

	AssignedOverheadLocation = ResolvedOverheadLocation;
	AssignedImpactLocation = ResolvedImpactLocation;
	bHasAssignedAttackLocation = true;
	return true;
}

bool APRFaerinGodFallStaticSwordActor::IsValidAssignedTarget(AActor* CandidateTarget) const
{
	if (!IsValid(CandidateTarget))
	{
		return false;
	}

	const UAbilitySystemComponent* TargetAbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CandidateTarget);
	return IsValid(TargetAbilitySystem)
		&& !TargetAbilitySystem->HasMatchingGameplayTag(PRGameplayTags::State_Down)
		&& !TargetAbilitySystem->HasMatchingGameplayTag(PRGameplayTags::State_Dead);
}

void APRFaerinGodFallStaticSwordActor::FinishMoveToTargetOverhead()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!IsValidAssignedTarget(PatternTarget.Get()))
	{
		BeginReturning();
		return;
	}

	ApplySwordPresentationLocation(AssignedOverheadLocation);
	MulticastSetSwordPresentationLocation(AssignedOverheadLocation);
	RefreshSwordTickEnabled();
	BeginTelegraph();
}

void APRFaerinGodFallStaticSwordActor::BeginTelegraph()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!IsValidAssignedTarget(PatternTarget.Get()))
	{
		BeginReturning();
		return;
	}

	ApplySwordPresentationLocation(AssignedOverheadLocation);
	MulticastSetSwordPresentationLocation(AssignedOverheadLocation);
	SetSwordState(EPRFaerinGodFallStaticSwordState::Telegraph);
	ScheduleImpactWarning();

	if (AssignedWarningSeconds <= 0.0f)
	{
		BeginDropping();
		return;
	}

	GetWorldTimerManager().SetTimer(
		TelegraphTimerHandle,
		this,
		&APRFaerinGodFallStaticSwordActor::BeginDropping,
		AssignedWarningSeconds,
		false);
}

bool APRFaerinGodFallStaticSwordActor::ResolveAssignedAttackLocations(FVector& OutOverheadLocation, FVector& OutGroundLocation) const
{
	if (!ResolveAssignedImpactLocation(OutGroundLocation))
	{
		return false;
	}

	OutOverheadLocation = FVector(OutGroundLocation.X, OutGroundLocation.Y, HomeLocation.Z);
	return true;
}

bool APRFaerinGodFallStaticSwordActor::ResolveAssignedImpactLocation(FVector& OutGroundLocation) const
{
	if (!IsValid(PatternTarget))
	{
		return false;
	}

	return ProjectTargetLocationToGround(PatternTarget->GetActorLocation(), OutGroundLocation);
}

bool APRFaerinGodFallStaticSwordActor::ProjectTargetLocationToGround(const FVector& TargetLocation, FVector& OutGroundLocation) const
{
	if (!IsValid(GodFallData))
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FaerinGodFallGroundTrace), false, this);
	QueryParams.AddIgnoredActor(this);
	if (IsValid(OwnerBoss))
	{
		QueryParams.AddIgnoredActor(OwnerBoss);
	}
	if (IsValid(PatternTarget))
	{
		QueryParams.AddIgnoredActor(PatternTarget);
	}

	const FVector TraceStart(TargetLocation.X, TargetLocation.Y, HomeLocation.Z);
	const FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, GodFallData->GroundTraceDownDistance);

	FHitResult HitResult;
	if (!World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, GodFallData->GroundTraceChannel, QueryParams))
	{
		return false;
	}

	OutGroundLocation = HitResult.ImpactPoint;

	if (GodFallData->bDrawDebug)
	{
		DrawDebugLine(World, TraceStart, TraceEnd, FColor::Blue, false, 1.0f);
		DrawDebugSphere(World, OutGroundLocation, GodFallData->ImpactRadius, 16, FColor::Red, false, 1.0f);
	}

	return true;
}

// ===== 피해 =====

void APRFaerinGodFallStaticSwordActor::ApplySwordPresentationLocation(const FVector& Location)
{
	SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
}

void APRFaerinGodFallStaticSwordActor::SpawnNiagaraAtLocationLocal(UNiagaraSystem* NiagaraSystem,
	const FVector& Location,
	const FRotator& Rotation,
	const FVector& Scale,
	const float LifeSeconds) const
{
	if (!IsValid(NiagaraSystem))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		NiagaraSystem,
		Location,
		Rotation,
		Scale,
		true,
		true);
	if (!IsValid(NiagaraComponent) || LifeSeconds <= UE_SMALL_NUMBER)
	{
		return;
	}

	TWeakObjectPtr<UNiagaraComponent> WeakNiagaraComponent = NiagaraComponent;
	FTimerHandle CleanupTimerHandle;
	World->GetTimerManager().SetTimer(
		CleanupTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [WeakNiagaraComponent]()
		{
			if (UNiagaraComponent* ActiveNiagaraComponent = WeakNiagaraComponent.Get())
			{
				ActiveNiagaraComponent->Deactivate();
				ActiveNiagaraComponent->DestroyComponent();
			}
		}),
		LifeSeconds,
		false);
}

void APRFaerinGodFallStaticSwordActor::CaptureMeshBaselineTransform()
{
	if (!IsValid(StaticSwordMeshComponent))
	{
		return;
	}

	MeshBaselineRelativeTransform = StaticSwordMeshComponent->GetRelativeTransform();
	bMeshBaselineCaptured = true;
}

void APRFaerinGodFallStaticSwordActor::ResetSwordVisualTransform()
{
	if (!IsValid(StaticSwordMeshComponent))
	{
		return;
	}

	if (!bMeshBaselineCaptured)
	{
		CaptureMeshBaselineTransform();
	}

	StaticSwordMeshComponent->SetRelativeTransform(MeshBaselineRelativeTransform);
}

void APRFaerinGodFallStaticSwordActor::UpdateSwordVisualPresentation(const float DeltaSeconds)
{
	if (!IsValid(StaticSwordMeshComponent))
	{
		return;
	}

	if (!bMeshBaselineCaptured)
	{
		CaptureMeshBaselineTransform();
	}

	if (!ShouldTickSwordVisual())
	{
		ResetSwordVisualTransform();
		return;
	}

	VisualElapsedSeconds += DeltaSeconds;
	VisualStateElapsedSeconds += DeltaSeconds;

	// 적용 순서: Baseline -> HoverLocationDelta -> ChargeShakeLocation/RotationDelta -> ImpactSlantRotationDelta -> FinalRelativeTransform.
	FVector FinalLocation = MeshBaselineRelativeTransform.GetLocation();
	FQuat FinalRotation = MeshBaselineRelativeTransform.GetRotation();
	const FVector FinalScale = MeshBaselineRelativeTransform.GetScale3D();

	FinalLocation += ResolveHoverLocationDelta();

	FVector ChargeShakeLocationDelta = FVector::ZeroVector;
	FRotator ChargeShakeRotationDelta = FRotator::ZeroRotator;
	ResolveChargeShakeDelta(ChargeShakeLocationDelta, ChargeShakeRotationDelta);
	FinalLocation += ChargeShakeLocationDelta;
	FinalRotation = FinalRotation * ChargeShakeRotationDelta.Quaternion();

	FinalRotation = FinalRotation * ResolveImpactSlantRotationDelta().Quaternion();

	StaticSwordMeshComponent->SetRelativeTransform(FTransform(FinalRotation, FinalLocation, FinalScale));
}

bool APRFaerinGodFallStaticSwordActor::ShouldTickSwordVisual() const
{
	if (!IsValid(GodFallData) || !IsValid(StaticSwordMeshComponent))
	{
		return false;
	}

	const EPRFaerinGodFallStaticSwordState VisualState = bClientSwordPresentationActive
		? ClientPresentationState
		: SwordState;

	const bool bCanHover = GodFallData->bEnableStaticSwordHoverVisual
		&& (VisualState == EPRFaerinGodFallStaticSwordState::Charging
			|| VisualState == EPRFaerinGodFallStaticSwordState::Charged
			|| VisualState == EPRFaerinGodFallStaticSwordState::MovingToTargetOverhead
			|| VisualState == EPRFaerinGodFallStaticSwordState::Telegraph);
	const bool bCanChargeShake = GodFallData->bEnableChargeShakeVisual
		&& VisualState == EPRFaerinGodFallStaticSwordState::Charging;
	const bool bCanSlant = GodFallData->bEnableImpactVisualSlant
		&& (VisualState == EPRFaerinGodFallStaticSwordState::Dropping
			|| VisualState == EPRFaerinGodFallStaticSwordState::Impact);

	return bCanHover || bCanChargeShake || bCanSlant;
}

bool APRFaerinGodFallStaticSwordActor::ShouldTickSwordMovement() const
{
	if (!HasAuthority())
	{
		return bClientSwordPresentationActive;
	}

	return SwordState == EPRFaerinGodFallStaticSwordState::EntryDiving
		|| SwordState == EPRFaerinGodFallStaticSwordState::EntryDiveReturning
		|| SwordState == EPRFaerinGodFallStaticSwordState::MovingToTargetOverhead
		|| SwordState == EPRFaerinGodFallStaticSwordState::Dropping
		|| SwordState == EPRFaerinGodFallStaticSwordState::Returning;
}

void APRFaerinGodFallStaticSwordActor::RefreshSwordTickEnabled()
{
	SetActorTickEnabled(ShouldTickSwordMovement() || ShouldTickSwordVisual());
}

FVector APRFaerinGodFallStaticSwordActor::ResolveHoverLocationDelta() const
{
	if (!IsValid(GodFallData) || !GodFallData->bEnableStaticSwordHoverVisual)
	{
		return FVector::ZeroVector;
	}

	const EPRFaerinGodFallStaticSwordState VisualState = bClientSwordPresentationActive
		? ClientPresentationState
		: SwordState;
	if (VisualState != EPRFaerinGodFallStaticSwordState::Charging
		&& VisualState != EPRFaerinGodFallStaticSwordState::Charged
		&& VisualState != EPRFaerinGodFallStaticSwordState::MovingToTargetOverhead
		&& VisualState != EPRFaerinGodFallStaticSwordState::Telegraph)
	{
		return FVector::ZeroVector;
	}

	const float Phase = VisualElapsedSeconds * 2.0f * PI * GodFallData->HoverFrequencyHz
		+ static_cast<float>(SwordIndex) * GodFallData->HoverPhaseOffsetPerSword;
	return FVector(
		FMath::Cos(Phase) * GodFallData->HoverLateralAmplitude,
		FMath::Sin(Phase * 0.67f) * GodFallData->HoverLateralAmplitude,
		FMath::Sin(Phase) * GodFallData->HoverAmplitudeZ);
}

void APRFaerinGodFallStaticSwordActor::ResolveChargeShakeDelta(FVector& OutLocationDelta, FRotator& OutRotationDelta) const
{
	OutLocationDelta = FVector::ZeroVector;
	OutRotationDelta = FRotator::ZeroRotator;

	const EPRFaerinGodFallStaticSwordState VisualState = bClientSwordPresentationActive
		? ClientPresentationState
		: SwordState;
	if (!IsValid(GodFallData)
		|| !GodFallData->bEnableChargeShakeVisual
		|| VisualState != EPRFaerinGodFallStaticSwordState::Charging)
	{
		return;
	}

	const float RampAlpha = GodFallData->ChargeShakeRampInSeconds > UE_SMALL_NUMBER
		? FMath::Clamp(VisualStateElapsedSeconds / GodFallData->ChargeShakeRampInSeconds, 0.0f, 1.0f)
		: 1.0f;
	const float Phase = VisualElapsedSeconds * 2.0f * PI * GodFallData->ChargeShakeFrequencyHz
		+ static_cast<float>(SwordIndex) * GodFallData->ChargeShakePhaseOffsetPerSword;

	OutLocationDelta = FVector(
		FMath::Sin(Phase) * GodFallData->ChargeShakeAmplitudeLocation.X,
		FMath::Sin(Phase * 1.31f) * GodFallData->ChargeShakeAmplitudeLocation.Y,
		FMath::Cos(Phase * 0.73f) * GodFallData->ChargeShakeAmplitudeLocation.Z) * RampAlpha;
	OutRotationDelta = FRotator(
		FMath::Sin(Phase * 0.89f) * GodFallData->ChargeShakeAmplitudeRotation.Pitch,
		FMath::Sin(Phase * 1.17f) * GodFallData->ChargeShakeAmplitudeRotation.Yaw,
		FMath::Cos(Phase * 1.07f) * GodFallData->ChargeShakeAmplitudeRotation.Roll) * RampAlpha;
}

FRotator APRFaerinGodFallStaticSwordActor::ResolveImpactSlantRotationDelta() const
{
	if (!IsValid(GodFallData) || !GodFallData->bEnableImpactVisualSlant)
	{
		return FRotator::ZeroRotator;
	}

	const float Alpha = ResolveImpactSlantAlpha();
	if (Alpha <= UE_SMALL_NUMBER)
	{
		return FRotator::ZeroRotator;
	}

	const uint32 Seed = GetTypeHash(SourceBoneName) ^ static_cast<uint32>((SwordIndex + 17) * 196613);
	FRandomStream RandomStream(static_cast<int32>(Seed));
	const float MinYaw = FMath::Min(GodFallData->ImpactSlantRandomYawMinDegrees, GodFallData->ImpactSlantRandomYawMaxDegrees);
	const float MaxYaw = FMath::Max(GodFallData->ImpactSlantRandomYawMinDegrees, GodFallData->ImpactSlantRandomYawMaxDegrees);
	const float RandomYaw = RandomStream.FRandRange(MinYaw, MaxYaw);

	return FRotator(
		GodFallData->ImpactSlantPitchDegrees * Alpha,
		RandomYaw * Alpha,
		GodFallData->ImpactSlantRollDegrees * Alpha);
}

float APRFaerinGodFallStaticSwordActor::ResolveImpactSlantAlpha() const
{
	const EPRFaerinGodFallStaticSwordState VisualState = bClientSwordPresentationActive
		? ClientPresentationState
		: SwordState;

	if (VisualState == EPRFaerinGodFallStaticSwordState::Dropping)
	{
		const float ElapsedSeconds = bClientSwordPresentationActive ? ClientSegmentElapsedSeconds : SegmentElapsedSeconds;
		const float DurationSeconds = bClientSwordPresentationActive ? ClientSegmentDurationSeconds : SegmentDurationSeconds;
		const float DropAlpha = FMath::Clamp(ElapsedSeconds / FMath::Max(DurationSeconds, UE_SMALL_NUMBER), 0.0f, 1.0f);
		const float StartAlpha = FMath::Clamp(IsValid(GodFallData) ? GodFallData->ImpactSlantBlendInStartAlpha : 0.0f, 0.0f, 1.0f);
		return DropAlpha <= StartAlpha
			? 0.0f
			: FMath::Clamp((DropAlpha - StartAlpha) / FMath::Max(1.0f - StartAlpha, UE_SMALL_NUMBER), 0.0f, 1.0f);
	}

	if (VisualState == EPRFaerinGodFallStaticSwordState::Impact)
	{
		return 1.0f;
	}

	return 0.0f;
}

void APRFaerinGodFallStaticSwordActor::ScheduleImpactWarning()
{
	if (!HasAuthority()
		|| bImpactWarningSpawned
		|| !bHasAssignedAttackLocation
		|| !IsValid(GodFallData)
		|| !IsValid(GodFallData->ImpactWarningNiagaraSystem))
	{
		return;
	}

	const float TimeUntilImpact = FMath::Max(AssignedWarningSeconds, 0.0f) + FMath::Max(GodFallData->DropSeconds, 0.0f);
	const float WarningDelaySeconds = FMath::Max(TimeUntilImpact - FMath::Max(GodFallData->ImpactWarningLeadSeconds, 0.0f), 0.0f);
	if (WarningDelaySeconds <= UE_SMALL_NUMBER)
	{
		SpawnImpactWarning();
		return;
	}

	GetWorldTimerManager().SetTimer(
		ImpactWarningTimerHandle,
		this,
		&APRFaerinGodFallStaticSwordActor::SpawnImpactWarning,
		WarningDelaySeconds,
		false);
}

void APRFaerinGodFallStaticSwordActor::SpawnImpactWarning()
{
	if (!HasAuthority()
		|| bImpactWarningSpawned
		|| !bHasAssignedAttackLocation
		|| !IsValid(GodFallData)
		|| !IsValid(GodFallData->ImpactWarningNiagaraSystem))
	{
		return;
	}

	bImpactWarningSpawned = true;
	MulticastGodFallSwordImpactWarning(
		AssignedImpactLocation,
		GodFallData->ImpactWarningNiagaraRotationOffset,
		GodFallData->ImpactWarningNiagaraSystem,
		GodFallData->ImpactWarningNiagaraScale,
		GodFallData->ImpactWarningNiagaraLifeSeconds);
}

void APRFaerinGodFallStaticSwordActor::UpdateClientSwordPresentation(const float DeltaSeconds)
{
	if (!bClientSwordPresentationActive)
	{
		RefreshSwordTickEnabled();
		return;
	}

	if (ClientPresentationState == EPRFaerinGodFallStaticSwordState::MovingToTargetOverhead)
	{
		UpdateClientTargetOverheadMovement(DeltaSeconds);
		return;
	}

	UpdateClientSegmentMovement(DeltaSeconds);
}

void APRFaerinGodFallStaticSwordActor::UpdateClientSegmentMovement(const float DeltaSeconds)
{
	ClientSegmentElapsedSeconds += DeltaSeconds;
	const float Alpha = FMath::Clamp(
		ClientSegmentElapsedSeconds / FMath::Max(ClientSegmentDurationSeconds, UE_SMALL_NUMBER),
		0.0f,
		1.0f);

	SetActorLocation(FMath::Lerp(ClientSegmentStartLocation, ClientSegmentTargetLocation, Alpha), false, nullptr, ETeleportType::TeleportPhysics);

	if (Alpha >= 1.0f)
	{
		bClientSwordPresentationActive = false;
		ClientPresentationState = EPRFaerinGodFallStaticSwordState::None;
		RefreshSwordTickEnabled();
	}
}

void APRFaerinGodFallStaticSwordActor::UpdateClientTargetOverheadMovement(const float DeltaSeconds)
{
	ClientOverheadMoveElapsedSeconds += DeltaSeconds;
	ClientOverheadMoveSpeed += ClientOverheadMoveAcceleration * DeltaSeconds;

	if (AActor* AssignedTarget = ClientAssignedTarget.Get())
	{
		const FVector TargetLocation = AssignedTarget->GetActorLocation();
		ClientAssignedOverheadLocation = FVector(TargetLocation.X, TargetLocation.Y, HomeLocation.Z);
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector ToTarget = ClientAssignedOverheadLocation - CurrentLocation;
	const float Distance = ToTarget.Size();
	if (Distance > UE_SMALL_NUMBER && ClientOverheadMoveSpeed > 0.0f)
	{
		const float MoveDistance = FMath::Min(ClientOverheadMoveSpeed * DeltaSeconds, Distance);
		SetActorLocation(CurrentLocation + ToTarget.GetSafeNormal() * MoveDistance, false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (ClientOverheadMoveElapsedSeconds >= ClientOverheadMoveDurationSeconds)
	{
		SetClientSwordPresentationLocation(ClientAssignedOverheadLocation);
	}
}

void APRFaerinGodFallStaticSwordActor::StartClientSwordPresentationSegment(
	const EPRFaerinGodFallStaticSwordState NewState,
	const FVector& StartLocation,
	const FVector& TargetLocation,
	const float DurationSeconds)
{
	ClientPresentationState = NewState;
	ClientSegmentStartLocation = StartLocation;
	ClientSegmentTargetLocation = TargetLocation;
	ClientSegmentElapsedSeconds = 0.0f;
	ClientSegmentDurationSeconds = FMath::Max(DurationSeconds, 0.0f);
	bClientSwordPresentationActive = true;

	SetActorLocation(StartLocation, false, nullptr, ETeleportType::TeleportPhysics);

	if (ClientSegmentDurationSeconds <= UE_SMALL_NUMBER)
	{
		SetClientSwordPresentationLocation(TargetLocation);
		return;
	}

	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::StartClientSwordTargetOverhead(
	AActor* AssignedTarget,
	const FVector& InitialOverheadLocation,
	const float MoveDurationSeconds,
	const float MoveAcceleration)
{
	ClientAssignedTarget = AssignedTarget;
	ClientAssignedOverheadLocation = InitialOverheadLocation;
	ClientOverheadMoveElapsedSeconds = 0.0f;
	ClientOverheadMoveDurationSeconds = FMath::Max(MoveDurationSeconds, 0.0f);
	ClientOverheadMoveSpeed = 0.0f;
	ClientOverheadMoveAcceleration = FMath::Max(MoveAcceleration, 0.0f);
	ClientPresentationState = EPRFaerinGodFallStaticSwordState::MovingToTargetOverhead;
	bClientSwordPresentationActive = true;

	if (ClientOverheadMoveDurationSeconds <= UE_SMALL_NUMBER)
	{
		SetClientSwordPresentationLocation(InitialOverheadLocation);
		return;
	}

	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::SetClientSwordPresentationLocation(const FVector& Location)
{
	SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	bClientSwordPresentationActive = false;
	ClientPresentationState = EPRFaerinGodFallStaticSwordState::None;
	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::MulticastSetSwordPresentationLocation_Implementation(FVector Location)
{
	if (HasAuthority())
	{
		return;
	}

	SetClientSwordPresentationLocation(Location);
}

void APRFaerinGodFallStaticSwordActor::MulticastStartSwordPresentationSegment_Implementation(
	EPRFaerinGodFallStaticSwordState NewState,
	FVector StartLocation,
	FVector TargetLocation,
	float DurationSeconds)
{
	if (HasAuthority())
	{
		return;
	}

	StartClientSwordPresentationSegment(NewState, StartLocation, TargetLocation, DurationSeconds);
}

void APRFaerinGodFallStaticSwordActor::MulticastStartSwordTargetOverhead_Implementation(
	AActor* AssignedTarget,
	FVector InitialOverheadLocation,
	float MoveDurationSeconds,
	float MoveAcceleration)
{
	if (HasAuthority())
	{
		return;
	}

	StartClientSwordTargetOverhead(AssignedTarget, InitialOverheadLocation, MoveDurationSeconds, MoveAcceleration);
}

void APRFaerinGodFallStaticSwordActor::ApplyImpactDamage()
{
	if (!HasAuthority() || !IsValid(GodFallData) || !IsValid(OwnerBoss))
	{
		return;
	}

	UPRAbilitySystemComponent* SourceASC = OwnerBoss->GetEnemyAbilitySystemComponent();
	if (!IsValid(SourceASC))
	{
		return;
	}

	TSubclassOf<UGameplayEffect> ResolvedDamageEffectClass = GodFallData->ImpactDamageEffectClass;
	if (!IsValid(ResolvedDamageEffectClass))
	{
		const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
		if (IsValid(Registry))
		{
			ResolvedDamageEffectClass = Registry->DamageGE_FromEnemy;
		}
	}

	if (!IsValid(ResolvedDamageEffectClass))
	{
		UE_LOG(LogPRFaerinGodFallSword, Warning,
			TEXT("God Fall sword damage skipped because damage effect is missing. Sword=%s"),
			*GetNameSafe(this));
		return;
	}

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FaerinGodFallImpactOverlap), false, this);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwnerBoss);

	const FCollisionShape CollisionShape = FCollisionShape::MakeSphere(FMath::Max(GodFallData->ImpactRadius, 0.0f));
	const bool bOverlapped = World->OverlapMultiByChannel(
		OverlapResults,
		GetActorLocation(),
		FQuat::Identity,
		GodFallData->ImpactOverlapChannel,
		CollisionShape,
		QueryParams);

	if (GodFallData->bDrawDebug)
	{
		DrawDebugSphere(World, GetActorLocation(), GodFallData->ImpactRadius, 18, FColor::Orange, false, 1.0f);
	}

	if (!bOverlapped)
	{
		return;
	}

	TSet<TWeakObjectPtr<AActor>> DamagedActors;
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* TargetActor = OverlapResult.GetActor();
		if (!IsValid(TargetActor)
			|| DamagedActors.Contains(TargetActor)
			|| UPRCombatStatics::GetActorTeam(TargetActor) == EPRTeam::Enemy)
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (!IsValid(TargetASC))
		{
			continue;
		}

		FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
			ResolvedDamageEffectClass,
			1.0f,
			EffectContext);

		if (!SpecHandle.IsValid())
		{
			continue;
		}

		SpecHandle.Data->SetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_AttackMultiplier,
			GodFallData->ImpactDamageMultiplier);

		SpecHandle.Data->SetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_PoiseDamage,
			GodFallData->ImpactPoiseDamage);

		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		DamagedActors.Add(TargetActor);
	}
}
