// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Loading Screen 서브시스템 구현)
#include "PRLoadingScreenSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/UserInterfaceSettings.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/PackageName.h"
#include "MoviePlayer.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ProjectR/FX/PRFXSubsystem.h"
#include "ProjectR/Game/PRGameInstance.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Shop/Components/PRShopComponent.h"
#include "ProjectR/Shop/Data/PRShopDataAsset.h"
#include "ProjectR/Shop/Types/PRShopTypes.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/System/PRDeveloperSettings.h"
#include "ProjectR/System/PRMapPreloadDataAsset.h"
#include "ProjectR/System/PRRuntimePreloadDataAsset.h"
#include "ProjectR/UI/Components/PRUIControllerComponent.h"
#include "ProjectR/UI/Loading/PRLoadingScreenWidgetBase.h"
#include "ProjectR/Utils/PRAssetUtils.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	constexpr int32 NiagaraRenderPrewarmFrameCount = 8;
	constexpr int32 NiagaraRenderPrewarmAdvanceTickCount = 4;
	constexpr float NiagaraRenderPrewarmAdvanceTickDelta = 0.016667f;
	constexpr float NiagaraRenderPrewarmDistance = 250.0f;
}

void UPRLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UPRLoadingScreenSubsystem::HandlePreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UPRLoadingScreenSubsystem::HandlePostLoadMapWithWorld);
}

void UPRLoadingScreenSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	HideViewportLoadingWidget();
	FinishNiagaraRenderPrewarm();
	StopMoviePlayerScreen();

	Super::Deinitialize();
}

void UPRLoadingScreenSubsystem::BeginTravel(FName TravelReason)
{
	BeginTravelToMap(TravelReason, FString());
}

void UPRLoadingScreenSubsystem::BeginTravelToMap(FName TravelReason, const FString& MapName)
{
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (!Settings || !Settings->bEnableLoadingScreenSystem)
	{
		return;
	}

	CurrentTravelReason = TravelReason;
	CurrentTravelMapName = MapName;
	TravelStartSeconds = FPlatformTime::Seconds();
	bRequiredPreloadComplete = false;
	bCharacterRuntimeBasePreloadComplete = true;
	bCharacterRuntimePreloadComplete = true;
	bCharacterRuntimeCuePreloadStarted = false;
	bNiagaraRenderPrewarmComplete = true;
	bNiagaraRenderPrewarmInProgress = false;
	bShopUIPrewarmComplete = true;
	bMinimumDisplayTimeComplete = Settings->MinimumLoadingScreenSeconds <= 0.0f;
	RemainingNiagaraRenderPrewarmFrames = 0;
	PrewarmNiagaraComponents.Reset();
	RequiredPreloadRequestId = 0;
	CharacterRuntimeBasePreloadRequestId = 0;
	CharacterRuntimeCuePreloadRequestId = 0;
	SetLoadingState(EPRLoadingState::PreTravel);
	if (!CurrentTravelMapName.IsEmpty())
	{
		StartMoviePlayerScreen(CurrentTravelMapName);
	}
	StartCharacterRuntimePreload();
}

void UPRLoadingScreenSubsystem::HandlePreLoadMap(const FString& MapName)
{
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (!Settings || !Settings->bEnableLoadingScreenSystem)
	{
		return;
	}

	if (LoadingState != EPRLoadingState::PreTravel)
	{
		return;
	}

	if (CurrentTravelMapName.IsEmpty())
	{
		CurrentTravelMapName = MapName;
	}
	SetLoadingState(EPRLoadingState::MapLoading);
	StartMoviePlayerScreen(CurrentTravelMapName);
}

void UPRLoadingScreenSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (!Settings || !Settings->bEnableLoadingScreenSystem || !IsValid(LoadedWorld))
	{
		return;
	}

	if (LoadingState != EPRLoadingState::PreTravel && LoadingState != EPRLoadingState::MapLoading)
	{
		return;
	}

	if (!IsLoadedWorldTravelDestination(LoadedWorld))
	{
		return;
	}

	CurrentLoadedWorld = LoadedWorld;
	ShowViewportLoadingWidget();
	TryStartCharacterRuntimeCuePreload();
	StartPostMapPreload(LoadedWorld);
}

