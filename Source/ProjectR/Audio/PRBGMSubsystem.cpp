// Copyright ProjectR. All Rights Reserved.

#include "PRBGMSubsystem.h"

#include "AbilitySystemComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Audio/PRBGMRegistryDataAsset.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PRDeveloperSettings.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace
{
	EPRBGMState MapBossPhaseToBGMState(EPRBossPhase BossPhase)
	{
		switch (BossPhase)
		{
		case EPRBossPhase::Phase1:
			return EPRBGMState::BossPhaseA;
		case EPRBossPhase::Phase2:
			return EPRBGMState::BossPhaseB;
		case EPRBossPhase::Phase3:
			return EPRBGMState::BossPhaseC;
		case EPRBossPhase::Phase4:
			return EPRBGMState::BossPhaseD;
		default:
			return EPRBGMState::BossPhaseA;
		}
	}
}

/*~ UWorldSubsystem Interface ~*/

void UPRBGMSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	BindBossBGMEvents();
}

void UPRBGMSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	BindBossBGMEvents();
	StartLevelBGM();
	BindLocalPlayerCombatTag();
}

void UPRBGMSubsystem::Deinitialize()
{
	UnbindLocalPlayerCombatTag();
	UnbindBossBGMEvents();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CombatBGMReleaseTimerHandle);
	}
	StopLocalPlayerCombatTagBindRetry();

	StopBGM(0.0f);
	for (TWeakObjectPtr<UAudioComponent>& WeakAudioComponent : FadingOutAudioComponents)
	{
		if (UAudioComponent* AudioComponent = WeakAudioComponent.Get())
		{
			AudioComponent->Stop();
			AudioComponent->DestroyComponent();
		}
	}
	FadingOutAudioComponents.Empty();

	Super::Deinitialize();
}

/*~ BGM 제어 ~*/

void UPRBGMSubsystem::StartLevelBGM()
{
	if (!TryCacheCurrentLevelEntry())
	{
		return;
	}

	SetBGMState(CurrentLevelEntry.DefaultState);
}

void UPRBGMSubsystem::StopBGM(float FadeOutDuration)
{
	if (IsValid(CurrentAudioComponent))
	{
		FadeOutAndDestroyAudioComponent(CurrentAudioComponent, FadeOutDuration);
	}

	CurrentAudioComponent = nullptr;
	CurrentSound = nullptr;
	CurrentState = EPRBGMState::None;
}

void UPRBGMSubsystem::SetBGMState(EPRBGMState NewState)
{
	if (NewState == EPRBGMState::None)
	{
		StopBGM(0.0f);
		return;
	}

	if (CurrentState == EPRBGMState::Victory && NewState != EPRBGMState::Victory)
	{
		return;
	}

	if (IsBossPhaseState(CurrentState) && !IsBossPhaseState(NewState) && NewState != EPRBGMState::Victory)
	{
		return;
	}

	if (!bHasCurrentLevelEntry && !TryCacheCurrentLevelEntry())
	{
		return;
	}

	FPRBGMTrack Track;
	if (!UPRBGMRegistryDataAsset::TryGetTrackForState(CurrentLevelEntry, NewState, Track))
	{
		return;
	}

	USoundBase* Sound = LoadTrackSound(Track);
	if (!IsValid(Sound))
	{
		return;
	}

	if (CurrentState == NewState && IsValid(CurrentAudioComponent))
	{
		return;
	}

	if (CurrentSound.Get() == Sound && IsValid(CurrentAudioComponent))
	{
		CurrentState = NewState;
		return;
	}

	PlayTrack(NewState, Track);
}

void UPRBGMSubsystem::RestoreDefaultLevelBGM()
{
	if (IsBossPhaseState(CurrentState) || CurrentState == EPRBGMState::Victory)
	{
		return;
	}

	if (!bHasCurrentLevelEntry && !TryCacheCurrentLevelEntry())
	{
		return;
	}

	SetBGMState(CurrentLevelEntry.DefaultState);
}

/*~ 레벨 데이터 ~*/

FName UPRBGMSubsystem::ResolveCurrentLevelId() const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return NAME_None;
	}

	const FString CleanMapName = UWorld::RemovePIEPrefix(World->GetMapName());
	return FName(*CleanMapName);
}

bool UPRBGMSubsystem::TryCacheCurrentLevelEntry()
{
	CurrentLevelId = ResolveCurrentLevelId();
	bHasCurrentLevelEntry = false;

	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (!IsValid(Settings))
	{
		return false;
	}

	const UPRBGMRegistryDataAsset* Registry = Settings->GetDefaultBGMRegistrySync();
	if (!IsValid(Registry))
	{
		return false;
	}

	if (!Registry->FindLevelEntry(CurrentLevelId, CurrentLevelEntry))
	{
		return false;
	}

	bHasCurrentLevelEntry = true;
	return true;
}

USoundBase* UPRBGMSubsystem::LoadTrackSound(const FPRBGMTrack& Track) const
{
	return Track.Sound.LoadSynchronous();
}

