// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "PRWeaponManagerComponent.generated.h"

class APRWeaponActor;
class UPRWeaponDataAsset;

// 캐릭터 기준 무기 장착 공개 상태와 슬롯별 로컬 Actor 생명주기를 관리하는 허브다.
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRWeaponManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 무기 장착 허브 컴포넌트를 초기화한다
	UPRWeaponManagerComponent();

	/*~ UActorComponent Interface ~*/
	// 종료 시점에 슬롯별 로컬 Actor를 정리한다
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 복제할 공개 상태를 등록한다
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 현재 활성 슬롯 공개 상태를 반환한다
	const FPRActiveWeaponSlot& GetActiveSlot() const { return ActiveSlot; }

	// 현재 무장 상태를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	EPRWeaponArmedState GetWeaponArmedState() const { return ArmedState; }

	// 대상 슬롯의 현재 무기 데이터를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	UPRWeaponDataAsset* GetWeaponDataBySlot(EPRWeaponSlotType SlotType) const;

	// 대상 슬롯의 현재 공개 비주얼 상태를 반환한다
	const FPRWeaponVisualSlot& GetVisualSlotBySlotType(EPRWeaponSlotType SlotType) const;

	// 테스트용 무기 데이터를 지정 슬롯에 장착한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	void EquipTestWeaponToSlot(UPRWeaponDataAsset* WeaponData, EPRWeaponSlotType TargetSlot);

	// 지정 슬롯의 무기를 해제한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	void UnequipWeaponSlot(EPRWeaponSlotType TargetSlot);

	// 활성 무기 슬롯을 지정 슬롯으로 전환한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	void SwapActiveSlot(EPRWeaponSlotType TargetSlot);

	// 현재 무장 상태를 변경한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	void SetWeaponArmedState(EPRWeaponArmedState NewArmedState);

	// 26.04.26, Yuchan, 무기 액터 반환 함수가 없어 임시로 추가. Fire 어빌리티가 의존하므로 아래 인터페이스는 유지되어야 함.
	UFUNCTION(BlueprintPure)
	APRWeaponActor* GetActiveWeaponActor() const;
	
protected:
	// 서버 권위에서 슬롯 데이터와 공개 상태를 갱신한다
	void EquipTestWeaponToSlotInternal(UPRWeaponDataAsset* WeaponData, EPRWeaponSlotType TargetSlot);

	// 서버 권위에서 슬롯 데이터 제거와 공개 상태를 갱신한다
	void UnequipWeaponSlotInternal(EPRWeaponSlotType TargetSlot);

	// 서버 권위에서 활성 슬롯과 공개 상태를 갱신한다
	void SwapActiveSlotInternal(EPRWeaponSlotType TargetSlot);

	// 서버 권위에서 무장 상태와 공개 상태를 갱신한다
	void SetWeaponArmedStateInternal(EPRWeaponArmedState NewArmedState);

	// 대상 슬롯의 로컬 무기 Actor를 생성 또는 갱신한다
	void RefreshWeaponActorForSlot(EPRWeaponSlotType SlotType);

	// 두 슬롯의 로컬 무기 Actor를 현재 공개 상태 기준으로 갱신한다
	void RefreshAllWeaponActors();

	// 대상 슬롯의 현재 휴대 상태에 맞춰 부착 소켓을 최신화한다
	void RefreshWeaponAttachmentForSlot(EPRWeaponSlotType SlotType);

	// 새로 복제된 활성 슬롯 상태를 기준으로 로컬 부착 상태를 갱신한다
	UFUNCTION()
	void OnRep_ActiveSlot(FPRActiveWeaponSlot OldActiveSlot);

	// 새로 복제된 주무기 공개 비주얼 상태를 기준으로 로컬 Actor를 갱신한다
	UFUNCTION()
	void OnRep_PrimaryVisualSlot(FPRWeaponVisualSlot OldVisualSlot);

	// 새로 복제된 보조무기 공개 비주얼 상태를 기준으로 로컬 Actor를 갱신한다
	UFUNCTION()
	void OnRep_SecondaryVisualSlot(FPRWeaponVisualSlot OldVisualSlot);

	// 새로 복제된 무장 상태를 기준으로 로컬 부착 상태를 갱신한다
	UFUNCTION()
	void OnRep_ArmedState(EPRWeaponArmedState OldArmedState);

	// 슬롯 데이터에서 활성 공개 상태를 구성한다
	FPRActiveWeaponSlot BuildActiveSlot(EPRWeaponSlotType SlotType, UPRWeaponDataAsset* WeaponData) const;

	// 클라이언트 장착 요청을 서버 권위 경로로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_EquipTestWeaponToSlot(UPRWeaponDataAsset* WeaponData, EPRWeaponSlotType TargetSlot);

	// 클라이언트 장비 해제 요청을 서버 권위 경로로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_UnequipWeaponSlot(EPRWeaponSlotType TargetSlot);

	// 클라이언트 슬롯 전환 요청을 서버 권위 경로로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_SwapActiveSlot(EPRWeaponSlotType TargetSlot);

	// 클라이언트 무장 상태 변경 요청을 서버 권위 경로로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_SetWeaponArmedState(EPRWeaponArmedState NewArmedState);