void UPRLoadingScreenSubsystem::StartMoviePlayerScreen(const FString& MapName)
{
	if (!IsMoviePlayerEnabled())
	{
		return;
	}

	if (GetMoviePlayer()->IsMovieCurrentlyPlaying())
	{
		return;
	}

	MoviePlayerWidget = nullptr;
	TSharedPtr<SWidget> LoadingWidget;
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (Settings && !Settings->MoviePlayerWidgetClass.IsNull())
	{
		UClass* WidgetClass = Settings->MoviePlayerWidgetClass.LoadSynchronous();
		if (IsValid(WidgetClass))
		{
			UGameInstance* GameInstance = GetGameInstance();
			APlayerController* PlayerController = IsValid(GameInstance) ? GameInstance->GetFirstLocalPlayerController() : nullptr;
			MoviePlayerWidget = IsValid(PlayerController)
				? CreateWidget<UUserWidget>(PlayerController, WidgetClass)
				: CreateWidget<UUserWidget>(GameInstance, WidgetClass);

			if (IsValid(MoviePlayerWidget))
			{
				if (UPRLoadingScreenWidgetBase* LoadingScreenWidget = Cast<UPRLoadingScreenWidgetBase>(MoviePlayerWidget))
				{
					LoadingScreenWidget->SetLoadingScreenWidgetPhase(EPRLoadingScreenWidgetPhase::MoviePlayer);
					LoadingScreenWidget->SetLoadingDestination(MapName);
				}

				FVector2D ViewportSize = FVector2D::ZeroVector;
				if (UGameViewportClient* ViewportClient = IsValid(GameInstance) ? GameInstance->GetGameViewportClient() : nullptr)
				{
					ViewportClient->GetViewportSize(ViewportSize);
				}

				if (ViewportSize.IsNearlyZero() && IsValid(PlayerController))
				{
					int32 ViewportSizeX = 0;
					int32 ViewportSizeY = 0;
					PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
					ViewportSize = FVector2D(ViewportSizeX, ViewportSizeY);
				}

				const FIntPoint ViewportIntSize(
					FMath::Max(1, FMath::TruncToInt32(ViewportSize.X)),
					FMath::Max(1, FMath::TruncToInt32(ViewportSize.Y)));
				const float DPIScale = GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(ViewportIntSize);

				TSharedRef<SWidget> MoviePlayerRootWidget = SNew(SBox)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Fill)
						[
							SNew(SDPIScaler)
							.DPIScale(DPIScale)
							[
								MoviePlayerWidget->TakeWidget()
							]
						]
					];

				LoadingWidget = MoviePlayerRootWidget;
			}
		}
	}

	if (!LoadingWidget.IsValid())
	{
		LoadingWidget = SNew(STextBlock)
			.Text(FText::FromString(TEXT("Loading...")));
	}

	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
	LoadingScreen.bMoviesAreSkippable = false;
	LoadingScreen.bWaitForManualStop = false;
	LoadingScreen.bAllowEngineTick = false;
	LoadingScreen.MinimumLoadingScreenDisplayTime = -1.0f;
	LoadingScreen.WidgetLoadingScreen = LoadingWidget;

	GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
	GetMoviePlayer()->PlayMovie();
}

void UPRLoadingScreenSubsystem::StopMoviePlayerScreen()
{
	if (IsMoviePlayerEnabled() && GetMoviePlayer()->IsMovieCurrentlyPlaying())
	{
		GetMoviePlayer()->StopMovie();
	}

	MoviePlayerWidget = nullptr;
}

void UPRLoadingScreenSubsystem::ShowViewportLoadingWidget()
{
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (!Settings || Settings->LoadingScreenWidgetClass.IsNull())
	{
		return;
	}

	if (IsValid(ViewportLoadingWidget) && ViewportLoadingWidget->IsInViewport())
	{
		return;
	}

	UClass* WidgetClass = Settings->LoadingScreenWidgetClass.LoadSynchronous();
	if (!IsValid(WidgetClass))
	{
		return;
	}

	ViewportLoadingWidget = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
	if (IsValid(ViewportLoadingWidget))
	{
		if (UPRLoadingScreenWidgetBase* LoadingScreenWidget = Cast<UPRLoadingScreenWidgetBase>(ViewportLoadingWidget))
		{
			const FString MapName = !CurrentTravelMapName.IsEmpty()
				? CurrentTravelMapName
				: (IsValid(CurrentLoadedWorld.Get()) ? CurrentLoadedWorld->GetOutermost()->GetName() : FString());
			LoadingScreenWidget->SetLoadingScreenWidgetPhase(EPRLoadingScreenWidgetPhase::Viewport);
			LoadingScreenWidget->SetLoadingDestination(MapName);
		}

		ViewportLoadingWidget->AddToViewport(100000);
		ViewportLoadingWidget->ForceLayoutPrepass();
	}
}

