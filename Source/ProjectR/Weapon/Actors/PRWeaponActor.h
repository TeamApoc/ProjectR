// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "PRWeaponActor.generated.h"

class ACharacter;
class USceneComponent;
class USkeletalMeshComponent;

// 슬롯에 장착된 무기의 로컬 공개 표현만 담당하는 Actor다.
UCLASS()
class PROJECTR_API APRWeaponActor : public AActor
{
	GENERATED_BODY()

public:
	// 로컬 공개 표현 Actor를 초기화한다
	APRWeaponActor();

	/*~ AActor Interface ~*/
	// 초기 생성 이후 추가 초기화 지점을 제공한다
	virtual void BeginPlay() override;

public:
	// 슬롯 공개 상태를 기준으로 메시와 내부 표시 상태를 갱신한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	void InitializeFromVisualSlot(const FPRWeaponVisualSlot& InVisualSlot);

	// 현재 공개 비주얼 상태를 반환한다
	const FPRWeaponVisualSlot& GetVisualSlot() const { return VisualSlot; }

	// 전달받은 소켓 이름 기준으로 캐릭터 메시 소켓에 자신을 부착한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	void AttachToOwnerMesh(ACharacter* OwnerCharacter, FName SocketName);

protected:
	// 현재 공개 상태를 기준으로 메시를 갱신한다
	void RefreshVisualMesh();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	FPRWeaponVisualSlot VisualSlot;
};