protected:
	// 슬롯 데이터에서 공개 비주얼 상태를 구성한다
	FPRWeaponVisualSlot BuildVisualSlot(EPRWeaponSlotType SlotType, UPRWeaponDataAsset* WeaponData) const;

	// 현재 슬롯이 Armed 상태인지 Stowed 상태인지 계산한다
	EPRWeaponCarryState ResolveCarryState(EPRWeaponSlotType SlotType) const;

	// 슬롯과 휴대 상태에 맞는 부착 소켓 이름을 반환한다
	FName ResolveAttachSocketName(EPRWeaponSlotType SlotType, EPRWeaponCarryState CarryState) const;

	// 현재 원본 슬롯 데이터 기준으로 공개 비주얼 상태를 다시 구성한다
	void RefreshVisualSlotsFromCurrentState();

	// 현재 장착 상태를 기준으로 다음 활성 슬롯을 결정한다
	FPRActiveWeaponSlot ResolveNextActiveSlotAfterUnequip(EPRWeaponSlotType UnequippedSlot) const;

	// 1차 단계에서 활성 무기 어빌리티 부여 시점을 로그로 남긴다
	void GrantWeaponAbilitiesSkeleton(const FPRActiveWeaponSlot& WeaponSlot) const;

	// 1차 단계에서 활성 무기 어빌리티 해제 시점을 로그로 남긴다
	void ClearWeaponAbilitiesSkeleton(const FPRActiveWeaponSlot& WeaponSlot) const;

	// 잘못된 슬롯 입력을 차단한다
	bool IsSupportedSlot(EPRWeaponSlotType SlotType) const;

private:
	// 대상 슬롯 공개 비주얼 상태를 수정 가능한 참조로 반환한다
	FPRWeaponVisualSlot& GetMutableVisualSlotBySlotType(EPRWeaponSlotType SlotType);

	// 대상 슬롯의 현재 로컬 Actor를 반환한다
	APRWeaponActor* GetWeaponActorBySlot(EPRWeaponSlotType SlotType) const; // TODO: 왜 private?

	// 대상 슬롯 로컬 Actor를 수정 가능한 참조로 반환한다
	TObjectPtr<APRWeaponActor>& GetMutableWeaponActorBySlot(EPRWeaponSlotType SlotType); // TODO: 역할이 모호함. GetWeaponActorBySlot으로 충분하지 않은지?

	// 대상 슬롯의 현재 로컬 Actor를 안전하게 정리한다
	void DestroyWeaponActorForSlot(EPRWeaponSlotType SlotType);

protected:
	// 현재 무장 상태
	UPROPERTY(ReplicatedUsing = OnRep_ArmedState, VisibleInstanceOnly, Category = "ProjectR|Weapon")
	EPRWeaponArmedState ArmedState = EPRWeaponArmedState::Armed; // TODO: 기본값이 Armed?

	// 현재 활성 무기 공개 상태
	UPROPERTY(ReplicatedUsing = OnRep_ActiveSlot, VisibleInstanceOnly, Category = "ProjectR|Weapon")
	FPRActiveWeaponSlot ActiveSlot; // TODO: FPRActiveWeaponSlot과 FPRWeaponVisualSlot은 하나로 통합가능하지 않은지?

	// 주무기 슬롯 공개 비주얼 상태
	UPROPERTY(ReplicatedUsing = OnRep_PrimaryVisualSlot, VisibleInstanceOnly, Category = "ProjectR|Weapon")
	FPRWeaponVisualSlot PrimaryVisualSlot;

	// 보조무기 슬롯 공개 비주얼 상태
	UPROPERTY(ReplicatedUsing = OnRep_SecondaryVisualSlot, VisibleInstanceOnly, Category = "ProjectR|Weapon")
	FPRWeaponVisualSlot SecondaryVisualSlot;

	// 주무기 슬롯에 연결된 최소 무기 데이터
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TObjectPtr<UPRWeaponDataAsset> PrimaryWeaponData;

	// 보조무기 슬롯에 연결된 최소 무기 데이터
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TObjectPtr<UPRWeaponDataAsset> SecondaryWeaponData;

private:
	// 현재 머신에서만 유지하는 주무기 슬롯 Actor
	UPROPERTY(Transient)
	TObjectPtr<APRWeaponActor> PrimaryWeaponActor;

	// 현재 머신에서만 유지하는 보조무기 슬롯 Actor
	UPROPERTY(Transient)
	TObjectPtr<APRWeaponActor> SecondaryWeaponActor;
};