void UPRLoadingScreenSubsystem::HideViewportLoadingWidget()
{
	if (IsValid(ViewportLoadingWidget))
	{
		ViewportLoadingWidget->RemoveFromParent();
		ViewportLoadingWidget = nullptr;
	}
}

void UPRLoadingScreenSubsystem::StartCharacterRuntimePreload()
{
	TArray<FSoftObjectPath> AssetPaths;
	if (const UPRGameInstance* PRGameInstance = Cast<UPRGameInstance>(GetGameInstance()))
	{
		UPRAssetUtils::CollectPlayerRuntimePreloadPaths(PRGameInstance->GetLocalCharacter(), AssetPaths);
	}

	const UPRRuntimePreloadDataAsset* RuntimePreloadData = ResolveRuntimePreloadData();
	const bool bHasRuntimePreloadFXTags = IsValid(RuntimePreloadData) && !RuntimePreloadData->PreloadFXTags.IsEmpty();
	if (IsValid(RuntimePreloadData))
	{
		for (const TSoftObjectPtr<UObject>& RequiredAsset : RuntimePreloadData->RequiredAssets)
		{
			if (!RequiredAsset.IsNull())
			{
				AssetPaths.AddUnique(RequiredAsset.ToSoftObjectPath());
			}
		}

		for (const TSoftObjectPtr<UNiagaraSystem>& NiagaraSystem : RuntimePreloadData->RenderPrewarmNiagaraSystems)
		{
			if (!NiagaraSystem.IsNull())
			{
				AssetPaths.AddUnique(NiagaraSystem.ToSoftObjectPath());
			}
		}

		for (const TSoftObjectPtr<USoundBase>& SFX : RuntimePreloadData->SFX)
		{
			if (!SFX.IsNull())
			{
				AssetPaths.AddUnique(SFX.ToSoftObjectPath());
			}
		}
	}

	if (AssetPaths.IsEmpty())
	{
		if (bHasRuntimePreloadFXTags)
		{
			bCharacterRuntimePreloadComplete = false;
		}
		return;
	}

	bCharacterRuntimeBasePreloadComplete = false;
	bCharacterRuntimePreloadComplete = false;

	FPRPreloadRequest Request;
	Request.Group = EPRPreloadGroup::CharacterRuntime;
	Request.QueueType = EPRPreloadQueueType::Required;
	Request.AssetPaths = AssetPaths;

	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	Request.TimeoutSeconds = Settings ? Settings->SoftPreloadTimeoutSeconds : 0.0f;

	CharacterRuntimeBasePreloadRequestId = UPRAssetManager::Get().StartPreloadRequest(
		Request,
		FSimpleDelegate::CreateUObject(this, &UPRLoadingScreenSubsystem::HandleCharacterRuntimeBasePreloadComplete),
		FSimpleDelegate::CreateUObject(this, &UPRLoadingScreenSubsystem::HandleCharacterRuntimeBasePreloadTimeout));
}

void UPRLoadingScreenSubsystem::HandleCharacterRuntimeBasePreloadComplete()
{
	bCharacterRuntimeBasePreloadComplete = true;
	TryStartCharacterRuntimeCuePreload();
}

void UPRLoadingScreenSubsystem::HandleCharacterRuntimeBasePreloadTimeout()
{
	UE_LOG(LogTemp, Warning, TEXT("PRLoadingScreenSubsystem: Character runtime base preload timeout TravelReason=%s"), *CurrentTravelReason.ToString());
	bCharacterRuntimeBasePreloadComplete = true;
	TryStartCharacterRuntimeCuePreload();
}

