// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRCharacterPreviewWidget.h"

#include "Components/Image.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/UI/Preview/PRCharacterPreviewActor.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"

void UPRCharacterPreviewWidget::SetPreviewSources(APRPlayerCharacter* InSourceCharacter, UPRWeaponManagerComponent* InWeaponManagerComponent)
{
	SourceCharacter = InSourceCharacter;
	WeaponManagerComponent = InWeaponManagerComponent;

	// 소스 설정 후 프리뷰 갱신
	RefreshPreview();
}

void UPRCharacterPreviewWidget::RefreshPreview()
{
	if (!GetOwningLocalPlayer())
	{
		return;
	}
	
	// 소스가 비어있는 경우 플레이어 기준으로 갱신
	ResolvePreviewSourcesFromOwningPlayerIfNeeded();
	// 액터 스폰 혹은 기존 액터 재사용
	EnsurePreviewActor();
	// 프리뷰 이미지 갱신 
	RefreshPreviewBrush();

	if (!IsValid(PreviewActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CharacterPreviewWidget] 프리뷰 갱신 실패. PreviewActor 없음"));
		return;
	}

	if (!IsValid(SourceCharacter))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CharacterPreviewWidget] 프리뷰 갱신 실패. SourceCharacter 없음"));
		return;
	}

	// 씬 캡처 컴포넌트에 렌더 타겟 설정
	PreviewActor->SetRenderTargetToSceneCapture(IsValid(DynamicRenderTarget) ? DynamicRenderTarget.Get() : PreviewRenderTarget.Get());
	// 
	PreviewActor->RefreshPreviewActorFromPlayer(SourceCharacter, WeaponManagerComponent);
	PreviewActor->SetContinuousCapture(true);
}

/*~ UUserWidget Interface ~*/

void UPRCharacterPreviewWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshPreview();
}

void UPRCharacterPreviewWidget::NativeDestruct()
{
	if (IsValid(PreviewActor))
	{
		PreviewActor->SetContinuousCapture(false);
	}

	DestroyPreviewActor();

	SourceCharacter = nullptr;
	WeaponManagerComponent = nullptr;

	Super::NativeDestruct();
}

void UPRCharacterPreviewWidget::EnsurePreviewActor()
{
	if (IsValid(PreviewActor))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CharacterPreviewWidget] 프리뷰 액터 생성 실패. World 없음"));
		return;
	}

	TSubclassOf<APRCharacterPreviewActor> SpawnClass = PreviewActorClass;
	if (!IsValid(SpawnClass.Get()))
	{
		SpawnClass = APRCharacterPreviewActor::StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = SourceCharacter.Get();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FTransform SpawnTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, -100000.0f));
	PreviewActor = World->SpawnActor<APRCharacterPreviewActor>(SpawnClass, SpawnTransform, SpawnParameters);
	if (!IsValid(PreviewActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CharacterPreviewWidget] 프리뷰 액터 생성 실패"));
		return;
	}

	if (!IsValid(DynamicRenderTarget) && IsValid(PreviewRenderTarget))
	{
		DynamicRenderTarget = DuplicateObject<UTextureRenderTarget2D>(PreviewRenderTarget, this);
	}

	PreviewActor->SetRenderTargetToSceneCapture(IsValid(DynamicRenderTarget) ? DynamicRenderTarget.Get() : PreviewRenderTarget.Get());
}

void UPRCharacterPreviewWidget::DestroyPreviewActor()
{
	if (IsValid(PreviewActor))
	{
		PreviewActor->Destroy();
	}

	PreviewActor = nullptr;
}

void UPRCharacterPreviewWidget::RefreshPreviewBrush()
{
	if (!IsValid(CharacterPreviewImage))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CharacterPreviewWidget] CharacterPreviewImage 바인딩 없음"));
		return;
	}

	if (!IsValid(PreviewRenderTarget))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CharacterPreviewWidget] PreviewRenderTarget 설정 없음"));
		CharacterPreviewImage->SetBrushResourceObject(nullptr);
		CharacterPreviewImage->SetDesiredSizeOverride(FVector2D::ZeroVector);
		return;
	}

	if (!IsValid(DynamicRenderTarget))
	{
		DynamicRenderTarget = DuplicateObject<UTextureRenderTarget2D>(PreviewRenderTarget, this);
	}

	if (IsValid(PreviewMaterial))
	{
		if (!IsValid(DynamicMaterial))
		{
			DynamicMaterial = UMaterialInstanceDynamic::Create(PreviewMaterial, this);
		}
		
		// 머티리얼 내부에 사용될 수 있는 주요 텍스처 파라미터 이름 시도
		DynamicMaterial->SetTextureParameterValue(TEXT("RenderTarget"), DynamicRenderTarget);
		CharacterPreviewImage->SetBrushFromMaterial(DynamicMaterial);
	}
	else
	{
		CharacterPreviewImage->SetBrushResourceObject(DynamicRenderTarget);
	}

	// 렌더타겟의 이미지 사이즈 오버라이드
	CharacterPreviewImage->SetDesiredSizeOverride(FVector2D(DynamicRenderTarget->SizeX, DynamicRenderTarget->SizeY));
}

void UPRCharacterPreviewWidget::ResolvePreviewSourcesFromOwningPlayerIfNeeded()
{
	// 이미 소스가 있다면
	if (IsValid(SourceCharacter) && IsValid(WeaponManagerComponent))
	{
		// 조기 리턴
		return;
	}
	
	APlayerController* OwningPlayerController = GetOwningPlayer();
	if (!IsValid(OwningPlayerController))
	{
		return;
	}

	APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(OwningPlayerController->GetPawn());
	if (!IsValid(PlayerCharacter))
	{
		return;
	}

	// 소스 캐릭터와 무기 매니저 컴포넌트가 유효하지 않을 때 Owning Player 기준으로 설정
	if (!IsValid(SourceCharacter))
	{
		SourceCharacter = PlayerCharacter;
	}

	if (!IsValid(WeaponManagerComponent))
	{
		WeaponManagerComponent = PlayerCharacter->GetWeaponManager();
	}
}
