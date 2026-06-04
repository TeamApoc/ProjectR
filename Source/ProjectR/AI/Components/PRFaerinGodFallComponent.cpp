// Copyright ProjectR. All Rights Reserved.

#include "PRFaerinGodFallComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"
#include "ProjectR/AI/Boss/PRFaerinGodFallStaticSwordActor.h"
#include "ProjectR/AI/Data/PRFaerinGodFallDataAsset.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/PRGameplayTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRFaerinGodFall, Log, All);

// ===== 생성 =====

UPRFaerinGodFallComponent::UPRFaerinGodFallComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

// ===== UActorComponent Interface =====

void UPRFaerinGodFallComponent::BeginPlay()
{
	Super::BeginPlay();

	SetComponentTickEnabled(false);
	if (bHideRigActorUntilEntry && IsValid(PlacedSwordRigActor))
	{
		SetPlacedRigHidden(true);
	}
}

void UPRFaerinGodFallComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CleanupGodFallBodyNiagaraLocal();
	CancelSwordRiseCameraShakeLocal();
	CancelGodFallEntry();
	CancelGodFallHazards();
	Super::EndPlay(EndPlayReason);
}

void UPRFaerinGodFallComponent::TickComponent(const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (IsValid(OwnerBoss) && !OwnerBoss->HasAuthority())
	{
		UpdateClientBossPresentation(DeltaTime);
		return;
	}

	switch (EntryRuntimeState)
	{
	case EPRFaerinGodFallEntryRuntimeState::MovingToCastLocation:
		UpdateMoveToCastLocation(DeltaTime);
		break;
	case EPRFaerinGodFallEntryRuntimeState::ChargeRising:
		UpdateBossChargePresentation(DeltaTime);
		break;
	case EPRFaerinGodFallEntryRuntimeState::TiltHolding:
		HoldBossAtApex();
		break;
	case EPRFaerinGodFallEntryRuntimeState::BossDropWaiting:
		HoldBossAtApex();
		break;
	case EPRFaerinGodFallEntryRuntimeState::BossFastDropping:
		UpdateBossFastDropPresentation(DeltaTime);
		break;
	case EPRFaerinGodFallEntryRuntimeState::BossDropGroundHolding:
		HoldBossAtCastGroundLocation();
		break;
	default:
		break;
	}
}

// ===== Entry =====

bool UPRFaerinGodFallComponent::StartGodFallEntry(AActor* InPatternTarget)
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss) || !OwnerBoss->HasAuthority())
	{
		return false;
	}

	if (bGodFallEntryRunning || bGodFallConvertedToStaticSwords)
	{
		UE_LOG(LogPRFaerinGodFall, Warning,
			TEXT("God Fall entry cannot start because it is already running or converted. Owner=%s"),
			*GetNameSafe(GetOwner()));
		return false;
	}

	ActivePatternTargets.Reset();
	NextTargetAssignmentIndex = 0;
	RefreshGodFallTargets(InPatternTarget);
	ActiveStaticSwords.Reset();

	if (!ValidateGodFallEntryInputs())
	{
		return false;
	}

	BeginBossPresentationReplicationOverride();

	bGodFallEntryRunning = true;
	bGodFallConvertedToStaticSwords = false;
	bBodyDropMontageStarted = false;
	bBodyMontageSequenceFinished = false;
	EntryRuntimeState = EPRFaerinGodFallEntryRuntimeState::Idle;
	CurrentGodFallPhase = EPRBossPhase::Phase2;

	StartMovingToCastLocation();
	return true;
}

void UPRFaerinGodFallComponent::CancelGodFallEntry()
{
	if (!bGodFallEntryRunning && EntryRuntimeState == EPRFaerinGodFallEntryRuntimeState::Idle)
	{
		return;
	}

	ClearRigTimers();
	MulticastCleanupGodFallBodyNiagara();
	MulticastCancelSwordRiseCameraShake();
	bGodFallEntryRunning = false;
	EntryRuntimeState = EPRFaerinGodFallEntryRuntimeState::Idle;
	SetComponentTickEnabled(false);
	EndBossPresentationReplicationOverride();

	if (!bGodFallConvertedToStaticSwords && bHideRigActorUntilEntry && IsValid(PlacedSwordRigActor))
	{
		if (AActor* OwnerActor = GetOwner(); IsValid(OwnerActor) && OwnerActor->HasAuthority())
		{
			MulticastSetPlacedRigHidden(true);
		}
		else
		{
			SetPlacedRigHidden(true);
		}
	}
}

void UPRFaerinGodFallComponent::CancelGodFallHazards()
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss) || !OwnerBoss->HasAuthority())
	{
		return;
	}

	TArray<TObjectPtr<APRFaerinGodFallStaticSwordActor>> SwordsToCancel = ActiveStaticSwords;
	ActiveStaticSwords.Reset();
	ActivePatternTargets.Reset();
	NextTargetAssignmentIndex = 0;

	for (APRFaerinGodFallStaticSwordActor* StaticSword : SwordsToCancel)
	{
		if (IsValid(StaticSword))
		{
			StaticSword->CancelPatternActor();
		}
	}
}

void UPRFaerinGodFallComponent::ApplyGodFallPhaseScaling(const EPRBossPhase Phase)
{
	CurrentGodFallPhase = Phase;
	TryAssignNextSword();
}

void UPRFaerinGodFallComponent::ConvertRigToStaticSwords()
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss) || !OwnerBoss->HasAuthority() || !bGodFallEntryRunning)
	{
		return;
	}

	if (!ValidateGodFallEntryInputs())
	{
		BroadcastEntryFinished(false);
		return;
	}

	USkeletalMeshComponent* RigMeshComponent = ResolveRigMeshComponent();
	if (!IsValid(RigMeshComponent))
	{
		BroadcastEntryFinished(false);
		return;
	}

	ActiveStaticSwords.Reset();
	RefreshGodFallTargets();

	for (int32 BoneArrayIndex = 0; BoneArrayIndex < GodFallData->SwordBoneNames.Num(); ++BoneArrayIndex)
	{
		const FName BoneName = GodFallData->SwordBoneNames[BoneArrayIndex];
		if (!SpawnStaticSwordFromBone(BoneArrayIndex, BoneName))
		{
			CancelGodFallHazards();
			BroadcastEntryFinished(false);
			return;
		}
	}

	bGodFallConvertedToStaticSwords = true;
	DestroyPlacedRigActor();
	BroadcastEntryFinished(!ActiveStaticSwords.IsEmpty());
}

