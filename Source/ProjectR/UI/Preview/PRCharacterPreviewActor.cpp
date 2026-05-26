// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRCharacterPreviewActor.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/ItemSystem/Actors/PRWeaponActor.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"

namespace
{
	bool IsPreviewSupportedSlot(EPRWeaponSlotType SlotType)
	{
		return SlotType == EPRWeaponSlotType::Primary || SlotType == EPRWeaponSlotType::Secondary;
	}

	FName GetPreviewAttachSocketName(const UPRWeaponDataAsset* WeaponData, EPRArmedState ArmedState)
	{
		if (!IsValid(WeaponData))
		{
			return NAME_None;
		}

		if (ArmedState == EPRArmedState::Armed)
		{
			return FName(PREnumHelper::EnumToFName(WeaponData->ArmedSocketName));
		}

		return FName(PREnumHelper::EnumToFName(WeaponData->StowedSocketName));
	}

	void AttachWeaponActorToPreviewMesh(APRWeaponActor* WeaponActor, USkeletalMeshComponent* PreviewMeshComponent, const UPRWeaponDataAsset* WeaponData, EPRArmedState ArmedState)
	{
		if (!IsValid(WeaponActor) || !IsValid(PreviewMeshComponent) || !IsValid(WeaponData))
		{
			return;
		}

		const FName AttachSocketName = GetPreviewAttachSocketName(WeaponData, ArmedState);
		if (AttachSocketName.IsNone() || !PreviewMeshComponent->DoesSocketExist(AttachSocketName))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[CharacterPreviewActor] 무기 프리뷰 부착 소켓 없음 | Weapon = %s | Socket = %s"),
				*GetNameSafe(WeaponData),
				*AttachSocketName.ToString());
			return;
		}

		WeaponActor->AttachToComponent(
			PreviewMeshComponent,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			AttachSocketName);
	}
}

APRCharacterPreviewActor::APRCharacterPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	// 캐릭터 프리뷰 메시
	PreviewMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewMeshComponent"));
	SetRootComponent(PreviewMeshComponent);
	PreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMeshComponent->SetGenerateOverlapEvents(false);
	PreviewMeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// 씬 캡처 컴포넌트 스프링 암
	CaptureSpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("CaptureSpringArmComponent"));
	CaptureSpringArmComponent->SetupAttachment(PreviewMeshComponent);
	CaptureSpringArmComponent->TargetArmLength = 230.0f;
	CaptureSpringArmComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
	CaptureSpringArmComponent->SetRelativeRotation(FRotator(0.0f, -7.0f, -90.0f));
	CaptureSpringArmComponent->bDoCollisionTest = false;
	CaptureSpringArmComponent->bUsePawnControlRotation = false;
	CaptureSpringArmComponent->bInheritPitch = false;
	CaptureSpringArmComponent->bInheritYaw = false;
	CaptureSpringArmComponent->bInheritRoll = false;

	// ===== 씬 캡처 컴포넌트 =====
	SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent"));
	SceneCaptureComponent->SetupAttachment(CaptureSpringArmComponent, USpringArmComponent::SocketName);
	SceneCaptureComponent->SetRelativeLocation(FVector::ZeroVector);
	SceneCaptureComponent->SetRelativeRotation(FRotator::ZeroRotator);
	SceneCaptureComponent->FOVAngle = 35.0f;
	SceneCaptureComponent->bCaptureEveryFrame = false;
	SceneCaptureComponent->bCaptureOnMovement = false;
	SceneCaptureComponent->CaptureSource = SCS_SceneColorHDR;
	SceneCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;

	// 월드 환경 요소 차단 — 전용 채널 1 DirectionalLight만 사용
	SceneCaptureComponent->ShowFlags.SetAtmosphere(false);
	SceneCaptureComponent->ShowFlags.SetFog(false);
	SceneCaptureComponent->ShowFlags.SetVolumetricFog(false);
	SceneCaptureComponent->ShowFlags.SetSkyLighting(false);

	// 그림자 — 전용 조명이므로 불필요
	SceneCaptureComponent->ShowFlags.SetDynamicShadows(false);
	SceneCaptureComponent->ShowFlags.SetContactShadows(false);
	SceneCaptureComponent->ShowFlags.SetAmbientOcclusion(false);
	SceneCaptureComponent->ShowFlags.SetDistanceFieldAO(false);

	// 격리된 씬이므로 환경 반사·간접광 불필요
	SceneCaptureComponent->ShowFlags.SetGlobalIllumination(false);
	SceneCaptureComponent->ShowFlags.SetReflectionEnvironment(false);
	SceneCaptureComponent->ShowFlags.SetScreenSpaceReflections(false);
	SceneCaptureComponent->ShowFlags.SetIndirectLightingCache(false);

	// 포스트 프로세스 — 저해상도 프리뷰에 불필요한 효과 제거
	SceneCaptureComponent->ShowFlags.SetEyeAdaptation(false);
	SceneCaptureComponent->ShowFlags.SetBloom(false);
	SceneCaptureComponent->ShowFlags.SetMotionBlur(false);
	SceneCaptureComponent->ShowFlags.SetLensFlares(false);
	SceneCaptureComponent->ShowFlags.SetToneCurve(false);

	// 에디터 시각화 요소 차단
	SceneCaptureComponent->ShowFlags.SetCameraFrustums(false);
	SceneCaptureComponent->ShowFlags.SetBillboardSprites(false);

	// ===== 라이트 스프링 암 =====
	LightSpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("LightSpringArmComponent"));
	LightSpringArmComponent->SetupAttachment(PreviewMeshComponent);
	LightSpringArmComponent->TargetArmLength = 320.0f;
	LightSpringArmComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
	LightSpringArmComponent->SetRelativeRotation(FRotator(0.0f, 0.0f, -90.0f));
	LightSpringArmComponent->bDoCollisionTest = false;
	LightSpringArmComponent->bUsePawnControlRotation = false;
	LightSpringArmComponent->bInheritPitch = false;
	LightSpringArmComponent->bInheritYaw = false;
	LightSpringArmComponent->bInheritRoll = false;

	// ===== 라이트 컴포넌트 =====
	KeyLightComponent = CreateDefaultSubobject<USpotLightComponent>(TEXT("KeyLightComponent"));
	KeyLightComponent->SetupAttachment(LightSpringArmComponent, USpringArmComponent::SocketName);
	KeyLightComponent->SetRelativeLocation(FVector::ZeroVector);
	KeyLightComponent->SetRelativeRotation(FRotator::ZeroRotator);
	KeyLightComponent->SetIntensity(16000.0f);
	KeyLightComponent->SetAttenuationRadius(650.0f);
	KeyLightComponent->SetOuterConeAngle(55.0f);
}