void UPRLoadingScreenSubsystem::TryStartCharacterRuntimeCuePreload()
{
	if (bCharacterRuntimePreloadComplete || bCharacterRuntimeCuePreloadStarted || !bCharacterRuntimeBasePreloadComplete)
	{
		return;
	}

	UWorld* LoadedWorld = CurrentLoadedWorld.Get();
	if (!IsValid(LoadedWorld))
	{
		return;
	}

	FGameplayTagContainer FXTags;
	UPRAssetUtils::CollectDefaultPlayerCombatFXTags(FXTags);
	if (UPRRuntimePreloadDataAsset* RuntimePreloadData = ResolveRuntimePreloadData())
	{
		FXTags.AppendTags(RuntimePreloadData->PreloadFXTags);
	}

	TArray<FSoftObjectPath> AssetPaths;
	if (UPRFXSubsystem* FXSubsystem = LoadedWorld->GetSubsystem<UPRFXSubsystem>())
	{
		FXSubsystem->CollectPreloadAssetPathsForTags(FXTags, AssetPaths);
	}

	if (AssetPaths.IsEmpty())
	{
		bCharacterRuntimePreloadComplete = true;
		TryReveal();
		return;
	}

	bCharacterRuntimeCuePreloadStarted = true;

	FPRPreloadRequest Request;
	Request.Group = EPRPreloadGroup::CharacterRuntime;
	Request.QueueType = EPRPreloadQueueType::Required;
	Request.AssetPaths = AssetPaths;

	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	Request.TimeoutSeconds = Settings ? Settings->SoftPreloadTimeoutSeconds : 0.0f;

	CharacterRuntimeCuePreloadRequestId = UPRAssetManager::Get().StartPreloadRequest(
		Request,
		FSimpleDelegate::CreateUObject(this, &UPRLoadingScreenSubsystem::HandleCharacterRuntimeCuePreloadComplete),
		FSimpleDelegate::CreateUObject(this, &UPRLoadingScreenSubsystem::HandleCharacterRuntimeCuePreloadTimeout));
}

void UPRLoadingScreenSubsystem::HandleCharacterRuntimeCuePreloadComplete()
{
	bCharacterRuntimePreloadComplete = true;
	TryReveal();
}

void UPRLoadingScreenSubsystem::HandleCharacterRuntimeCuePreloadTimeout()
{
	UE_LOG(LogTemp, Warning, TEXT("PRLoadingScreenSubsystem: Character runtime cue preload timeout TravelReason=%s"), *CurrentTravelReason.ToString());
	bCharacterRuntimePreloadComplete = true;
	TryReveal();
}

void UPRLoadingScreenSubsystem::StartPostMapPreload(UWorld* LoadedWorld)
{
	SetLoadingState(EPRLoadingState::PostMapPreload);

	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	const float ElapsedSeconds = static_cast<float>(FPlatformTime::Seconds() - TravelStartSeconds);
	const float RemainingMinimumSeconds = Settings ? FMath::Max(0.0f, Settings->MinimumLoadingScreenSeconds - ElapsedSeconds) : 0.0f;
	if (RemainingMinimumSeconds > 0.0f)
	{
		LoadedWorld->GetTimerManager().SetTimer(
			MinimumDisplayTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				bMinimumDisplayTimeComplete = true;
				TryReveal();
			}),
			RemainingMinimumSeconds,
			false);
	}
	else
	{
		bMinimumDisplayTimeComplete = true;
	}

	UPRMapPreloadDataAsset* MapPreloadData = ResolveMapPreloadData(LoadedWorld);
	StartRequiredPreload(MapPreloadData);
}

void UPRLoadingScreenSubsystem::StartRequiredPreload(UPRMapPreloadDataAsset* MapPreloadData)
{
	TArray<FSoftObjectPath> AssetPaths;
	CollectRequiredAssetPaths(MapPreloadData, AssetPaths);

	FPRPreloadRequest Request;
	Request.Group = EPRPreloadGroup::MapTransition;
	Request.QueueType = EPRPreloadQueueType::Required;
	Request.AssetPaths = AssetPaths;
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	Request.TimeoutSeconds = Settings ? Settings->RequiredPreloadTimeoutSeconds : 0.0f;

	RequiredPreloadRequestId = UPRAssetManager::Get().StartPreloadRequest(
		Request,
		FSimpleDelegate::CreateUObject(this, &UPRLoadingScreenSubsystem::HandleRequiredPreloadComplete),
		FSimpleDelegate::CreateUObject(this, &UPRLoadingScreenSubsystem::HandleRequiredPreloadTimeout));
}

