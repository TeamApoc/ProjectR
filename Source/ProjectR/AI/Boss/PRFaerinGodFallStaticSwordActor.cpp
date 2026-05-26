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
#include "TimerManager.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AI/Data/PRFaerinGodFallDataAsset.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Combat/PRCombatStatics.h"
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

	if (!HasAuthority())
	{
		UpdateClientSwordPresentation(DeltaSeconds);
		return;
	}

	if (SwordState == EPRFaerinGodFallStaticSwordState::MovingToTargetOverhead)
	{
		UpdateTargetOverheadMovement(DeltaSeconds);
		return;
	}

	if (SwordState == EPRFaerinGodFallStaticSwordState::EntryDiving
		|| SwordState == EPRFaerinGodFallStaticSwordState::EntryDiveReturning
		|| SwordState == EPRFaerinGodFallStaticSwordState::Dropping
		|| SwordState == EPRFaerinGodFallStaticSwordState::Returning)
	{
		UpdateSegmentMovement(DeltaSeconds);
	}
}

void APRFaerinGodFallStaticSwordActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRFaerinGodFallStaticSwordActor, SwordState);
	DOREPLIFETIME(APRFaerinGodFallStaticSwordActor, SwordIndex);
	DOREPLIFETIME(APRFaerinGodFallStaticSwordActor, SourceBoneName);
	DOREPLIFETIME(APRFaerinGodFallStaticSwordActor, HomeLocation);
}

void APRFaerinGodFallStaticSwordActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearSwordTimers();
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
	SetActorTickEnabled(false);
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
	OverheadMoveSpeed = 0.0f;
	SetActorTickEnabled(false);
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

	SetActorTickEnabled(true);
	return true;
}

bool APRFaerinGodFallStaticSwordActor::StartAssignedAttack(AActor* InAssignedTarget,
	const float InWarningSeconds,
	const float InOverheadMoveSeconds)
{
	if (!HasAuthority() || !CanStartAssignedAttack() || !IsValid(InAssignedTarget))
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

	SetActorTickEnabled(true);
	return true;
}

// ===== 복제 / BP 이벤트 =====

void APRFaerinGodFallStaticSwordActor::OnRep_SwordState()
{
	BP_OnGodFallSwordStateChanged(SwordState);
}

void APRFaerinGodFallStaticSwordActor::MulticastGodFallSwordImpact_Implementation()
{
	BP_OnGodFallSwordImpact();
}

void APRFaerinGodFallStaticSwordActor::MulticastGodFallSwordCancelled_Implementation()
{
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
	BP_OnGodFallSwordStateChanged(SwordState);
}

void APRFaerinGodFallStaticSwordActor::ClearSwordTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChargeTimerHandle);
		World->GetTimerManager().ClearTimer(TelegraphTimerHandle);
		World->GetTimerManager().ClearTimer(ImpactHoldTimerHandle);
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
	MulticastGodFallSwordImpact();

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

	SetActorTickEnabled(true);
}

void APRFaerinGodFallStaticSwordActor::FinishEntryDiveReturn()
{
	if (!HasAuthority())
	{
		return;
	}

	ApplySwordPresentationLocation(HomeLocation);
	MulticastSetSwordPresentationLocation(HomeLocation);
	SetActorTickEnabled(false);
	BeginCharging(EntryDiveChargeSecondsAfterReturn);
}

void APRFaerinGodFallStaticSwordActor::BeginDropping()
{
	if (!HasAuthority() || !IsValid(GodFallData))
	{
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
	SetActorTickEnabled(true);
}

void APRFaerinGodFallStaticSwordActor::FinishDrop()
{
	if (!HasAuthority())
	{
		return;
	}

	ApplySwordPresentationLocation(SegmentTargetLocation);
	MulticastSetSwordPresentationLocation(SegmentTargetLocation);
	SetActorTickEnabled(false);
	SetSwordState(EPRFaerinGodFallStaticSwordState::Impact);

	ApplyImpactDamage();
	MulticastGodFallSwordImpact();

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
	SetActorTickEnabled(true);
}

void APRFaerinGodFallStaticSwordActor::FinishReturning()
{
	if (!HasAuthority())
	{
		return;
	}

	ApplySwordPresentationLocation(HomeLocation);
	MulticastSetSwordPresentationLocation(HomeLocation);
	SetActorTickEnabled(false);
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

void APRFaerinGodFallStaticSwordActor::FinishMoveToTargetOverhead()
{
	if (!HasAuthority())
	{
		return;
	}

	ApplySwordPresentationLocation(AssignedOverheadLocation);
	MulticastSetSwordPresentationLocation(AssignedOverheadLocation);
	SetActorTickEnabled(false);
	BeginTelegraph();
}

void APRFaerinGodFallStaticSwordActor::BeginTelegraph()
{
	if (!HasAuthority())
	{
		return;
	}

	ApplySwordPresentationLocation(AssignedOverheadLocation);
	MulticastSetSwordPresentationLocation(AssignedOverheadLocation);
	SetSwordState(EPRFaerinGodFallStaticSwordState::Telegraph);

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

void APRFaerinGodFallStaticSwordActor::UpdateClientSwordPresentation(const float DeltaSeconds)
{
	if (!bClientSwordPresentationActive)
	{
		SetActorTickEnabled(false);
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
		SetActorTickEnabled(false);
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

	SetActorTickEnabled(true);
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

	SetActorTickEnabled(true);
}

void APRFaerinGodFallStaticSwordActor::SetClientSwordPresentationLocation(const FVector& Location)
{
	SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	bClientSwordPresentationActive = false;
	ClientPresentationState = EPRFaerinGodFallStaticSwordState::None;
	SetActorTickEnabled(false);
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
