// Copyright ProjectR. All Rights Reserved.

#include "PRMenuGameMode.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Game/PRGameInstance.h"
#include "ProjectR/Game/PRMenuHUD.h"
#include "ProjectR/Player/PRMenuPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/System/PRDeveloperSettings.h"
#include "ProjectR/World/PRWorldRegistry.h"

APRMenuGameMode::APRMenuGameMode()
{
	// 메뉴는 복제 불필요. GameMode 기본값 유지
	bUseSeamlessTravel = true;

	// 메뉴 플레이어 입력과 HUD 표시 클래스 지정
	PlayerControllerClass = APRMenuPlayerController::StaticClass();
	PlayerStateClass = APRPlayerState::StaticClass();
	HUDClass = APRMenuHUD::StaticClass();
	DefaultPawnClass = nullptr;
	PreviewCharacterClass = APRPlayerCharacter::StaticClass();
}

/*~ AGameModeBase Interface ~*/

void APRMenuGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 메뉴 진입 직후 프리뷰 런타임 소스 준비
	EnsurePreviewCharacter(NewPlayer);
}

void APRMenuGameMode::Logout(AController* Exiting)
{
	if (PreviewController.Get() == Exiting && IsValid(PreviewCharacter))
	{
		PreviewCharacter->Destroy();
		PreviewCharacter = nullptr;
		PreviewController = nullptr;
	}

	Super::Logout(Exiting);
}

/*~ APRMenuGameMode Interface ~*/

bool APRMenuGameMode::RequestStartSession(APlayerController* RequestingController, const FString& Address)
{
	// 메뉴 위젯 소유 컨트롤러 유효성 확인
	if (!IsValid(RequestingController))
	{
		return false;
	}

	UPRGameInstance* GameInstance = Cast<UPRGameInstance>(GetGameInstance());
	if (!IsValid(GameInstance))
	{
		return false;
	}

	const FString TrimmedAddress = Address.TrimStartAndEnd();
	if (TrimmedAddress.IsEmpty())
	{
		FPRHostSessionParams HostParams;
		const FPRWorldSaveData& LocalWorldSave = GameInstance->GetLocalWorldSave();
		// 세션 서브시스템 OpenLevel 경로에 전달할 리슨 서버 설정
		HostParams.MapName = ResolveHostMapNameFromSave(LocalWorldSave);
		HostParams.MaxPlayers = HostMaxPlayers;

		// 인게임 GameMode 초기화 단계에서 복원할 월드 진행 상태 예약
		GameInstance->SetPendingWorldSaveData(LocalWorldSave);
		if (LocalWorldSave.SavedSpawnPoint.IsValid())
		{
			// 이어하기 최초 스폰 지점 예약
			GameInstance->SetPendingTravelSpawnPointId(LocalWorldSave.SavedSpawnPoint.SpawnPointId);
		}

		GameInstance->HostSession(HostParams);
		return true;
	}

	FPRJoinSessionParams JoinParams;
	// 입력 주소는 세션 서브시스템에서 형식 검증
	JoinParams.Address = TrimmedAddress;

	GameInstance->JoinSession(JoinParams);
	return true;
}

bool APRMenuGameMode::ApplyPreviewSaveData(APlayerController* RequestingController, const FPRCharacterSaveData& InSaveData)
{
	APRPlayerState* PlayerState = GetPreviewPlayerState(RequestingController);
	if (!IsValid(PlayerState))
	{
		return false;
	}

	APRPlayerCharacter* TargetPreviewCharacter = EnsurePreviewCharacter(RequestingController);
	if (!IsValid(TargetPreviewCharacter))
	{
		return false;
	}

	// 실제 PlayerState 시스템에 세이브 데이터 적용
	PlayerState->ApplySaveData(InSaveData);
	ConfigurePreviewCharacter(TargetPreviewCharacter);
	return true;
}

APRPlayerState* APRMenuGameMode::GetPreviewPlayerState(APlayerController* RequestingController) const
{
	APlayerController* SourceController = IsValid(RequestingController)
		? RequestingController
		: PreviewController.Get();
	return IsValid(SourceController) ? SourceController->GetPlayerState<APRPlayerState>() : nullptr;
}

APRPlayerCharacter* APRMenuGameMode::GetPreviewCharacter(APlayerController* RequestingController) const
{
	if (!IsValid(PreviewCharacter))
	{
		return nullptr;
	}

	if (IsValid(RequestingController) && PreviewController.IsValid() && PreviewController.Get() != RequestingController)
	{
		return nullptr;
	}

	return PreviewCharacter;
}

APRPlayerCharacter* APRMenuGameMode::EnsurePreviewCharacter(APlayerController* RequestingController)
{
	if (!IsValid(RequestingController))
	{
		return nullptr;
	}

	if (PreviewController.IsValid() && PreviewController.Get() != RequestingController && IsValid(PreviewCharacter))
	{
		PreviewCharacter->Destroy();
		PreviewCharacter = nullptr;
	}

	PreviewController = RequestingController;
	bool bSpawnedPreviewCharacter = false;
	if (!IsValid(PreviewCharacter))
	{
		UWorld* World = GetWorld();
		if (!IsValid(World))
		{
			return nullptr;
		}

		TSubclassOf<APRPlayerCharacter> SpawnClass = PreviewCharacterClass;
		if (!IsValid(SpawnClass.Get()))
		{
			SpawnClass = APRPlayerCharacter::StaticClass();
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = RequestingController;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const FTransform SpawnTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, -90000.0f));
		PreviewCharacter = World->SpawnActor<APRPlayerCharacter>(SpawnClass, SpawnTransform, SpawnParameters);
		bSpawnedPreviewCharacter = IsValid(PreviewCharacter);
	}

	if (!IsValid(PreviewCharacter))
	{
		return nullptr;
	}

	ConfigurePreviewCharacter(PreviewCharacter);
	if (bSpawnedPreviewCharacter)
	{
		PreviewCharacter->InitCharacter(RequestingController);
	}

	return PreviewCharacter;
}

void APRMenuGameMode::ConfigurePreviewCharacter(APRPlayerCharacter* InPreviewCharacter) const
{
	if (!IsValid(InPreviewCharacter))
	{
		return;
	}

	// 메뉴 월드 직접 노출과 충돌 차단
	InPreviewCharacter->SetActorLocation(FVector(0.0f, 0.0f, -90000.0f));
	InPreviewCharacter->SetActorEnableCollision(false);
	InPreviewCharacter->SetActorTickEnabled(false);
	if (UCapsuleComponent* CapsuleComponent = InPreviewCharacter->GetCapsuleComponent())
	{
		CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

FName APRMenuGameMode::ResolveHostMapNameFromSave(const FPRWorldSaveData& WorldSaveData) const
{
	if (!WorldSaveData.SavedSpawnPoint.IsValid())
	{
		return HostMapName;
	}

	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	const UPRWorldRegistry* WorldRegistry = IsValid(Settings) ? Settings->GetWorldRegistrySync() : nullptr;
	if (!IsValid(WorldRegistry))
	{
		return HostMapName;
	}

	TSoftObjectPtr<UWorld> MapAsset;
	if (!WorldRegistry->ResolveMapAsset(WorldSaveData.SavedSpawnPoint.WorldId, MapAsset) || MapAsset.IsNull())
	{
		return HostMapName;
	}

	const FString MapPackageName = MapAsset.GetLongPackageName();
	return MapPackageName.IsEmpty() ? HostMapName : FName(*MapPackageName);
}