void UPRLoadingScreenSubsystem::HandleRequiredPreloadComplete()
{
	bRequiredPreloadComplete = true;
	UPRMapPreloadDataAsset* MapPreloadData = ResolveMapPreloadData(CurrentLoadedWorld.Get());
	StartNiagaraRenderPrewarm(CurrentLoadedWorld.Get(), MapPreloadData);
	StartShopUIPrewarm(CurrentLoadedWorld.Get(), MapPreloadData);
	TryReveal();
}

void UPRLoadingScreenSubsystem::HandleRequiredPreloadTimeout()
{
	UE_LOG(LogTemp, Error, TEXT("PRLoadingScreenSubsystem: Required preload timeout TravelReason=%s"), *CurrentTravelReason.ToString());
}

void UPRLoadingScreenSubsystem::StartNiagaraRenderPrewarm(UWorld* LoadedWorld, UPRMapPreloadDataAsset* MapPreloadData)
{
	bNiagaraRenderPrewarmComplete = true;
	if (!IsValid(LoadedWorld))
	{
		return;
	}

	if (!IsValid(ViewportLoadingWidget) || !ViewportLoadingWidget->IsInViewport())
	{
		return;
	}

	TArray<UNiagaraSystem*> NiagaraSystems;
	CollectNiagaraRenderPrewarmSystems(MapPreloadData, ResolveRuntimePreloadData(), NiagaraSystems);
	if (NiagaraSystems.IsEmpty())
	{
		return;
	}

	APlayerController* PlayerController = LoadedWorld->GetFirstPlayerController();
	if (!IsValid(PlayerController) || !IsValid(PlayerController->PlayerCameraManager))
	{
		return;
	}

	const FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
	const FRotator CameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
	const FVector ForwardVector = CameraRotation.Vector();
	const FVector RightVector = FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::Y);
	const FVector UpVector = FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::Z);
	const UPRRuntimePreloadDataAsset* RuntimePreloadData = ResolveRuntimePreloadData();
	const int32 PrewarmFrameCount = IsValid(RuntimePreloadData)
		? FMath::Max(1, RuntimePreloadData->RenderPrewarmFrameCount)
		: NiagaraRenderPrewarmFrameCount;
	const int32 AdvanceTickCount = IsValid(RuntimePreloadData)
		? FMath::Max(0, RuntimePreloadData->RenderPrewarmAdvanceTickCount)
		: NiagaraRenderPrewarmAdvanceTickCount;
	const float AdvanceTickDelta = IsValid(RuntimePreloadData)
		? FMath::Max(0.001f, RuntimePreloadData->RenderPrewarmAdvanceTickDelta)
		: NiagaraRenderPrewarmAdvanceTickDelta;

	bNiagaraRenderPrewarmComplete = false;
	bNiagaraRenderPrewarmInProgress = true;
	RemainingNiagaraRenderPrewarmFrames = PrewarmFrameCount;
	PrewarmNiagaraComponents.Reset();

	for (int32 SystemIndex = 0; SystemIndex < NiagaraSystems.Num(); ++SystemIndex)
	{
		UNiagaraSystem* NiagaraSystem = NiagaraSystems[SystemIndex];
		if (!IsValid(NiagaraSystem))
		{
			continue;
		}

		const float HorizontalOffset = static_cast<float>((SystemIndex % 5) - 2) * 35.0f;
		const float VerticalOffset = static_cast<float>((SystemIndex / 5) % 3) * 35.0f;
		const FVector SpawnLocation = CameraLocation
			+ ForwardVector * NiagaraRenderPrewarmDistance
			+ RightVector * HorizontalOffset
			+ UpVector * VerticalOffset;

		UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			LoadedWorld,
			NiagaraSystem,
			SpawnLocation,
			CameraRotation,
			FVector::OneVector,
			false,
			false,
			ENCPoolMethod::None,
			false);

		if (IsValid(NiagaraComponent))
		{
			NiagaraComponent->SetForceSolo(true);
			NiagaraComponent->SetAutoDestroy(false);
			NiagaraComponent->Activate(true);
			if (AdvanceTickCount > 0)
			{
				NiagaraComponent->AdvanceSimulation(AdvanceTickCount, AdvanceTickDelta);
			}

			PrewarmNiagaraComponents.Add(NiagaraComponent);
		}
	}

	if (PrewarmNiagaraComponents.IsEmpty())
	{
		FinishNiagaraRenderPrewarm();
		return;
	}

	LoadedWorld->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &UPRLoadingScreenSubsystem::AdvanceNiagaraRenderPrewarmFrame));
}

