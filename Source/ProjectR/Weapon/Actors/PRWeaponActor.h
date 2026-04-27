// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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
	// 전달받은 소켓 이름 기준으로 캐릭터 메시 소켓에 자신을 부착한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	void AttachToOwnerMesh(ACharacter* OwnerCharacter, FName SocketName);

	// 26.04.26, Yuchan, 머즐 트랜스폼 반환 함수가 없어 아래 코드 임시 추가.
	// 총구 트랜스폼 반환 함수, BP에서 override
	UFUNCTION(BlueprintNativeEvent)
	FTransform GetMuzzleTransform() const;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComponent;
};