void UPRFaerinGodFallComponent::NotifyGodFallBodyDropMontageStarted()
{
	bBodyDropMontageStarted = true;

	if (EntryRuntimeState == EPRFaerinGodFallEntryRuntimeState::TiltHolding)
	{
		BeginBossDropPresentation();
	}
}

void UPRFaerinGodFallComponent::NotifyGodFallBodyMontageSequenceFinished()
{
	bBodyMontageSequenceFinished = true;

	if (EntryRuntimeState == EPRFaerinGodFallEntryRuntimeState::BossDropGroundHolding
		|| EntryRuntimeState == EPRFaerinGodFallEntryRuntimeState::TiltHolding)
	{
		MulticastCleanupGodFallBodyNiagara();
		EntryRuntimeState = EPRFaerinGodFallEntryRuntimeState::HazardActive;
		SetComponentTickEnabled(false);
		EndBossPresentationReplicationOverride();
	}
}

// ===== 시전 위치 / 본체 연출 =====

void UPRFaerinGodFallComponent::StartMovingToCastLocation()
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss) || !IsValid(GodFallData))
	{
		BroadcastEntryFinished(false);
		return;
	}

	BossCastGroundLocation = ResolveGodFallCastLocation();
	BossCastRotation = ResolveGodFallCastRotation();
	const float DistanceToCastLocation = FVector::Dist(OwnerBoss->GetActorLocation(), BossCastGroundLocation);
	if (DistanceToCastLocation <= GodFallData->BossCastAcceptanceRadius)
	{
		ApplyBossPresentationTransform(BossCastGroundLocation, BossCastRotation);
		MulticastSetBossPresentationTransform(BossCastGroundLocation, BossCastRotation, false);
		StartGodFallCast();
		return;
	}

	EntryRuntimeState = EPRFaerinGodFallEntryRuntimeState::MovingToCastLocation;
	const float MoveDurationSeconds = DistanceToCastLocation / FMath::Max(GodFallData->BossCastMoveSpeed, UE_SMALL_NUMBER);
	MulticastStartBossPresentationSegment(
		EPRFaerinGodFallEntryRuntimeState::MovingToCastLocation,
		OwnerBoss->GetActorLocation(),
		BossCastGroundLocation,
		OwnerBoss->GetActorRotation(),
		BossCastRotation,
		MoveDurationSeconds);
	SetComponentTickEnabled(true);
}

void UPRFaerinGodFallComponent::UpdateMoveToCastLocation(const float DeltaTime)
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss) || !IsValid(GodFallData))
	{
		BroadcastEntryFinished(false);
		return;
	}

	const FVector CurrentLocation = OwnerBoss->GetActorLocation();
	const FVector ToCastLocation = BossCastGroundLocation - CurrentLocation;
	const float Distance = ToCastLocation.Size();
	if (Distance <= GodFallData->BossCastAcceptanceRadius)
	{
		ApplyBossPresentationTransform(BossCastGroundLocation, BossCastRotation);
		MulticastSetBossPresentationTransform(BossCastGroundLocation, BossCastRotation, false);
		StartGodFallCast();
		return;
	}

	const float MoveDistance = FMath::Min(GodFallData->BossCastMoveSpeed * DeltaTime, Distance);
	const FVector MoveDirection = ToCastLocation.GetSafeNormal();
	const FVector NextLocation = CurrentLocation + MoveDirection * MoveDistance;
	ApplyBossPresentationTransform(NextLocation, BossCastRotation);
}

void UPRFaerinGodFallComponent::StartGodFallCast()
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss) || !IsValid(GodFallData))
	{
		BroadcastEntryFinished(false);
		return;
	}

	if (!ValidateGodFallEntryInputs())
	{
		BroadcastEntryFinished(false);
		return;
	}

	if (IsValid(PlacedSwordRigActor))
	{
		MulticastSetPlacedRigHidden(false);
	}

	ApplyBossPresentationTransform(BossCastGroundLocation, BossCastRotation);
	BossPresentationElapsedSeconds = 0.0f;
	BossChargeApexLocation = BossCastGroundLocation + FVector(0.0f, 0.0f, GodFallData->BossChargeRiseHeight);
	BossChargeStartRotation = BossCastRotation;
	BossChargeApexRotation = BossChargeStartRotation;
	BossChargeApexRotation.Yaw += GodFallData->BossChargeYawDegrees;
	MulticastStartBossPresentationSegment(
		EPRFaerinGodFallEntryRuntimeState::ChargeRising,
		BossCastGroundLocation,
		BossChargeApexLocation,
		BossChargeStartRotation,
		BossChargeApexRotation,
		FMath::Max(GodFallData->BossChargeRiseSeconds, UE_SMALL_NUMBER));
	MulticastStartGodFallBodyNiagaraCues();
	if (IsValid(GodFallData->SwordRiseCameraShakeClass))
	{
		MulticastStartSwordRiseCameraShake(
			GodFallData->SwordRiseCameraShakeClass,
			GodFallData->SwordRiseCameraShakeDelaySeconds,
			GodFallData->SwordRiseCameraShakeScale,
			GodFallData->SwordRiseCameraShakeDurationOverride);
	}
	EntryRuntimeState = EPRFaerinGodFallEntryRuntimeState::ChargeRising;
	SetComponentTickEnabled(true);

	if (!PlayRigAnimation(
		GodFallData->RigChargeAnimation,
		GodFallData->RigChargePlayRate,
		FTimerDelegate::CreateUObject(this, &UPRFaerinGodFallComponent::HandleRigChargeFinished)))
	{
		BroadcastEntryFinished(false);
		return;
	}

	OnGodFallCastStarted.Broadcast();
}

