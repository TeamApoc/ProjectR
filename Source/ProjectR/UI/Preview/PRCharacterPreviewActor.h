// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "PRCharacterPreviewActor.generated.h"

class APRPlayerCharacter;
class APRWeaponActor;
class USceneCaptureComponent2D;
class USkeletalMeshComponent;
class USpringArmComponent;
class USpotLightComponent;
class UTextureRenderTarget2D;
class UPRWeaponManagerComponent;

// 인벤토리 캐릭터 프리뷰를 위한 별도 3D 렌더 액터
UCLASS()
class PROJECTR_API APRCharacterPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	// 프리뷰 액터 기본 컴포넌트를 초기화한다
	APRCharacterPreviewActor();

	// 프리뷰 캡처가 사용할 렌더 타겟을 설정한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory|Preview")
	void SetRenderTargetToSceneCapture(UTextureRenderTarget2D* InRenderTarget);

	// 플레이어 캐릭터의 외형과 장착 무기 상태를 프리뷰에 반영한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory|Preview")
	void RefreshPreviewActorFromPlayer(APRPlayerCharacter* SourceCharacter, UPRWeaponManagerComponent* WeaponManagerComponent);

	// 프리뷰 캡처를 즉시 갱신한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory|Preview")
	void CapturePreview();

	// 프리뷰 캡처를 매 프레임 갱신할지 설정한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory|Preview")
	void SetContinuousCapture(bool bEnabled);

protected:
	/*~ AActor Interface ~*/
	// 액터 제거 시 생성한 무기 프리뷰 액터를 정리한다
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 캐릭터 메시를 소스 캐릭터 기준으로 갱신한다
	void RefreshCharacterMesh(APRPlayerCharacter* SourceCharacter);

	// 무기 매니저의 공개 비주얼 상태를 기준으로 무기 프리뷰를 갱신한다
	void RefreshWeaponPreview(UPRWeaponManagerComponent* WeaponManagerComponent);

	// 지정 슬롯의 무기 프리뷰 액터를 생성하거나 갱신한다
	void RefreshWeaponActorForSlot(EPRWeaponSlotType SlotType, const FPRWeaponVisualInfo& VisualInfo);

	// 지정 슬롯의 무기 프리뷰 액터를 제거한다
	void DestroyWeaponActorForSlot(EPRWeaponSlotType SlotType);

	// 지정 슬롯의 무기 프리뷰 액터를 반환한다
	APRWeaponActor* GetWeaponActorBySlot(EPRWeaponSlotType SlotType) const;

	// 지정 슬롯의 무기 프리뷰 액터 참조를 설정한다
	void SetWeaponActorBySlot(EPRWeaponSlotType SlotType, APRWeaponActor* NewWeaponActor);

	// 지정 슬롯의 무기 프리뷰 액터 참조를 비운다
	void ClearWeaponActorBySlot(EPRWeaponSlotType SlotType);

protected:
	// 프리뷰 액터 루트로 사용하는 캐릭터 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USkeletalMeshComponent> PreviewMeshComponent;

	// 프리뷰 캡처 거리와 각도를 제어하는 스프링 암
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USpringArmComponent> CaptureSpringArmComponent;

	// 프리뷰 이미지를 생성하는 캡처 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USceneCaptureComponent2D> SceneCaptureComponent;

	// 프리뷰 조명 거리와 각도를 제어하는 스프링 암
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USpringArmComponent> LightSpringArmComponent;

	// 프리뷰 전용 조명
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USpotLightComponent> KeyLightComponent;

private:
	// 주무기 프리뷰 액터
	UPROPERTY(Transient)
	TObjectPtr<APRWeaponActor> PrimaryWeaponActor;

	// 보조무기 프리뷰 액터
	UPROPERTY(Transient)
	TObjectPtr<APRWeaponActor> SecondaryWeaponActor;
};