void APRCharacterPreviewActor::SetRenderTargetToSceneCapture(UTextureRenderTarget2D* InRenderTarget)
{
	if (!IsValid(SceneCaptureComponent))
	{
		return;
	}

	// 씬 캡쳐 컴포넌트의 텍스처 타겟을 설정한다
	SceneCaptureComponent->TextureTarget = InRenderTarget;
}

void APRCharacterPreviewActor::RefreshPreviewActorFromPlayer(APRPlayerCharacter* SourceCharacter, UPRWeaponManagerComponent* WeaponManagerComponent)
{
	RefreshCharacterMesh(SourceCharacter);
	RefreshWeaponPreview(WeaponManagerComponent);
	CapturePreview();
}

void APRCharacterPreviewActor::SetContinuousCapture(bool bEnabled)
{
	if (!IsValid(SceneCaptureComponent))
	{
		return;
	}

	SceneCaptureComponent->bCaptureEveryFrame = bEnabled;
	SceneCaptureComponent->bCaptureOnMovement = bEnabled;
}

void APRCharacterPreviewActor::CapturePreview()
{
	if (!IsValid(SceneCaptureComponent))
	{
		return;
	}

	if (!IsValid(SceneCaptureComponent->TextureTarget))
	{
		return;
	}

	SceneCaptureComponent->ClearShowOnlyComponents();

	if (IsValid(PreviewMeshComponent))
	{
		SceneCaptureComponent->ShowOnlyComponent(PreviewMeshComponent);
	}

	if (APRWeaponActor* PrimaryWeapon = GetWeaponActorBySlot(EPRWeaponSlotType::Primary))
	{
		if (USkeletalMeshComponent* WeaponMeshComponent = PrimaryWeapon->GetWeaponMeshComponent())
		{
			SceneCaptureComponent->ShowOnlyComponent(WeaponMeshComponent);
		}
	}

	if (APRWeaponActor* SecondaryWeapon = GetWeaponActorBySlot(EPRWeaponSlotType::Secondary))
	{
		if (USkeletalMeshComponent* WeaponMeshComponent = SecondaryWeapon->GetWeaponMeshComponent())
		{
			SceneCaptureComponent->ShowOnlyComponent(WeaponMeshComponent);
		}
	}

	SceneCaptureComponent->CaptureScene();
}

/*~ AActor Interface ~*/

void APRCharacterPreviewActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyWeaponActorForSlot(EPRWeaponSlotType::Primary);
	DestroyWeaponActorForSlot(EPRWeaponSlotType::Secondary);

	Super::EndPlay(EndPlayReason);
}