void UPRFaerinGodFallComponent::UpdateBossChargePresentation(const float DeltaTime)
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss) || !IsValid(GodFallData))
	{
		return;
	}

	BossPresentationElapsedSeconds += DeltaTime;
	const float Alpha = FMath::Clamp(
		BossPresentationElapsedSeconds / FMath::Max(GodFallData->BossChargeRiseSeconds, UE_SMALL_NUMBER),
		0.0f,
		1.0f);

	const FVector NextLocation = FMath::Lerp(BossCastGroundLocation, BossChargeApexLocation, Alpha);
	FRotator NextRotation = BossChargeStartRotation;
	NextRotation.Yaw = BossChargeStartRotation.Yaw + GodFallData->BossChargeYawDegrees * Alpha;
	ApplyBossPresentationTransform(NextLocation, NextRotation);
}

void UPRFaerinGodFallComponent::HoldBossAtApex()
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss))
	{
		return;
	}

	ApplyBossPresentationTransform(BossChargeApexLocation, BossChargeApexRotation);
}

void UPRFaerinGodFallComponent::BeginBossDropPresentation()
{
	BossPresentationElapsedSeconds = 0.0f;
	EntryRuntimeState = EPRFaerinGodFallEntryRuntimeState::BossDropWaiting;
	SetComponentTickEnabled(true);
	HoldBossAtApex();

	if (!IsValid(GodFallData))
	{
		return;
	}

	ClearRigTimers();
	const float DropDelaySeconds = FMath::Max(GodFallData->BossDropSeconds, 0.0f);
	if (DropDelaySeconds <= 0.0f)
	{
		ExecuteBossFastDrop();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			BossDropTimerHandle,
			this,
			&UPRFaerinGodFallComponent::ExecuteBossFastDrop,
			DropDelaySeconds,
			false);
	}
}

void UPRFaerinGodFallComponent::ExecuteBossFastDrop()
{
	StartStaticSwordEntryDive();
	BossPresentationElapsedSeconds = 0.0f;
	EntryRuntimeState = EPRFaerinGodFallEntryRuntimeState::BossFastDropping;
	SetComponentTickEnabled(true);

	const float FastDropSeconds = IsValid(GodFallData)
		? FMath::Max(GodFallData->BossFastDropSeconds, 0.0f)
		: 0.0f;
	MulticastStartBossPresentationSegment(
		EPRFaerinGodFallEntryRuntimeState::BossFastDropping,
		BossChargeApexLocation,
		BossCastGroundLocation,
		BossChargeApexRotation,
		BossChargeApexRotation,
		FMath::Max(FastDropSeconds, 0.0f));
	if (FastDropSeconds <= UE_SMALL_NUMBER)
	{
		FinishBossDropPresentation();
	}
}

void UPRFaerinGodFallComponent::UpdateBossFastDropPresentation(const float DeltaTime)
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss) || !IsValid(GodFallData))
	{
		return;
	}

	BossPresentationElapsedSeconds += DeltaTime;
	const float Alpha = FMath::Clamp(
		BossPresentationElapsedSeconds / FMath::Max(GodFallData->BossFastDropSeconds, UE_SMALL_NUMBER),
		0.0f,
		1.0f);

	const FVector NextLocation = FMath::Lerp(BossChargeApexLocation, BossCastGroundLocation, Alpha);
	ApplyBossPresentationTransform(NextLocation, BossChargeApexRotation);

	if (Alpha >= 1.0f)
	{
		FinishBossDropPresentation();
	}
}

void UPRFaerinGodFallComponent::UpdateClientBossPresentation(const float DeltaTime)
{
	if (!bClientBossPresentationActive)
	{
		SetComponentTickEnabled(false);
		return;
	}

	if (bClientBossPresentationHolding)
	{
		ApplyBossPresentationTransform(ClientBossSegmentTargetLocation, ClientBossSegmentTargetRotation);
		return;
	}

	ClientBossSegmentElapsedSeconds += DeltaTime;
	const float Alpha = FMath::Clamp(
		ClientBossSegmentElapsedSeconds / FMath::Max(ClientBossSegmentDurationSeconds, UE_SMALL_NUMBER),
		0.0f,
		1.0f);

	const FVector NextLocation = FMath::Lerp(ClientBossSegmentStartLocation, ClientBossSegmentTargetLocation, Alpha);
	const FRotator NextRotation = FMath::Lerp(ClientBossSegmentStartRotation, ClientBossSegmentTargetRotation, Alpha);
	ApplyBossPresentationTransform(NextLocation, NextRotation);

	if (Alpha >= 1.0f)
	{
		bClientBossPresentationActive = false;
		ClientBossPresentationState = EPRFaerinGodFallEntryRuntimeState::Idle;
		SetComponentTickEnabled(false);
	}
}

void UPRFaerinGodFallComponent::FinishBossDropPresentation()
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (IsValid(OwnerBoss))
	{
		ApplyBossPresentationTransform(BossCastGroundLocation, BossChargeApexRotation);
		MulticastSetBossPresentationTransform(BossCastGroundLocation, BossChargeApexRotation, true);
	}

	if (bBodyMontageSequenceFinished)
	{
		MulticastCleanupGodFallBodyNiagara();
		EntryRuntimeState = EPRFaerinGodFallEntryRuntimeState::HazardActive;
		SetComponentTickEnabled(false);
		EndBossPresentationReplicationOverride();
		return;
	}

	EntryRuntimeState = EPRFaerinGodFallEntryRuntimeState::BossDropGroundHolding;
	SetComponentTickEnabled(true);
}

void UPRFaerinGodFallComponent::HoldBossAtCastGroundLocation()
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss))
	{
		return;
	}

	ApplyBossPresentationTransform(BossCastGroundLocation, BossChargeApexRotation);
}