/*~ 오디오 재생 ~*/

void UPRBGMSubsystem::PlayTrack(EPRBGMState NewState, const FPRBGMTrack& Track)
{
	USoundBase* Sound = LoadTrackSound(Track);
	if (!IsValid(Sound))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UAudioComponent* PreviousAudioComponent = CurrentAudioComponent;
	CurrentAudioComponent = UGameplayStatics::SpawnSound2D(
		World,
		Sound,
		FMath::Max(Track.VolumeMultiplier, 0.0f),
		1.0f,
		FMath::Max(Track.StartTime, 0.0f),
		nullptr,
		false,
		false);

	if (!IsValid(CurrentAudioComponent))
	{
		CurrentAudioComponent = PreviousAudioComponent;
		return;
	}

	CurrentAudioComponent->FadeIn(
		FMath::Max(Track.FadeInDuration, 0.0f),
		FMath::Max(Track.VolumeMultiplier, 0.0f),
		FMath::Max(Track.StartTime, 0.0f));

	if (IsValid(PreviousAudioComponent))
	{
		FadeOutAndDestroyAudioComponent(PreviousAudioComponent, Track.FadeOutDuration);
	}

	CurrentSound = Sound;
	CurrentState = NewState;
	CleanupFadingAudioComponents();
}

void UPRBGMSubsystem::FadeOutAndDestroyAudioComponent(UAudioComponent* AudioComponent, float FadeOutDuration)
{
	if (!IsValid(AudioComponent))
	{
		return;
	}

	UWorld* World = GetWorld();
	const float ClampedFadeOutDuration = FMath::Max(FadeOutDuration, 0.0f);
	if (!IsValid(World) || ClampedFadeOutDuration <= UE_SMALL_NUMBER)
	{
		AudioComponent->Stop();
		AudioComponent->DestroyComponent();
		return;
	}

	FadingOutAudioComponents.Add(AudioComponent);
	AudioComponent->FadeOut(ClampedFadeOutDuration, 0.0f);

	FTimerHandle DestroyTimerHandle;
	TWeakObjectPtr<UAudioComponent> WeakAudioComponent(AudioComponent);
	World->GetTimerManager().SetTimer(
		DestroyTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, WeakAudioComponent]()
		{
			if (UAudioComponent* FadingAudioComponent = WeakAudioComponent.Get())
			{
				FadingAudioComponent->Stop();
				FadingAudioComponent->DestroyComponent();
			}

			CleanupFadingAudioComponents();
		}),
		ClampedFadeOutDuration,
		false);
}

void UPRBGMSubsystem::CleanupFadingAudioComponents()
{
	FadingOutAudioComponents.RemoveAll([](const TWeakObjectPtr<UAudioComponent>& WeakAudioComponent)
	{
		return !WeakAudioComponent.IsValid();
	});
}

/*~ 전투 상태 연동 ~*/

void UPRBGMSubsystem::BindLocalPlayerCombatTag()
{
	UPRAbilitySystemComponent* AbilitySystemComponent = ResolveLocalPlayerAbilitySystem();
	if (!IsValid(AbilitySystemComponent))
	{
		StartLocalPlayerCombatTagBindRetry();
		return;
	}

	if (BoundAbilitySystemComponent.Get() == AbilitySystemComponent && CombatTagDelegateHandle.IsValid())
	{
		return;
	}

	UnbindLocalPlayerCombatTag();
	BoundAbilitySystemComponent = AbilitySystemComponent;
	CombatTagDelegateHandle = AbilitySystemComponent->OnGameplayTagUpdated.AddUObject(
		this,
		&ThisClass::HandleLocalPlayerGameplayTagUpdated);
	StopLocalPlayerCombatTagBindRetry();

	if (AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Combating))
	{
		SetBGMState(EPRBGMState::Combat);
	}
}

void UPRBGMSubsystem::StartLocalPlayerCombatTagBindRetry()
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetTimerManager().IsTimerActive(LocalPlayerBindRetryTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		LocalPlayerBindRetryTimerHandle,
		this,
		&UPRBGMSubsystem::BindLocalPlayerCombatTag,
		FMath::Max(LocalPlayerBindRetryInterval, 0.1f),
		true);
}

void UPRBGMSubsystem::StopLocalPlayerCombatTagBindRetry()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	World->GetTimerManager().ClearTimer(LocalPlayerBindRetryTimerHandle);
}

void UPRBGMSubsystem::UnbindLocalPlayerCombatTag()
{
	UPRAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
	if (IsValid(AbilitySystemComponent) && CombatTagDelegateHandle.IsValid())
	{
		AbilitySystemComponent->OnGameplayTagUpdated.Remove(CombatTagDelegateHandle);
	}

	CombatTagDelegateHandle.Reset();
	BoundAbilitySystemComponent = nullptr;
}