void UPRLoadingScreenSubsystem::AdvanceNiagaraRenderPrewarmFrame()
{
	UWorld* LoadedWorld = CurrentLoadedWorld.Get();
	if (!IsValid(LoadedWorld))
	{
		FinishNiagaraRenderPrewarm();
		TryReveal();
		return;
	}

	--RemainingNiagaraRenderPrewarmFrames;
	if (RemainingNiagaraRenderPrewarmFrames <= 0)
	{
		FinishNiagaraRenderPrewarm();
		TryReveal();
		return;
	}

	LoadedWorld->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &UPRLoadingScreenSubsystem::AdvanceNiagaraRenderPrewarmFrame));
}

void UPRLoadingScreenSubsystem::FinishNiagaraRenderPrewarm()
{
	for (UNiagaraComponent* NiagaraComponent : PrewarmNiagaraComponents)
	{
		if (IsValid(NiagaraComponent))
		{
			NiagaraComponent->Deactivate();
			NiagaraComponent->DestroyComponent();
		}
	}

	PrewarmNiagaraComponents.Reset();
	RemainingNiagaraRenderPrewarmFrames = 0;
	bNiagaraRenderPrewarmInProgress = false;
	bNiagaraRenderPrewarmComplete = true;
}

void UPRLoadingScreenSubsystem::CollectNiagaraRenderPrewarmSystems(UPRMapPreloadDataAsset* MapPreloadData, UPRRuntimePreloadDataAsset* RuntimePreloadData, TArray<UNiagaraSystem*>& OutNiagaraSystems) const
{
	if (IsValid(RuntimePreloadData))
	{
		for (const TSoftObjectPtr<UNiagaraSystem>& NiagaraSystem : RuntimePreloadData->RenderPrewarmNiagaraSystems)
		{
			if (UNiagaraSystem* LoadedSystem = NiagaraSystem.Get())
			{
				OutNiagaraSystems.AddUnique(LoadedSystem);
			}
		}
	}

	if (!IsValid(MapPreloadData))
	{
		return;
	}

	for (const TSoftObjectPtr<UNiagaraSystem>& NiagaraSystem : MapPreloadData->ExtraNiagaraSystems)
	{
		if (UNiagaraSystem* LoadedSystem = NiagaraSystem.Get())
		{
			OutNiagaraSystems.AddUnique(LoadedSystem);
		}
	}

	for (const TObjectPtr<UPREnemyPreloadSetDataAsset>& EnemyPreloadSet : MapPreloadData->EnemyPreloadSets)
	{
		if (!IsValid(EnemyPreloadSet))
		{
			continue;
		}

		for (const TSoftObjectPtr<UNiagaraSystem>& NiagaraSystem : EnemyPreloadSet->RenderPrewarmNiagaraSystems)
		{
			if (UNiagaraSystem* LoadedSystem = NiagaraSystem.Get())
			{
				OutNiagaraSystems.AddUnique(LoadedSystem);
			}
		}
	}
}