FVector UPRFaerinGodFallComponent::ResolveGodFallCastLocation() const
{
	if (IsValid(GodFallCastAnchorActor))
	{
		return GodFallCastAnchorActor->GetActorLocation();
	}

	const APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (bUseHomeLocationAsCastLocation && IsValid(OwnerBoss))
	{
		return OwnerBoss->GetHomeLocation();
	}

	return IsValid(OwnerBoss) ? OwnerBoss->GetActorLocation() : FVector::ZeroVector;
}

// ===== 내부 처리 =====

FRotator UPRFaerinGodFallComponent::ResolveGodFallCastRotation() const
{
	return FRotator(0.0f, GodFallCastYawDegrees, 0.0f);
}

APRBossBaseCharacter* UPRFaerinGodFallComponent::GetOwnerBoss() const
{
	return Cast<APRBossBaseCharacter>(GetOwner());
}

USkeletalMeshComponent* UPRFaerinGodFallComponent::ResolveRigMeshComponent()
{
	if (IsValid(PlacedSwordRigMeshComponent))
	{
		return PlacedSwordRigMeshComponent;
	}

	if (!IsValid(PlacedSwordRigActor))
	{
		return nullptr;
	}

	PlacedSwordRigMeshComponent = PlacedSwordRigActor->FindComponentByClass<USkeletalMeshComponent>();
	return PlacedSwordRigMeshComponent;
}

