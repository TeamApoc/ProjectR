// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectR/Weapon/Types/PRWeaponAnimationTypes.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "PRWeaponActor.generated.h"

enum class EPRArmedState : uint8;
class UNiagaraSystem;
class ACharacter;
class USceneComponent;
class USkeletalMeshComponent;
class UPRWeaponAnimInstance;

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

	// 무기 스켈레탈 메시 컴포넌트를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	USkeletalMeshComponent* GetWeaponMeshComponent() const { return WeaponMeshComponent; }

	// 무기 메시에서 ProjectR 무기 AnimInstance를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon|Animation")
	UPRWeaponAnimInstance* GetWeaponAnimInstance() const;

	// 무기 AnimInstance에 지정 애니메이션 상태 요청을 전달한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon|Animation")
	bool RequestWeaponAnimation(EPRWeaponAnimationState AnimationState);

	// 무기 AnimInstance에 발사 애니메이션 요청을 전달한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon|Animation")
	bool RequestShootAnimation();

	// 무기 AnimInstance에 재장전 애니메이션 요청을 전달한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon|Animation")
	bool RequestReloadAnimation();

	// 무기 AnimInstance를 Idle 상태로 되돌린다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon|Animation")
	bool SetWeaponAnimationIdle();

	// 26.04.26, Yuchan, 머즐 트랜스폼 반환 함수가 없어 아래 코드 임시 추가.
	// 총구 트랜스폼 반환 함수, BP에서 override
	UFUNCTION(BlueprintNativeEvent)
	FTransform GetMuzzleTransform() const;
	
	// 26.05.04, Yuchan, 총구 이펙트 재생 추가
	// 총기 관련 특정 이펙트 재생, BP에서 선택적 override 가능
	UFUNCTION(BlueprintNativeEvent)
	void PlayNiagaraEffect(EPRWeaponEffectType EffectType, UNiagaraSystem* InNiagaraSystem = nullptr);
	
	// 총기 관련 이펙트 중단, BP에서 선택적 구현
	UFUNCTION(BlueprintImplementableEvent)
	void StopEffect(EPRWeaponEffectType EffectType);
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComponent;
};