void UPRLoadingScreenSubsystem::StartShopUIPrewarm(UWorld* LoadedWorld, UPRMapPreloadDataAsset* MapPreloadData)
{
	bShopUIPrewarmComplete = true;
	if (!IsValid(LoadedWorld) || !IsValid(MapPreloadData) || !MapPreloadData->bPrewarmShopUI)
	{
		return;
	}

	if (!IsValid(ViewportLoadingWidget) || !ViewportLoadingWidget->IsInViewport())
	{
		return;
	}

	TArray<UPRShopComponent*> ShopComponents;
	for (TActorIterator<AActor> ActorIt(LoadedWorld); ActorIt; ++ActorIt)
	{
		AActor* Actor = *ActorIt;
		if (!IsValid(Actor))
		{
			continue;
		}

		if (UPRShopComponent* ShopComponent = Actor->FindComponentByClass<UPRShopComponent>())
		{
			ShopComponents.Add(ShopComponent);
		}
	}

	if (ShopComponents.IsEmpty())
	{
		return;
	}

	APRPlayerController* PlayerController = Cast<APRPlayerController>(LoadedWorld->GetFirstPlayerController());
	UPRUIControllerComponent* UIController = IsValid(PlayerController) ? PlayerController->GetUIController() : nullptr;
	if (!IsValid(UIController))
	{
		return;
	}

	bShopUIPrewarmComplete = false;
	UIController->PrewarmShopUI(
		ShopComponents,
		true,
		FSimpleDelegate::CreateUObject(this, &UPRLoadingScreenSubsystem::HandleShopUIPrewarmComplete));
}

void UPRLoadingScreenSubsystem::HandleShopUIPrewarmComplete()
{
	bShopUIPrewarmComplete = true;
	TryReveal();
}

void UPRLoadingScreenSubsystem::TryReveal()
{
	if (!bRequiredPreloadComplete || !bCharacterRuntimePreloadComplete || !bNiagaraRenderPrewarmComplete || !bShopUIPrewarmComplete || !bMinimumDisplayTimeComplete)
	{
		return;
	}

	SetLoadingState(EPRLoadingState::ReadyToReveal);
	HideViewportLoadingWidget();
	StopMoviePlayerScreen();
	UPRAssetManager::Get().ReleasePreloadGroup(EPRPreloadGroup::MapTransition);
	CurrentTravelReason = NAME_None;
	CurrentTravelMapName.Reset();
	SetLoadingState(EPRLoadingState::Idle);
}

void UPRLoadingScreenSubsystem::SetLoadingState(EPRLoadingState NewState)
{
	LoadingState = NewState;
}

bool UPRLoadingScreenSubsystem::IsLoadedWorldTravelDestination(UWorld* LoadedWorld) const
{
	if (!IsValid(LoadedWorld) || CurrentTravelMapName.IsEmpty())
	{
		return true;
	}

	const FString LoadedPackageName = UWorld::RemovePIEPrefix(LoadedWorld->GetOutermost()->GetName());
	const FString LoadedShortName = FPackageName::GetShortName(LoadedPackageName);
	FString TargetPackageName = UWorld::RemovePIEPrefix(CurrentTravelMapName);
	int32 ObjectNameSeparatorIndex = INDEX_NONE;
	if (TargetPackageName.FindChar(TEXT('.'), ObjectNameSeparatorIndex))
	{
		TargetPackageName.LeftInline(ObjectNameSeparatorIndex);
	}

	const FString TargetShortName = FPackageName::GetShortName(TargetPackageName);
	return LoadedPackageName == TargetPackageName || LoadedShortName == TargetShortName;
}

UPRMapPreloadDataAsset* UPRLoadingScreenSubsystem::ResolveMapPreloadData(UWorld* LoadedWorld) const
{
	if (!IsValid(LoadedWorld))
	{
		return nullptr;
	}

	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (!Settings)
	{
		return nullptr;
	}

	const FString LoadedPackageName = LoadedWorld->GetOutermost()->GetName();
	for (const TPair<TSoftObjectPtr<UWorld>, TSoftObjectPtr<UPRMapPreloadDataAsset>>& Pair : Settings->MapPreloadDataAssets)
	{
		if (Pair.Key.ToSoftObjectPath().GetLongPackageName() != LoadedPackageName)
		{
			continue;
		}

		return Pair.Value.LoadSynchronous();
	}

	return nullptr;
}

UPRRuntimePreloadDataAsset* UPRLoadingScreenSubsystem::ResolveRuntimePreloadData() const
{
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (!Settings || Settings->RuntimePreloadDataAsset.IsNull())
	{
		return nullptr;
	}

	return Settings->RuntimePreloadDataAsset.LoadSynchronous();
}