bool UPRFaerinGodFallComponent::ValidateGodFallEntryInputs()
{
	if (!IsValid(GodFallData))
	{
		UE_LOG(LogPRFaerinGodFall, Warning,
			TEXT("God Fall data asset is missing. Owner=%s"),
			*GetNameSafe(GetOwner()));
		return false;
	}

	if (GodFallData->SwordBoneNames.Num() != 10)
	{
		UE_LOG(LogPRFaerinGodFall, Warning,
			TEXT("God Fall requires exactly 10 sword bone names. Owner=%s, Count=%d"),
			*GetNameSafe(GetOwner()),
			GodFallData->SwordBoneNames.Num());
		return false;
	}

	if (!IsValid(GodFallData->StaticSwordActorClass))
	{
		UE_LOG(LogPRFaerinGodFall, Warning,
			TEXT("God Fall static sword actor class is missing. Owner=%s"),
			*GetNameSafe(GetOwner()));
		return false;
	}

	if (!IsValid(GodFallData->RigChargeAnimation) || !IsValid(GodFallData->RigTiltPullAnimation))
	{
		UE_LOG(LogPRFaerinGodFall, Warning,
			TEXT("God Fall rig animations are missing. Owner=%s"),
			*GetNameSafe(GetOwner()));
		return false;
	}

	USkeletalMeshComponent* RigMeshComponent = ResolveRigMeshComponent();
	if (!IsValid(RigMeshComponent))
	{
		UE_LOG(LogPRFaerinGodFall, Warning,
			TEXT("God Fall placed sword rig mesh component is missing. Owner=%s, RigActor=%s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(PlacedSwordRigActor.Get()));
		return false;
	}

	for (const FName BoneName : GodFallData->SwordBoneNames)
	{
		if (BoneName == NAME_None || RigMeshComponent->GetBoneIndex(BoneName) == INDEX_NONE)
		{
			UE_LOG(LogPRFaerinGodFall, Warning,
				TEXT("God Fall rig bone is invalid. Owner=%s, Bone=%s"),
				*GetNameSafe(GetOwner()),
				*BoneName.ToString());
			return false;
		}
	}

	return true;
}

bool UPRFaerinGodFallComponent::PlayRigAnimation(UAnimSequenceBase* Animation,
	const float PlayRate,
	FTimerDelegate FinishDelegate)
{
	USkeletalMeshComponent* RigMeshComponent = ResolveRigMeshComponent();
	if (!IsValid(RigMeshComponent) || !IsValid(Animation))
	{
		return false;
	}

	const float ResolvedPlayRate = FMath::Max(PlayRate, 0.01f);
	PlayRigAnimationLocal(Animation, ResolvedPlayRate);
	MulticastPlayRigAnimation(Animation, ResolvedPlayRate);

	const float Duration = Animation->GetPlayLength() / ResolvedPlayRate;
	if (Duration <= 0.0f)
	{
		FinishDelegate.ExecuteIfBound();
		return true;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	FTimerHandle& TimerHandle = Animation == GodFallData->RigChargeAnimation
		? RigChargeTimerHandle
		: RigTiltPullTimerHandle;
	World->GetTimerManager().SetTimer(
		TimerHandle,
		FinishDelegate,
		Duration,
		false);
	return true;
}

void UPRFaerinGodFallComponent::PlayRigAnimationLocal(UAnimSequenceBase* Animation, const float PlayRate)
{
	USkeletalMeshComponent* RigMeshComponent = ResolveRigMeshComponent();
	if (!IsValid(RigMeshComponent) || !IsValid(Animation))
	{
		return;
	}

	RigMeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	RigMeshComponent->PlayAnimation(Animation, false);
	RigMeshComponent->SetPlayRate(FMath::Max(PlayRate, 0.01f));
}

void UPRFaerinGodFallComponent::HandleRigChargeFinished()
{
	if (!bGodFallEntryRunning || !IsValid(GodFallData))
	{
		return;
	}

	HoldBossAtApex();
	EntryRuntimeState = EPRFaerinGodFallEntryRuntimeState::TiltHolding;
	MulticastSetBossPresentationTransform(BossChargeApexLocation, BossChargeApexRotation, true);

	if (!PlayRigAnimation(
		GodFallData->RigTiltPullAnimation,
		GodFallData->RigTiltPullPlayRate,
		FTimerDelegate::CreateUObject(this, &UPRFaerinGodFallComponent::HandleRigTiltPullFinished)))
	{
		BroadcastEntryFinished(false);
	}
}

void UPRFaerinGodFallComponent::HandleRigTiltPullFinished()
{
	ConvertRigToStaticSwords();

	if (bBodyDropMontageStarted
		&& EntryRuntimeState == EPRFaerinGodFallEntryRuntimeState::TiltHolding)
	{
		BeginBossDropPresentation();
	}
}

bool UPRFaerinGodFallComponent::SpawnStaticSwordFromBone(const int32 BoneArrayIndex, const FName BoneName)
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	UWorld* World = GetWorld();
	USkeletalMeshComponent* RigMeshComponent = ResolveRigMeshComponent();
	if (!IsValid(OwnerBoss) || !IsValid(World) || !IsValid(RigMeshComponent) || !IsValid(GodFallData))
	{
		return false;
	}

	const int32 BoneIndex = RigMeshComponent->GetBoneIndex(BoneName);
	if (BoneIndex == INDEX_NONE)
	{
		return false;
	}

	const FTransform BoneTransform = RigMeshComponent->GetBoneTransform(BoneIndex);
	const FTransform SpawnTransform(FQuat::Identity, BoneTransform.GetLocation(), FVector::OneVector);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = OwnerBoss;
	SpawnParameters.Instigator = OwnerBoss;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APRFaerinGodFallStaticSwordActor* StaticSword = World->SpawnActor<APRFaerinGodFallStaticSwordActor>(
		GodFallData->StaticSwordActorClass,
		SpawnTransform,
		SpawnParameters);

	if (!IsValid(StaticSword))
	{
		UE_LOG(LogPRFaerinGodFall, Warning,
			TEXT("God Fall static sword spawn failed. Owner=%s, Bone=%s"),
			*GetNameSafe(GetOwner()),
			*BoneName.ToString());
		return false;
	}

	StaticSword->InitializeGodFallStaticSword(
		OwnerBoss,
		nullptr,
		GodFallData,
		BoneArrayIndex,
		BoneName,
		SpawnTransform);
	StaticSword->OnChargeFinished.AddUObject(this, &UPRFaerinGodFallComponent::HandleStaticSwordChargeFinished);
	StaticSword->OnAssignedAttackFinished.AddUObject(this, &UPRFaerinGodFallComponent::HandleStaticSwordAssignedAttackFinished);
	ActiveStaticSwords.Add(StaticSword);
	return true;
}

// ===== StaticSword 충전 / 배정 =====

void UPRFaerinGodFallComponent::StartStaticSwordEntryDive()
{
	ClearInvalidStaticSwordRefs();
	if (!IsValid(GodFallData))
	{
		return;
	}

	for (APRFaerinGodFallStaticSwordActor* StaticSword : ActiveStaticSwords)
	{
		if (IsValid(StaticSword))
		{
			StaticSword->StartEntryDive(
				GodFallData->EntrySwordDiveDistance,
				GodFallData->BossFastDropSeconds,
				GodFallData->EntrySwordDiveReturnSeconds,
				ResolveSwordChargeSeconds());
		}
	}
}

void UPRFaerinGodFallComponent::HandleStaticSwordChargeFinished(APRFaerinGodFallStaticSwordActor* FinishedSword)
{
	if (!IsValid(FinishedSword))
	{
		return;
	}

	TryAssignNextSword();
}

void UPRFaerinGodFallComponent::HandleStaticSwordAssignedAttackFinished(APRFaerinGodFallStaticSwordActor* FinishedSword)
{
	if (!IsValid(FinishedSword))
	{
		return;
	}

	FinishedSword->BeginCharging(ResolveSwordChargeSeconds());
	TryAssignNextSword();
}

void UPRFaerinGodFallComponent::TryAssignNextSword()
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss) || !OwnerBoss->HasAuthority())
	{
		return;
	}

	RefreshGodFallTargets();
	ClearInvalidStaticSwordRefs();

	if (ActivePatternTargets.IsEmpty())
	{
		return;
	}

	TArray<APRFaerinGodFallStaticSwordActor*> ChargedSwords;
	for (APRFaerinGodFallStaticSwordActor* StaticSword : ActiveStaticSwords)
	{
		if (IsValid(StaticSword) && StaticSword->CanStartAssignedAttack())
		{
			ChargedSwords.Add(StaticSword);
		}
	}

	if (ChargedSwords.IsEmpty())
	{
		return;
	}

	const int32 TargetCount = ActivePatternTargets.Num();
	const int32 StartTargetIndex = NextTargetAssignmentIndex % TargetCount;
	for (int32 TargetOffset = 0; TargetOffset < TargetCount; ++TargetOffset)
	{
		const int32 TargetIndex = (StartTargetIndex + TargetOffset) % TargetCount;
		AActor* TargetActor = ActivePatternTargets[TargetIndex].Get();
		if (!IsValid(TargetActor) || IsTargetAlreadyAssigned(TargetActor))
		{
			continue;
		}

		if (ChargedSwords.IsEmpty())
		{
			return;
		}

		const int32 SelectedIndex = FMath::RandRange(0, ChargedSwords.Num() - 1);
		APRFaerinGodFallStaticSwordActor* SelectedSword = ChargedSwords[SelectedIndex];
		ChargedSwords.RemoveAtSwap(SelectedIndex);
		if (IsValid(SelectedSword))
		{
			const bool bAssigned = SelectedSword->StartAssignedAttack(
				TargetActor,
				ResolveWarningSeconds(),
				ResolveTargetOverheadMoveSeconds());
			if (bAssigned)
			{
				AdvanceTargetAssignmentCursor(TargetIndex);
			}
		}
	}
}

void UPRFaerinGodFallComponent::RefreshGodFallTargets(AActor* FallbackTarget)
{
	ActivePatternTargets.Reset();

	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		for (TActorIterator<APRPlayerCharacter> It(World); It; ++It)
		{
			APRPlayerCharacter* PlayerCharacter = *It;
			if (IsValidGodFallTarget(PlayerCharacter))
			{
				ActivePatternTargets.Add(PlayerCharacter);
			}
		}
	}

	if (ActivePatternTargets.IsEmpty() && IsValidGodFallTarget(FallbackTarget))
	{
		ActivePatternTargets.Add(FallbackTarget);
	}

	if (ActivePatternTargets.IsEmpty())
	{
		NextTargetAssignmentIndex = 0;
		return;
	}

	NextTargetAssignmentIndex %= ActivePatternTargets.Num();
}