UPRAbilitySystemComponent* UPRBGMSubsystem::ResolveLocalPlayerAbilitySystem() const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PlayerController = It->Get();
		if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
		{
			continue;
		}

		const APRPlayerState* PRPlayerState = PlayerController->GetPlayerState<APRPlayerState>();
		if (!IsValid(PRPlayerState))
		{
			continue;
		}

		return Cast<UPRAbilitySystemComponent>(PRPlayerState->GetAbilitySystemComponent());
	}

	return nullptr;
}

void UPRBGMSubsystem::HandleLocalPlayerGameplayTagUpdated(const FGameplayTag& Tag, bool bTagExists)
{
	if (!Tag.MatchesTagExact(PRGameplayTags::State_Combating))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	World->GetTimerManager().ClearTimer(CombatBGMReleaseTimerHandle);

	if (bTagExists)
	{
		SetBGMState(EPRBGMState::Combat);
		return;
	}

	World->GetTimerManager().SetTimer(
		CombatBGMReleaseTimerHandle,
		this,
		&UPRBGMSubsystem::HandleCombatBGMReleaseDelayElapsed,
		FMath::Max(CombatBGMReleaseDelay, 0.0f),
		false);
}

void UPRBGMSubsystem::HandleCombatBGMReleaseDelayElapsed()
{
	UPRAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
	if (IsValid(AbilitySystemComponent) && AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Combating))
	{
		return;
	}

	RestoreDefaultLevelBGM();
}

void UPRBGMSubsystem::BindBossBGMEvents()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>();
	if (!IsValid(EventMgr))
	{
		return;
	}

	if (!BossEncounterBeginDelegateHandle.IsValid())
	{
		BossEncounterBeginDelegateHandle = EventMgr->Listen(
			PRGameplayTags::Event_Boss_Encounter_Begin,
			FPREventMulticast::FDelegate::CreateUObject(this, &ThisClass::HandleBossEncounterBeginEvent));
	}

	if (!BossEncounterEndDelegateHandle.IsValid())
	{
		BossEncounterEndDelegateHandle = EventMgr->Listen(
			PRGameplayTags::Event_Boss_Encounter_End,
			FPREventMulticast::FDelegate::CreateUObject(this, &ThisClass::HandleBossEncounterEndEvent));
	}

	if (!BossPhaseChangedDelegateHandle.IsValid())
	{
		BossPhaseChangedDelegateHandle = EventMgr->Listen(
			PRGameplayTags::Event_Boss_PhaseChanged,
			FPREventMulticast::FDelegate::CreateUObject(this, &ThisClass::HandleBossPhaseChangedEvent));
	}
}

void UPRBGMSubsystem::UnbindBossBGMEvents()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		BossEncounterBeginDelegateHandle.Reset();
		BossEncounterEndDelegateHandle.Reset();
		BossPhaseChangedDelegateHandle.Reset();
		return;
	}

	UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>();
	if (!IsValid(EventMgr))
	{
		BossEncounterBeginDelegateHandle.Reset();
		BossEncounterEndDelegateHandle.Reset();
		BossPhaseChangedDelegateHandle.Reset();
		return;
	}

	EventMgr->Unlisten(PRGameplayTags::Event_Boss_Encounter_Begin, BossEncounterBeginDelegateHandle);
	EventMgr->Unlisten(PRGameplayTags::Event_Boss_Encounter_End, BossEncounterEndDelegateHandle);
	EventMgr->Unlisten(PRGameplayTags::Event_Boss_PhaseChanged, BossPhaseChangedDelegateHandle);
}

void UPRBGMSubsystem::HandleBossEncounterBeginEvent(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FPRBossEncounterEventPayload* EncounterPayload = Payload.GetPtr<FPRBossEncounterEventPayload>();
	if (!EncounterPayload || !IsValid(EncounterPayload->Boss))
	{
		return;
	}

	SetBGMState(MapBossPhaseToBGMState(EncounterPayload->Boss->GetCurrentPhase()));
}

void UPRBGMSubsystem::HandleBossEncounterEndEvent(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const EPRBGMState PreviousState = CurrentState;
	SetBGMState(EPRBGMState::Victory);

	if (IsBossPhaseState(PreviousState) && IsBossPhaseState(CurrentState))
	{
		StopBGM(1.0f);
	}
}

void UPRBGMSubsystem::HandleBossPhaseChangedEvent(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FPRBossPhaseChangedEventPayload* PhasePayload = Payload.GetPtr<FPRBossPhaseChangedEventPayload>();
	if (!PhasePayload || !IsValid(PhasePayload->Boss))
	{
		return;
	}

	SetBGMState(MapBossPhaseToBGMState(PhasePayload->NewPhase));
}

bool UPRBGMSubsystem::IsBossPhaseState(EPRBGMState State) const
{
	return State == EPRBGMState::BossPhaseA
		|| State == EPRBGMState::BossPhaseB
		|| State == EPRBGMState::BossPhaseC
		|| State == EPRBGMState::BossPhaseD;
}