void UPRLoadingScreenSubsystem::CollectRequiredAssetPaths(UPRMapPreloadDataAsset* MapPreloadData, TArray<FSoftObjectPath>& OutAssetPaths) const
{
	if (!IsValid(MapPreloadData))
	{
		return;
	}

	for (const TSoftObjectPtr<UObject>& RequiredAsset : MapPreloadData->RequiredAssets)
	{
		if (!RequiredAsset.IsNull())
		{
			OutAssetPaths.AddUnique(RequiredAsset.ToSoftObjectPath());
		}
	}

	for (const TSoftClassPtr<UUserWidget>& WidgetClass : MapPreloadData->ExtraPreloadWidgetClasses)
	{
		if (!WidgetClass.IsNull())
		{
			OutAssetPaths.AddUnique(WidgetClass.ToSoftObjectPath());
		}
	}

	if (UPRFXSubsystem* FXSubsystem = IsValid(CurrentLoadedWorld.Get()) ? CurrentLoadedWorld->GetSubsystem<UPRFXSubsystem>() : nullptr)
	{
		FXSubsystem->CollectPreloadAssetPathsForTags(MapPreloadData->PreloadFXTags, OutAssetPaths);
	}

	for (const TObjectPtr<UPREnemyPreloadSetDataAsset>& EnemyPreloadSet : MapPreloadData->EnemyPreloadSets)
	{
		if (!IsValid(EnemyPreloadSet))
		{
			continue;
		}

		for (const TSoftClassPtr<APawn>& EnemyClass : EnemyPreloadSet->ExpectedEnemyClasses)
		{
			if (!EnemyClass.IsNull())
			{
				OutAssetPaths.AddUnique(EnemyClass.ToSoftObjectPath());
			}
		}

		for (const TSoftObjectPtr<UObject>& EnemyDataAsset : EnemyPreloadSet->EnemyDataAssets)
		{
			if (!EnemyDataAsset.IsNull())
			{
				OutAssetPaths.AddUnique(EnemyDataAsset.ToSoftObjectPath());
			}
		}

		if (UPRFXSubsystem* FXSubsystem = IsValid(CurrentLoadedWorld.Get()) ? CurrentLoadedWorld->GetSubsystem<UPRFXSubsystem>() : nullptr)
		{
			FXSubsystem->CollectPreloadAssetPathsForTags(EnemyPreloadSet->FXTags, OutAssetPaths);
		}

		for (const TSoftObjectPtr<UNiagaraSystem>& NiagaraSystem : EnemyPreloadSet->RenderPrewarmNiagaraSystems)
		{
			if (!NiagaraSystem.IsNull())
			{
				OutAssetPaths.AddUnique(NiagaraSystem.ToSoftObjectPath());
			}
		}

		for (const TSoftObjectPtr<USoundBase>& SFX : EnemyPreloadSet->SFX)
		{
			if (!SFX.IsNull())
			{
				OutAssetPaths.AddUnique(SFX.ToSoftObjectPath());
			}
		}
	}

	for (const TSoftObjectPtr<UNiagaraSystem>& NiagaraSystem : MapPreloadData->ExtraNiagaraSystems)
	{
		if (!NiagaraSystem.IsNull())
		{
			OutAssetPaths.AddUnique(NiagaraSystem.ToSoftObjectPath());
		}
	}

	for (const TSoftObjectPtr<USoundBase>& SFX : MapPreloadData->ExtraSFX)
	{
		if (!SFX.IsNull())
		{
			OutAssetPaths.AddUnique(SFX.ToSoftObjectPath());
		}
	}

	for (UPRShopDataAsset* ShopData : MapPreloadData->ExplicitShopDataAssets)
	{
		if (!IsValid(ShopData))
		{
			continue;
		}

		OutAssetPaths.AddUnique(FSoftObjectPath(ShopData));
		for (const FPRShopEntry& Entry : ShopData->Entries)
		{
			const FSoftObjectPath ItemPath = UPRAssetManager::Get().GetPrimaryAssetPath(Entry.ItemAssetId);
			if (ItemPath.IsValid())
			{
				OutAssetPaths.AddUnique(ItemPath);
			}

			for (const FPRShopMaterialCost& MaterialCost : Entry.BuyMaterialCosts)
			{
				const FSoftObjectPath MaterialPath = UPRAssetManager::Get().GetPrimaryAssetPath(MaterialCost.MaterialAssetId);
				if (MaterialPath.IsValid())
				{
					OutAssetPaths.AddUnique(MaterialPath);
				}
			}
		}
	}
}