bool UPRFaerinGodFallComponent::IsValidGodFallTarget(AActor* CandidateTarget) const
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

bool UPRFaerinGodFallComponent::IsTargetAlreadyAssigned(const AActor* CandidateTarget) const
{
	if (!IsValid(CandidateTarget))
	{
		return true;
	}

	for (const APRFaerinGodFallStaticSwordActor* StaticSword : ActiveStaticSwords)
	{
		if (IsValid(StaticSword) && StaticSword->GetPatternTarget() == CandidateTarget)
		{
			return true;
		}
	}

	return false;
}

void UPRFaerinGodFallComponent::AdvanceTargetAssignmentCursor(const int32 AssignedTargetIndex)
{
	const int32 TargetCount = ActivePatternTargets.Num();
	if (TargetCount <= 0)
	{
		NextTargetAssignmentIndex = 0;
		return;
	}

	NextTargetAssignmentIndex = (AssignedTargetIndex + 1) % TargetCount;
}

float UPRFaerinGodFallComponent::ResolveSwordChargeSeconds() const
{
	if (!IsValid(GodFallData))
	{
		return 1.0f;
	}

	const float MinSeconds = FMath::Max(GodFallData->Phase2SwordChargeMinSeconds, 0.0f);
	const float MaxSeconds = FMath::Max(GodFallData->Phase2SwordChargeMaxSeconds, MinSeconds);
	return FMath::FRandRange(MinSeconds, MaxSeconds) * ResolvePhaseTimingScale();
}

float UPRFaerinGodFallComponent::ResolveTargetOverheadMoveSeconds() const
{
	return IsValid(GodFallData)
		? FMath::Max(GodFallData->TargetOverheadMoveSeconds * ResolvePhaseTimingScale(), 0.0f)
		: 0.0f;
}

float UPRFaerinGodFallComponent::ResolveWarningSeconds() const
{
	return IsValid(GodFallData)
		? FMath::Max(GodFallData->WarningSeconds * ResolvePhaseTimingScale(), 0.0f)
		: 0.0f;
}

float UPRFaerinGodFallComponent::ResolvePhaseTimingScale() const
{
	if (!IsValid(GodFallData))
	{
		return 1.0f;
	}

	switch (CurrentGodFallPhase)
	{
	case EPRBossPhase::Phase3:
		return FMath::Max(GodFallData->Phase3GodFallTimingScale, 0.01f);
	case EPRBossPhase::Phase4:
		return FMath::Max(GodFallData->Phase4GodFallTimingScale, 0.01f);
	default:
		return 1.0f;
	}
}

void UPRFaerinGodFallComponent::BroadcastEntryFinished(const bool bSucceeded)
{
	ClearRigTimers();
	bGodFallEntryRunning = false;
	if (!bSucceeded)
	{
		MulticastCleanupGodFallBodyNiagara();
		MulticastCancelSwordRiseCameraShake();
		EntryRuntimeState = EPRFaerinGodFallEntryRuntimeState::Idle;
		SetComponentTickEnabled(false);
		EndBossPresentationReplicationOverride();
	}
	OnGodFallEntryFinished.Broadcast(bSucceeded);
}

void UPRFaerinGodFallComponent::ClearRigTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RigChargeTimerHandle);
		World->GetTimerManager().ClearTimer(RigTiltPullTimerHandle);
		World->GetTimerManager().ClearTimer(BossDropTimerHandle);
	}
}

void UPRFaerinGodFallComponent::StartGodFallBodyNiagaraCuesLocal()
{
	CleanupGodFallBodyNiagaraLocal();

	if (!IsValid(GodFallData) || GodFallData->BodyNiagaraCues.IsEmpty())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	BodyNiagaraCueTimerHandles.SetNum(GodFallData->BodyNiagaraCues.Num());
	for (int32 CueIndex = 0; CueIndex < GodFallData->BodyNiagaraCues.Num(); ++CueIndex)
	{
		const FPRFaerinGodFallBodyNiagaraCue& Cue = GodFallData->BodyNiagaraCues[CueIndex];
		if (!IsValid(Cue.NiagaraSystem))
		{
			continue;
		}

		if (Cue.StartDelaySeconds <= UE_SMALL_NUMBER)
		{
			SpawnGodFallBodyNiagaraCueLocal(CueIndex);
			continue;
		}

		World->GetTimerManager().SetTimer(
			BodyNiagaraCueTimerHandles[CueIndex],
			FTimerDelegate::CreateWeakLambda(this, [this, CueIndex]()
			{
				SpawnGodFallBodyNiagaraCueLocal(CueIndex);
			}),
			Cue.StartDelaySeconds,
			false);
	}
}

void UPRFaerinGodFallComponent::SpawnGodFallBodyNiagaraCueLocal(const int32 CueIndex)
{
	if (!IsValid(GodFallData) || !GodFallData->BodyNiagaraCues.IsValidIndex(CueIndex))
	{
		return;
	}

	const FPRFaerinGodFallBodyNiagaraCue& Cue = GodFallData->BodyNiagaraCues[CueIndex];
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	USkeletalMeshComponent* MeshComponent = IsValid(OwnerBoss) ? OwnerBoss->GetMesh() : nullptr;
	if (!IsValid(Cue.NiagaraSystem) || !IsValid(MeshComponent))
	{
		return;
	}

	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		Cue.NiagaraSystem,
		MeshComponent,
		Cue.AttachSocketName,
		Cue.LocationOffset,
		Cue.RotationOffset,
		EAttachLocation::KeepRelativeOffset,
		true);
	if (!IsValid(NiagaraComponent))
	{
		return;
	}

	NiagaraComponent->SetRelativeScale3D(Cue.Scale);
	ActiveBodyNiagaraComponents.Add(NiagaraComponent);

	if (Cue.LifeSeconds <= UE_SMALL_NUMBER)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const int32 ComponentIndex = ActiveBodyNiagaraComponents.Num() - 1;
	FTimerHandle CleanupTimerHandle;
	World->GetTimerManager().SetTimer(
		CleanupTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, ComponentIndex]()
		{
			if (!ActiveBodyNiagaraComponents.IsValidIndex(ComponentIndex))
			{
				return;
			}

			if (UNiagaraComponent* ActiveNiagaraComponent = ActiveBodyNiagaraComponents[ComponentIndex])
			{
				ActiveNiagaraComponent->Deactivate();
				ActiveNiagaraComponent->DestroyComponent();
			}
			ActiveBodyNiagaraComponents[ComponentIndex] = nullptr;
		}),
		Cue.LifeSeconds,
		false);
	BodyNiagaraCleanupTimerHandles.Add(CleanupTimerHandle);
}