void APRCharacterPreviewActor::RefreshCharacterMesh(APRPlayerCharacter* SourceCharacter)
{
	if (!IsValid(PreviewMeshComponent))
	{
		return;
	}

	if (!IsValid(SourceCharacter) || !IsValid(SourceCharacter->GetMesh()))
	{
		PreviewMeshComponent->SetSkeletalMesh(nullptr);
		return;
	}

	// 원본 캐릭터의 메시를 프리뷰 액터에 적용한다
	USkeletalMeshComponent* SourceMeshComponent = SourceCharacter->GetMesh();
	PreviewMeshComponent->SetSkeletalMesh(SourceMeshComponent->GetSkeletalMeshAsset());

	PreviewMeshComponent->UpdateBounds();
}

void APRCharacterPreviewActor::RefreshWeaponPreview(UPRWeaponManagerComponent* WeaponManagerComponent)
{
	// 무기 매니저 컴포넌트가 없다면
	if (!IsValid(WeaponManagerComponent))
	{
		// 프리뷰 무기 디스트로이
		DestroyWeaponActorForSlot(EPRWeaponSlotType::Primary);
		DestroyWeaponActorForSlot(EPRWeaponSlotType::Secondary);
		return;
	}

	const FPRWeaponVisualInfo& PrimaryVisualInfo = WeaponManagerComponent->GetVisualInfoBySlotType(EPRWeaponSlotType::Primary);
	const FPRWeaponVisualInfo& SecondaryVisualInfo = WeaponManagerComponent->GetVisualInfoBySlotType(EPRWeaponSlotType::Secondary);

	// 프리뷰 무기 갱신
	RefreshWeaponActorForSlot(EPRWeaponSlotType::Primary, PrimaryVisualInfo);
	RefreshWeaponActorForSlot(EPRWeaponSlotType::Secondary, SecondaryVisualInfo);
}

void APRCharacterPreviewActor::RefreshWeaponActorForSlot(EPRWeaponSlotType SlotType, const FPRWeaponVisualInfo& VisualInfo)
{
	if (!IsPreviewSupportedSlot(SlotType))
	{
		return;
	}

	if (VisualInfo.IsEmpty() || !IsValid(VisualInfo.WeaponData) || !IsValid(VisualInfo.WeaponData->WeaponActorClass))
	{
		DestroyWeaponActorForSlot(SlotType);
		return;
	}
	
	APRWeaponActor* WeaponActor = GetWeaponActorBySlot(SlotType);
	// 액터 유효성에 따라 재스폰 여부 결정
	const bool bNeedsRespawn = !IsValid(WeaponActor)
		|| WeaponActor->GetClass() != VisualInfo.WeaponData->WeaponActorClass;

	if (bNeedsRespawn)
	{
		//기존 액터 파괴
		DestroyWeaponActorForSlot(SlotType);
		
		if (!IsValid(GetWorld()))
		{
			return;
		}
		
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		// 무기 액터 스폰 
		WeaponActor = GetWorld()->SpawnActor<APRWeaponActor>(
			VisualInfo.WeaponData->WeaponActorClass,
			GetActorTransform(),
			SpawnParameters);

		// 무기 액터 캐싱
		SetWeaponActorBySlot(SlotType, WeaponActor);
	}

	if (!IsValid(WeaponActor))
	{
		return;
	}

	// 무기액터 콜리전, 히든 끄기
	WeaponActor->SetActorEnableCollision(false);
	WeaponActor->SetActorHiddenInGame(false);
	AttachWeaponActorToPreviewMesh(WeaponActor, PreviewMeshComponent, VisualInfo.WeaponData, EPRArmedState::Unarmed);
}

void APRCharacterPreviewActor::DestroyWeaponActorForSlot(EPRWeaponSlotType SlotType)
{
	APRWeaponActor* WeaponActor = GetWeaponActorBySlot(SlotType);
	// 무기 액터가 유효하면
	if (IsValid(WeaponActor))
	{
		// 무기 액터 파괴
		WeaponActor->Destroy();
	}

	// 캐시된 포인터 정리
	ClearWeaponActorBySlot(SlotType);
}

APRWeaponActor* APRCharacterPreviewActor::GetWeaponActorBySlot(EPRWeaponSlotType SlotType) const
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return PrimaryWeaponActor;
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return SecondaryWeaponActor;
	}

	return nullptr;
}

void APRCharacterPreviewActor::SetWeaponActorBySlot(EPRWeaponSlotType SlotType, APRWeaponActor* NewWeaponActor)
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		PrimaryWeaponActor = NewWeaponActor;
	}
	else if (SlotType == EPRWeaponSlotType::Secondary)
	{
		SecondaryWeaponActor = NewWeaponActor;
	}
}

void APRCharacterPreviewActor::ClearWeaponActorBySlot(EPRWeaponSlotType SlotType)
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		PrimaryWeaponActor = nullptr;
	}
	else if (SlotType == EPRWeaponSlotType::Secondary)
	{
		SecondaryWeaponActor = nullptr;
	}
}
