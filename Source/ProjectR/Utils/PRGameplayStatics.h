// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectR/FX/PRFXTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PRGameplayStatics.generated.h"

class UPREquipmentManagerComponent;
class UPRWeaponManagerComponent;
class UPRGameInstance;
class UGameplayEffect;
class APawn;
class UAbilitySystemComponent;
class UPRAbilitySystemComponent;
class UPRInventoryComponent;
class UPRQuickSlotComponent;
/**
 *
 */
UCLASS()
class PROJECTR_API UPRGameplayStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** 액터 및 모든 ChildActor를 재귀 탐색하여 UMeshComponent를 OutMeshes에 수집 */
	UFUNCTION(BlueprintCallable, Category = "PR|Utils")
	static void GetAllMeshComponents(AActor* Actor, TArray<UMeshComponent*>& OutMeshes);

	/** 액터의 InventoryComponent를 찾아 반환 */
	UFUNCTION(BlueprintCallable, Category = "PR|Utils")
	static UPRInventoryComponent* GetInventoryComponent(AActor* Actor);

	/** 액터의 WeaponManagerComponent 찾아 반환 */
	UFUNCTION(BlueprintCallable, Category = "PR|Utils")
	static UPRWeaponManagerComponent* GetWeaponManagerComponent(AActor* Actor);

	/** 액터의 QuickSlotComponent 찾아 반환 */
	UFUNCTION(BlueprintCallable, Category = "PR|Utils")
	static UPRQuickSlotComponent* GetQuickSlotComponent(AActor* Actor);
	
	/** 액터의 EquipmentManagerComponent 찾아 반환 */
	static UPREquipmentManagerComponent* GetEquipmentManagerComponent(AActor* Actor);
	
	/** Actor로 부터 ASC 획득 (Controller or Owner or Avatar 자동) */
	UFUNCTION(BlueprintCallable, Category = "PR|Utils")
	static UAbilitySystemComponent* GetAbilitySystemComponent(AActor* Actor);

	/** TargetActor의 ASC에서 지정 태그를 가진 활성 어빌리티 취소 */
	UFUNCTION(BlueprintCallable, Category = "PR|Ability")
	static void CancelAbilityWithTags(AActor* TargetActor, const FGameplayTagContainer& AbilityTags);

	// Ammo 부여
	UFUNCTION(BlueprintCallable, Category = "PR|Utils")
	static void GrantAmmo(AActor* TargetActor,TSubclassOf<UGameplayEffect> AmmoEffect,  float AmmoAmount);
	
	// GameInstance 획득
	UFUNCTION(BlueprintPure, Category = "PR|Utils")
	static UPRGameInstance* GetPRGameInstance(const UObject* WorldContext);
	
	/** 폰의 PlayerController 카메라 위치/회전 추출. 컨트롤러 부재 시 Pawn::GetActorEyesViewPoint 폴백.
	 * 로컬 컨트롤된 폰에서만 의미가 있음. 추출 성공 여부 반환  
	 */
	static bool GetPawnViewpoint(const APawn* Pawn, FVector& OutLocation, FRotator& OutRotation);

	/** 폰 카메라 뷰포인트에서 1차 라인 트레이스로 조준점 산출.
	 * 미충돌/월드 부재 시 카메라 끝점 반환. IgnoredActors는 트레이스에서 제외 
	 */
	static FVector ResolveCameraAimPoint(const APawn* Pawn, float TraceDistance, ECollisionChannel TraceChannel, const TArray<AActor*>& IgnoredActors);

	/* 머즐 위치 + 카메라 조준점 기반 발사 트랜스폼 산출.
	 * 위치 = MuzzleLocation, 회전 = (AimPoint - MuzzleLocation) Rotation. 거리 0이면 카메라 정면으로 폴백
	 */
	static FTransform ResolveProjectileLaunchTransform(const APawn* Pawn, const FVector& MuzzleLocation, float TraceDistance, ECollisionChannel TraceChannel, const TArray<AActor*>& IgnoredActors);

	// 로컬 월드에서만 FX Cue 실행
	UFUNCTION(BlueprintCallable, Category = "PR|FX", meta = (WorldContext = "WorldContextObject"))
	static void PlayLocalFX(const UObject* WorldContextObject, FGameplayTag FXTag, FInstancedStruct Payload);

	// 로컬 예측 FX Cue 실행 후 서버 확정 FX 중복 제거에 사용할 PredictionKey 반환
	UFUNCTION(BlueprintCallable, Category = "PR|FX", meta = (WorldContext = "WorldContextObject"))
	static FPRFXPredictionKey PlayPredictiveFX(const UObject* WorldContextObject, FGameplayTag FXTag, FInstancedStruct Payload);

	// 서버에서 확정된 FX Cue를 현재 클라이언트 월드에서 실행
	UFUNCTION(BlueprintCallable, Category = "PR|FX", meta = (WorldContext = "WorldContextObject"))
	static void PlayConfirmedFX(const UObject* WorldContextObject, FGameplayTag FXTag, FInstancedStruct Payload, FPRFXPredictionKey PredictionKey);

	// 로컬 예측 FX Cue 실행과 서버 전파 요청을 한 번에 처리
	UFUNCTION(BlueprintCallable, Category = "PR|FX", meta = (WorldContext = "WorldContextObject"))
	static FPRFXPredictionKey PlayPredictiveNetworkFX(const UObject* WorldContextObject, AActor* NetworkSourceActor, FGameplayTag FXTag, FInstancedStruct Payload);
};