void UPRFaerinGodFallComponent::CleanupGodFallBodyNiagaraLocal()
{
	if (UWorld* World = GetWorld())
	{
		for (FTimerHandle& TimerHandle : BodyNiagaraCueTimerHandles)
		{
			World->GetTimerManager().ClearTimer(TimerHandle);
		}

		for (FTimerHandle& TimerHandle : BodyNiagaraCleanupTimerHandles)
		{
			World->GetTimerManager().ClearTimer(TimerHandle);
		}
	}

	BodyNiagaraCueTimerHandles.Reset();
	BodyNiagaraCleanupTimerHandles.Reset();

	for (UNiagaraComponent* NiagaraComponent : ActiveBodyNiagaraComponents)
	{
		if (IsValid(NiagaraComponent))
		{
			NiagaraComponent->Deactivate();
			NiagaraComponent->DestroyComponent();
		}
	}
	ActiveBodyNiagaraComponents.Reset();
}

void UPRFaerinGodFallComponent::StartSwordRiseCameraShakeLocal(
	TSubclassOf<UCameraShakeBase> CameraShakeClass,
	const float DelaySeconds,
	const float Scale,
	const float DurationOverride)
{
	CancelSwordRiseCameraShakeLocal();

	if (!IsValid(CameraShakeClass))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const float ResolvedDelaySeconds = FMath::Max(DelaySeconds, 0.0f);
	if (ResolvedDelaySeconds <= UE_SMALL_NUMBER)
	{
		PlaySwordRiseCameraShakeLocal(CameraShakeClass, Scale, DurationOverride);
		return;
	}

	World->GetTimerManager().SetTimer(
		SwordRiseCameraShakeTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, CameraShakeClass, Scale, DurationOverride]()
		{
			PlaySwordRiseCameraShakeLocal(CameraShakeClass, Scale, DurationOverride);
		}),
		ResolvedDelaySeconds,
		false);
}

void UPRFaerinGodFallComponent::PlaySwordRiseCameraShakeLocal(
	TSubclassOf<UCameraShakeBase> CameraShakeClass,
	const float Scale,
	const float DurationOverride) const
{
	if (!IsValid(CameraShakeClass))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!IsValid(PlayerController) || !PlayerController->IsLocalController() || !IsValid(PlayerController->PlayerCameraManager))
		{
			continue;
		}

		UCameraShakeBase* CameraShake = PlayerController->PlayerCameraManager->StartCameraShake(
			CameraShakeClass,
			FMath::Max(Scale, 0.0f));
		if (!IsValid(CameraShake) || DurationOverride <= UE_SMALL_NUMBER)
		{
			continue;
		}

		TWeakObjectPtr<APlayerCameraManager> WeakCameraManager = PlayerController->PlayerCameraManager;
		TWeakObjectPtr<UCameraShakeBase> WeakCameraShake = CameraShake;
		FTimerHandle StopTimerHandle;
		World->GetTimerManager().SetTimer(
			StopTimerHandle,
			FTimerDelegate::CreateLambda([WeakCameraManager, WeakCameraShake]()
			{
				APlayerCameraManager* CameraManager = WeakCameraManager.Get();
				UCameraShakeBase* ActiveCameraShake = WeakCameraShake.Get();
				if (IsValid(CameraManager) && IsValid(ActiveCameraShake))
				{
					CameraManager->StopCameraShake(ActiveCameraShake, true);
				}
			}),
			DurationOverride,
			false);
	}
}

void UPRFaerinGodFallComponent::CancelSwordRiseCameraShakeLocal()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SwordRiseCameraShakeTimerHandle);
	}
}

void UPRFaerinGodFallComponent::DestroyPlacedRigActor()
{
	if (!bDestroyRigActorAfterConversion || !IsValid(PlacedSwordRigActor))
	{
		return;
	}

	MulticastDestroyPlacedRigActor();
}

void UPRFaerinGodFallComponent::SetPlacedRigHidden(const bool bNewHidden)
{
	if (!IsValid(PlacedSwordRigActor))
	{
		return;
	}

	PlacedSwordRigActor->SetActorHiddenInGame(bNewHidden);
	PlacedSwordRigActor->SetActorEnableCollision(!bNewHidden);
}

void UPRFaerinGodFallComponent::ApplyBossPresentationTransform(const FVector& Location, const FRotator& Rotation)
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss))
	{
		return;
	}

	OwnerBoss->SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
}

void UPRFaerinGodFallComponent::BeginBossPresentationReplicationOverride()
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss) || !OwnerBoss->HasAuthority() || bHasSavedBossReplicateMovement)
	{
		return;
	}

	bSavedBossReplicateMovement = OwnerBoss->IsReplicatingMovement();
	bHasSavedBossReplicateMovement = true;
	OwnerBoss->SetReplicateMovement(false);
}

void UPRFaerinGodFallComponent::EndBossPresentationReplicationOverride()
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss) || !OwnerBoss->HasAuthority() || !bHasSavedBossReplicateMovement)
	{
		return;
	}

	MulticastStopBossPresentation(OwnerBoss->GetActorLocation(), OwnerBoss->GetActorRotation());
	OwnerBoss->SetReplicateMovement(bSavedBossReplicateMovement);
	bHasSavedBossReplicateMovement = false;
	OwnerBoss->ForceNetUpdate();
}

