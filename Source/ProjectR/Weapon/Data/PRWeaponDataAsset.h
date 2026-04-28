// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectR/Weapon/Types/PRRecoilTypes.h"
#include "PRWeaponDataAsset.generated.h"

class APRWeaponActor;
class USkeletalMesh;
class UAnimMontage;

// 1차 무기 장착 공개 상태 검증에 사용하는 최소 무기 데이터다.
UCLASS(BlueprintType)
class PROJECTR_API UPRWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPRWeaponDataAsset();

public:
	// 디버그와 UI에서 구분할 무기 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	FName WeaponId;

	// 슬롯별 공개 무기 표현에 사용할 Actor 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TSubclassOf<APRWeaponActor> WeaponActorClass;

	// 테스트용 로컬 무기 메시
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TObjectPtr<USkeletalMesh> DisplayMesh;
	
	// 04.27 김동석 추가
	// 이 무기를 들었을 때 캐릭터에 적용할 애니메이션 레이어
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TSubclassOf<UAnimInstance> WeaponAnimLayerClass;

	// 이 무기의 카메라 반동과 크로스헤어 확산 데이터
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil")
	FPRRecoilProfile RecoilProfile;
	
	// 발사 시 재생할 캐릭터 몽타주                                                   
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	TObjectPtr<UAnimMontage> ShootMontage;                                            
                                                                                  
	// 재장전 시 재생할 캐릭터 몽타주                                                 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;                
	
	// 발사 몽타주 재생 속도                                                           
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Animation") 
	float ShootMontagePlayRate = 1.0f;                                                 
                                                                                   
	// 재장전 몽타주 재생 속도                                                         
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Animation") 
	float ReloadMontagePlayRate = 1.0f;                                                
};
