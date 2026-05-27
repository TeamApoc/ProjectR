// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRCharacterPreviewWidget.generated.h"

class APRCharacterPreviewActor;
class APRPlayerCharacter;
class UImage;
class UMaterialInterface;
class UPRWeaponManagerComponent;
class UTextureRenderTarget2D;

// UI에 캐릭터 3D 프리뷰 이미지를 표시하는 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRCharacterPreviewWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 프리뷰에 사용할 플레이어 캐릭터와 무기 상태 소스를 설정한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory|Preview")
	void SetPreviewSources(APRPlayerCharacter* InSourceCharacter, UPRWeaponManagerComponent* InWeaponManagerComponent);

	// 현재 소스 상태를 기준으로 프리뷰를 갱신한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory|Preview")
	void RefreshPreview();

	// 현재 사용 중인 프리뷰 액터를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|Preview")
	APRCharacterPreviewActor* GetPreviewActor() const { return PreviewActor; }

protected:
	/*~ UUserWidget Interface ~*/
	// 화면에 표시될 때 프리뷰 액터와 이미지를 준비한다
	virtual void NativeConstruct() override;

	// 화면에서 제거될 때 프리뷰 액터 참조를 정리한다
	virtual void NativeDestruct() override;

private:
	// 프리뷰 액터를 생성하거나 기존 액터를 재사용한다
	void EnsurePreviewActor();

	// 프리뷰 액터를 정리한다
	void DestroyPreviewActor();

	// 렌더 타겟을 이미지 위젯에 연결한다
	void RefreshPreviewBrush();

	// 소스가 비어 있으면 Owning Player 기준으로 프리뷰 소스를 조회한다
	void ResolvePreviewSourcesFromOwningPlayerIfNeeded();

protected:
	// UMG에서 바인딩할 캐릭터 프리뷰 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Preview")
	TObjectPtr<UImage> CharacterPreviewImage;

	// 프리뷰에 사용할 액터 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preview")
	TSubclassOf<APRCharacterPreviewActor> PreviewActorClass;

	// 프리뷰 캡처 결과를 받을 렌더 타겟
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preview")
	TObjectPtr<UTextureRenderTarget2D> PreviewRenderTarget;

	// 프리뷰 렌더 타겟을 표시할 UI 머티리얼
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preview")
	TObjectPtr<UMaterialInterface> PreviewMaterial;

private:
	// 프리뷰 메시와 캡처를 담당하는 액터
	UPROPERTY(Transient)
	TObjectPtr<APRCharacterPreviewActor> PreviewActor;

	// 프리뷰 기준이 되는 실제 플레이어 캐릭터
	UPROPERTY(Transient)
	TObjectPtr<APRPlayerCharacter> SourceCharacter;

	// 장착 무기 상태를 읽을 무기 매니저
	UPROPERTY(Transient)
	TObjectPtr<UPRWeaponManagerComponent> WeaponManagerComponent;

	// 동적으로 복제된 고유 렌더 타겟 (PIE 환경에서 여러 클라가 에셋을 공유하는 문제 방지)
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> DynamicRenderTarget;

	// 동적으로 생성된 UI 머티리얼 인스턴스
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
};