void UPRFaerinGodFallComponent::StartClientBossPresentationSegment(
	const EPRFaerinGodFallEntryRuntimeState NewState,
	const FVector& StartLocation,
	const FVector& TargetLocation,
	const FRotator& StartRotation,
	const FRotator& TargetRotation,
	const float DurationSeconds)
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss))
	{
		return;
	}

	ClientBossPresentationState = NewState;
	ClientBossSegmentStartLocation = StartLocation;
	ClientBossSegmentTargetLocation = TargetLocation;
	ClientBossSegmentStartRotation = StartRotation;
	ClientBossSegmentTargetRotation = TargetRotation;
	ClientBossSegmentElapsedSeconds = 0.0f;
	ClientBossSegmentDurationSeconds = FMath::Max(DurationSeconds, 0.0f);
	bClientBossPresentationActive = true;
	bClientBossPresentationHolding = false;

	OwnerBoss->SetActorLocationAndRotation(StartLocation, StartRotation, false, nullptr, ETeleportType::TeleportPhysics);

	if (ClientBossSegmentDurationSeconds <= UE_SMALL_NUMBER)
	{
		StopClientBossPresentation(TargetLocation, TargetRotation);
		return;
	}

	SetComponentTickEnabled(true);
}

void UPRFaerinGodFallComponent::SetClientBossPresentationTransform(
	const FVector& Location,
	const FRotator& Rotation,
	const bool bKeepHolding)
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss))
	{
		return;
	}

	ClientBossSegmentTargetLocation = Location;
	ClientBossSegmentTargetRotation = Rotation;
	bClientBossPresentationActive = bKeepHolding;
	bClientBossPresentationHolding = bKeepHolding;
	ClientBossPresentationState = bKeepHolding
		? EPRFaerinGodFallEntryRuntimeState::TiltHolding
		: EPRFaerinGodFallEntryRuntimeState::Idle;

	OwnerBoss->SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	SetComponentTickEnabled(bKeepHolding);
}

void UPRFaerinGodFallComponent::StopClientBossPresentation(const FVector& Location, const FRotator& Rotation)
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (IsValid(OwnerBoss))
	{
		OwnerBoss->SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	bClientBossPresentationActive = false;
	bClientBossPresentationHolding = false;
	ClientBossPresentationState = EPRFaerinGodFallEntryRuntimeState::Idle;
	SetComponentTickEnabled(false);
}

void UPRFaerinGodFallComponent::MulticastStartBossPresentationSegment_Implementation(
	EPRFaerinGodFallEntryRuntimeState NewState,
	FVector StartLocation,
	FVector TargetLocation,
	FRotator StartRotation,
	FRotator TargetRotation,
	float DurationSeconds)
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss) || OwnerBoss->HasAuthority())
	{
		return;
	}

	StartClientBossPresentationSegment(NewState, StartLocation, TargetLocation, StartRotation, TargetRotation, DurationSeconds);
}

void UPRFaerinGodFallComponent::MulticastSetBossPresentationTransform_Implementation(
	FVector Location,
	FRotator Rotation,
	const bool bKeepHolding)
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss) || OwnerBoss->HasAuthority())
	{
		return;
	}

	SetClientBossPresentationTransform(Location, Rotation, bKeepHolding);
}

void UPRFaerinGodFallComponent::MulticastStopBossPresentation_Implementation(FVector Location, FRotator Rotation)
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (!IsValid(OwnerBoss) || OwnerBoss->HasAuthority())
	{
		return;
	}

	StopClientBossPresentation(Location, Rotation);
}

void UPRFaerinGodFallComponent::MulticastSetPlacedRigHidden_Implementation(const bool bNewHidden)
{
	SetPlacedRigHidden(bNewHidden);
}

void UPRFaerinGodFallComponent::MulticastPlayRigAnimation_Implementation(UAnimSequenceBase* Animation, const float PlayRate)
{
	APRBossBaseCharacter* OwnerBoss = GetOwnerBoss();
	if (IsValid(OwnerBoss) && OwnerBoss->HasAuthority())
	{
		return;
	}

	PlayRigAnimationLocal(Animation, PlayRate);
}

void UPRFaerinGodFallComponent::MulticastDestroyPlacedRigActor_Implementation()
{
	if (IsValid(PlacedSwordRigActor))
	{
		PlacedSwordRigActor->SetActorHiddenInGame(true);
		PlacedSwordRigActor->SetActorEnableCollision(false);
		PlacedSwordRigActor->Destroy();
	}

	PlacedSwordRigActor = nullptr;
	PlacedSwordRigMeshComponent = nullptr;
}

void UPRFaerinGodFallComponent::MulticastStartGodFallBodyNiagaraCues_Implementation()
{
	StartGodFallBodyNiagaraCuesLocal();
}

void UPRFaerinGodFallComponent::MulticastCleanupGodFallBodyNiagara_Implementation()
{
	CleanupGodFallBodyNiagaraLocal();
}

void UPRFaerinGodFallComponent::MulticastStartSwordRiseCameraShake_Implementation(
	TSubclassOf<UCameraShakeBase> CameraShakeClass,
	const float DelaySeconds,
	const float Scale,
	const float DurationOverride)
{
	StartSwordRiseCameraShakeLocal(CameraShakeClass, DelaySeconds, Scale, DurationOverride);
}

void UPRFaerinGodFallComponent::MulticastCancelSwordRiseCameraShake_Implementation()
{
	CancelSwordRiseCameraShakeLocal();
}

void UPRFaerinGodFallComponent::ClearInvalidStaticSwordRefs()
{
	ActiveStaticSwords.RemoveAll([](const APRFaerinGodFallStaticSwordActor* StaticSword)
	{
		return !IsValid(StaticSword);
	});
}
