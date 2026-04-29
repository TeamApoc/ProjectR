// Copyright ProjectR. All Rights Reserved.

#include "PRWeaponManagerComponent.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Player.h"
#include "ProjectR/Inventory/Components/PRInventoryComponent.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/Weapon/Actors/PRWeaponActor.h"
#include "ProjectR/Weapon/Data/PRWeaponDataAsset.h"
#include "ProjectR/Weapon/Data/PRWeaponModDataAsset.h"
#include "ProjectR/Weapon/Items/PRItemInstance_Weapon.h"

namespace
{
	// 무기와 Mod의 태그 호환 여부를 판정한다
	bool IsModCompatible(const UPRWeaponDataAsset* WeaponData, const UPRWeaponModDataAsset* ModData)
	{
		// 무기 데이터나 Mod 데이터가 없는 경우
		if (!IsValid(WeaponData) || !IsValid(ModData))
		{
			// 호환성 판정 불가
			return false;
		}

		// Mod가 호환 판정에 사용할 태그를 갖지 않은 경우
		if (ModData->ModTags.IsEmpty())
		{
			// 호환 가능한 대상 없음
			return false;
		}

		// 무기가 지원 가능한 Mod 태그를 갖지 않은 경우
		if (WeaponData->SupportedModTags.IsEmpty())
		{
			// 허용 가능한 Mod 범위 없음
			return false;
		}

		// 무기 지원 태그와 Mod 태그가 하나라도 겹치면 호환으로 판정
		return WeaponData->SupportedModTags.HasAny(ModData->ModTags);
	}

	FString GetNetModeLogName(const UObject* WorldContextObject)
	{
		// 월드 컨텍스트를 얻을 수 없는 경우
		if (!IsValid(WorldContextObject) || !IsValid(WorldContextObject->GetWorld()))
		{
			// 로그에서 월드 없음 상태로 표기
			return TEXT("NoWorld");
		}

		// 로그 가독성을 위해 엔진 NetMode를 문자열로 변환
		switch (WorldContextObject->GetWorld()->GetNetMode())
		{
		case NM_Standalone:
			return TEXT("Standalone");
		case NM_DedicatedServer:
			return TEXT("DedicatedServer");
		case NM_ListenServer:
			return TEXT("ListenServer");
		case NM_Client:
			return TEXT("Client");
		default:
			return TEXT("Unknown");
		}
	}
}

UPRWeaponManagerComponent::UPRWeaponManagerComponent()
{
	// 무기 관리는 명시 요청과 동기화으로 갱신하므로 Tick 비활성화
	PrimaryComponentTick.bCanEverTick = false;

	// 슬롯 공개 상태와 무장 상태를 네트워크로 전달하기 위해 기본 복제 활성화
	SetIsReplicatedByDefault(true);
}

void UPRWeaponManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 컴포넌트 종료 시 남아 있는 주무기 Actor 정리
	DestroyWeaponActorForSlot(EPRWeaponSlotType::Primary);

	// 컴포넌트 종료 시 남아 있는 보조무기 Actor 정리
	DestroyWeaponActorForSlot(EPRWeaponSlotType::Secondary);

	Super::EndPlay(EndPlayReason);
}

void UPRWeaponManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 무장 상태는 클라이언트 부착 소켓과 공개 비주얼 상태를 결정하므로 복제
	DOREPLIFETIME(UPRWeaponManagerComponent, ArmedState);

	// 활성 무기 슬롯은 클라이언트가 손에 든 무기와 수납 무기를 구분하도록 복제
	DOREPLIFETIME(UPRWeaponManagerComponent, CurrentWeaponSlot);

	// 주무기 공개 비주얼 정보는 owner-only 원본 포인터 없이 클라이언트 표시를 갱신하기 위해 복제
	DOREPLIFETIME(UPRWeaponManagerComponent, PrimaryVisualInfo);

	// 보조무기 공개 비주얼 정보는 owner-only 원본 포인터 없이 클라이언트 표시를 갱신하기 위해 복제
	DOREPLIFETIME(UPRWeaponManagerComponent, SecondaryVisualInfo);
}

void UPRWeaponManagerComponent::InitializeRuntimeLinks()
{
	// PlayerState 재연결 시 이전 캐시가 남지 않도록 먼저 초기화
	CachedASC = nullptr;
	CachedPlayerSet = nullptr;
	CachedInventory = nullptr;

	// 무기 매니저는 캐릭터 소유를 전제로 PlayerState 런타임 링크를 조회
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	// 캐릭터 소유자가 아니면
	if (!IsValid(OwnerCharacter))
	{
		// 함수 조기 종료. 런타임 링크 구성 불가
		return;
	}

	// ASC, AttributeSet, Inventory 조회용
	const APRPlayerState* PlayerState = OwnerCharacter->GetPlayerState<APRPlayerState>();
	if (!IsValid(PlayerState))
	{
		// 함수 조기 종료. PlayerState가 아직 준비되지 않은 시점이므로 캐시를 비운 상태로 유지
		return;
	}

	// 서버 권위 장착 처리에서 사용할 런타임 의존성 캐시
	CachedASC = PlayerState->GetPRAbilitySystemComponent();
	CachedPlayerSet = PlayerState->GetPlayerSet();
	CachedInventory = PlayerState->GetInventoryComponent();
}

UPRItemInstance_Weapon* UPRWeaponManagerComponent::GetWeaponInstanceBySlotType(EPRWeaponSlotType SlotType) const
{
	// 주무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return PrimaryWeaponInstance;
	}

	// 보조무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return SecondaryWeaponInstance;
	}

	// 지원하지 않는 슬롯은 원본 Item 없음으로 nullptr 리턴
	return nullptr;
}

UPRWeaponDataAsset* UPRWeaponManagerComponent::GetWeaponDataBySlotType(EPRWeaponSlotType SlotType) const
{
	// 서버나 소유자 경로에서 원본 Item을 조회할 수 있는 경우
	if (UPRItemInstance_Weapon* WeaponInstance = GetWeaponInstanceBySlotType(SlotType))
	{
		// 원본 Item의 최신 무기 데이터를 리턴
		return WeaponInstance->GetWeaponData();
	}

	// 원본 Item이 없는 클라이언트는 공개 비주얼 정보를 기준으로 무기 데이터 리턴
	const FPRWeaponVisualInfo& VisualInfo = GetVisualInfoBySlotType(SlotType);
	return VisualInfo.WeaponData;
}

const FPRWeaponVisualInfo& UPRWeaponManagerComponent::GetCurrentWeaponVisualInfo() const
{
	// 현재 활성 슬롯에 대응하는 공개 비주얼 정보 리턴
	return GetVisualInfoBySlotType(CurrentWeaponSlot);
}

const FPRWeaponVisualInfo& UPRWeaponManagerComponent::GetVisualInfoBySlotType(EPRWeaponSlotType SlotType) const
{
	// 주무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return PrimaryVisualInfo;
	}

	// 보조무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return SecondaryVisualInfo;
	}

	// 지원하지 않는 슬롯은 변경되지 않는 빈 비주얼 정보 리턴
	static FPRWeaponVisualInfo EmptyVisualInfo;
	return EmptyVisualInfo;
}

bool UPRWeaponManagerComponent::EquipInventoryWeaponAtIndex(int32 InventoryIndex)
{
	// 클라이언트가 복제받은 인벤토리 항목으로 장착할 수 있도록 인덱스를 Item 식별자로 변환한다
	// 인벤토리 캐시가 비어 있을 수 있으므로 장착 요청마다 런타임 링크 갱신
	InitializeRuntimeLinks();

	// 인벤토리 컴포넌트를 찾지 못한 경우
	if (!IsValid(CachedInventory))
	{
		// 장착 실패. 장착 대상 조회 불가
		return false;
	}

	// 인덱스 기반 요청을 서버 검증 가능한 Item 식별자 요청으로 변환하기 위한 Item 조회
	UPRItemInstance_Weapon* WeaponItem = CachedInventory->GetWeaponItemAtIndex(InventoryIndex);

	// 인덱스에 대응하는 무기 Item이 없는 경우
	if (!IsValid(WeaponItem))
	{
		// 장착 실패 상황을 Owner, Index 기준으로 추적
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[WeaponManagerComponent][%s] 무기 장착 실패. EquipInventoryWeaponAtIndex() | Owner = %s | Index = %d"),
			*GetNetModeLogName(this),
			*GetNameSafe(GetOwner()),
			InventoryIndex);

		// 장착 실패. Item 조회 실패
		return false;
	}

	// 조회된 ItemId로 포인터 신뢰 없이 장착 요청을 이어감
	return EquipWeaponItemById(WeaponItem->GetItemId());
}

bool UPRWeaponManagerComponent::EquipInventoryWeaponAtIndex(int32 InventoryIndex)
{
	// 클라이언트가 복제받은 인벤토리 항목으로 장착할 수 있도록 인덱스를 Item 식별자로 변환한다
	// 인벤토리 캐시가 비어 있을 수 있으므로 장착 요청마다 런타임 링크 갱신
	InitializeRuntimeLinks();

	// 인벤토리 컴포넌트를 찾지 못한 경우
	if (!IsValid(CachedInventory))
	{
		// 장착 실패. 장착 대상 조회 불가
		return false;
	}

	// 인덱스 기반 요청을 서버 검증 가능한 Item 식별자 요청으로 변환하기 위한 Item 조회
	UPRItemInstance_Weapon* WeaponItem = CachedInventory->GetWeaponItemAtIndex(InventoryIndex);

	// 인덱스에 대응하는 무기 Item이 없는 경우
	if (!IsValid(WeaponItem))
	{
		// 장착 실패 상황을 Owner, Index 기준으로 추적
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[WeaponManagerComponent][%s] 무기 장착 실패. EquipInventoryWeaponAtIndex() | Owner = %s | Index = %d"),
			*GetNetModeLogName(this),
			*GetNameSafe(GetOwner()),
			InventoryIndex);

		// 장착 실패. Item 조회 실패
		return false;
	}

	// 조회된 ItemId로 포인터 신뢰 없이 장착 요청을 이어감
	return EquipWeaponItemById(WeaponItem->GetItemId());
}

bool UPRWeaponManagerComponent::EquipWeaponItemById(const FGuid& ItemId)
{
	// Item 식별자 기반 요청은 클라이언트가 원본 포인터를 신뢰하지 않도록 서버에서 다시 조회한다
	// 유효하지 않은 ItemId인 경우
	if (!ItemId.IsValid())
	{
		// 장착 실패. 요청 검증 실패
		return false;
	}

	// 서버 권위가 있으면 RPC를 거치지 않고 내부 장착 경로로 즉시 처리
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		// 내부 처리에서 인벤토리를 다시 조회할 수 있도록 런타임 링크 갱신
		InitializeRuntimeLinks();

		// 서버 권위 장착 결과 리턴
		return EquipWeaponItemByIdInternal(ItemId);
	}

	// 클라이언트 요청은 서버에서 ItemId를 다시 검증하도록 RPC 전송
	Server_EquipWeaponItemById(ItemId);

	// 요청 접수 성공. 서버 요청 전송 마침
	return true;
}

bool UPRWeaponManagerComponent::EquipWeapon(UPRItemInstance_Weapon* WeaponItem)
{
	// 무기 Item이 없는 경우
	if (!IsValid(WeaponItem))
	{
		// 장착 실패. 요청 검증 실패
		return false;
	}

	// 원본 포인터 기반 장착은 서버 권위에서만 직접 처리
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		// 내부 처리에서 인벤토리 소유 여부를 검증할 수 있도록 런타임 링크 갱신
		InitializeRuntimeLinks();

		// 서버 권위 장착 결과 리턴
		return EquipWeaponInternal(WeaponItem);
	}

	// 장착 실패. 클라이언트의 원본 포인터 기반 장착 요청은 신뢰하지 않음
	return false;
}

bool UPRWeaponManagerComponent::UnequipWeaponFromSlot(EPRWeaponSlotType TargetSlot)
{
	// 지원하지 않는 슬롯 요청이면 서버 전송 전에 차단
	if (!IsSupportedSlot(TargetSlot))
	{
		// 해제 실패. 요청 검증 실패
		return false;
	}

	// 서버 권위가 있으면 RPC를 거치지 않고 내부 해제 경로로 즉시 처리
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		// 서버 권위 해제 결과 리턴
		return UnequipWeaponFromSlotInternal(TargetSlot);
	}

	// 클라이언트 요청은 서버 권위에서 슬롯 해제를 수행하도록 RPC 전송
	Server_UnequipWeaponSlot(TargetSlot);

	// 요청 접수 성공. 서버 요청 전송 마침
	return true;
}

bool UPRWeaponManagerComponent::AttachModToSlot(EPRWeaponSlotType TargetSlot, UPRWeaponModDataAsset* NewModData)
{
	// 지원하지 않는 슬롯 요청인 경우
	if (!IsSupportedSlot(TargetSlot))
	{
		// Mod 장착 실패. 요청 검증 실패
		return false;
	}

	// 서버 권위가 있으면 RPC를 거치지 않고 내부 Mod 장착 경로로 즉시 처리
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		// 내부 처리에서 플레이어 자원을 갱신할 수 있도록 런타임 링크 갱신
		InitializeRuntimeLinks();

		// 서버 권위 Mod 장착 결과 리턴
		return AttachModToSlotInternal(TargetSlot, NewModData);
	}

	// 클라이언트 요청은 서버 권위에서 Mod 호환성을 검증하도록 RPC 전송
	Server_AttachModToSlot(TargetSlot, NewModData);

	// 요청 접수 성공. 서버 요청 전송 마침
	return true;
}

void UPRWeaponManagerComponent::SetCurrentWeaponSlot(EPRWeaponSlotType TargetSlot)
{
	// 지원하지 않는 슬롯 요청이면 서버 전송 전에 차단
	if (!IsSupportedSlot(TargetSlot))
	{
		// 함수 조기 종료. 요청 검증 실패
		return;
	}

	// 서버 권위가 있으면 RPC를 거치지 않고 활성 슬롯을 즉시 전환
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		SetCurrentWeaponSlotInternal(TargetSlot);

		// 함수 조기 종료. 서버 전환 처리 마침
		return;
	}

	// 클라이언트 요청은 서버 권위에서 활성 슬롯을 전환하도록 RPC 전송
	Server_SetCurrentWeaponSlot(TargetSlot);
}

void UPRWeaponManagerComponent::SetWeaponArmedState(EPRArmedState NewArmedState)
{
	// 서버 권위가 있으면 RPC를 거치지 않고 무장 상태를 즉시 변경
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		SetWeaponArmedStateInternal(NewArmedState);
		
		// 함수 조기 종료. 서버 무장 상태 변경 마침
		return;
	}

	// 클라이언트 요청은 서버 권위에서 무장 상태를 변경하도록 RPC 전송
	Server_SetWeaponArmedState(NewArmedState);
}

APRWeaponActor* UPRWeaponManagerComponent::GetActiveWeaponActor() const
{
	// 현재 활성 슬롯에 대응하는 로컬 무기 Actor 리턴
	return GetWeaponActorBySlot(CurrentWeaponSlot);
}

bool UPRWeaponManagerComponent::EquipWeaponItemByIdInternal(const FGuid& ItemId)
{
	// 서버 내부 장착에 필요한 ItemId와 인벤토리 캐시가 유효하지 않은 경우
	if (!ItemId.IsValid() || !IsValid(CachedInventory))
	{
		// 장착 실패. 내부 검증 실패
		return false;
	}

	// 서버 인벤토리에서 ItemId에 대응하는 실제 무기 Item 조회
	UPRItemInstance_Weapon* WeaponItem = CachedInventory->FindWeaponByItemId(ItemId);

	// 서버 인벤토리에서 무기 Item을 찾지 못한 경우
	if (!IsValid(WeaponItem))
	{
		// ItemId 기반 장착 실패 상황을 Owner, ItemId 기준으로 추적
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[WeaponManagerComponent][Server] 무기 장착 실패. EquipWeaponItemByIdInternal | Owner = %s | ItemId = %s"),
			*GetNameSafe(GetOwner()),
			*ItemId.ToString());
		
		// 장착 실패. Item 조회 실패
		return false;
	}

	// // ItemId 검증이 끝난 뒤 실제 장착 처리로 넘어가는 입력 상태를 추적
	// UE_LOG(
	// 	LogTemp,
	// 	Log,
	// 	TEXT("[WeaponManagerComponent][Server] ItemId 장착 처리 시작 | Owner = %s | ItemId = %s | Weapon = %s"),
	// 	*GetNameSafe(GetOwner()),
	// 	*ItemId.ToString(),
	// 	*GetNameSafe(WeaponItem->GetWeaponData()));

	// 검증된 무기 Item으로 슬롯 장착 처리 결과 리턴
	return EquipWeaponInternal(WeaponItem);
}

bool UPRWeaponManagerComponent::EquipWeaponInternal(UPRItemInstance_Weapon* WeaponItem)
{
	const UPRWeaponDataAsset* WeaponData = IsValid(WeaponItem) ? WeaponItem->GetWeaponData() : nullptr;
	const EPRWeaponSlotType WeaponSlot = IsValid(WeaponData) ? WeaponData->SlotType : EPRWeaponSlotType::None;

	// 잘못된 슬롯, 비소유 Item, 무기 데이터 누락인 경우
	if (!IsSupportedSlot(WeaponSlot)
		|| !IsValid(WeaponItem)
		|| !IsValid(CachedInventory)
		|| !CachedInventory->OwnsWeapon(WeaponItem)
		|| !IsValid(WeaponData))
	{
		// 장착 실패. 장착 전 검증 실패
		return false;
	}

	// 현재 장착 슬롯에 연결된 원본 Item. 중복 장착 차단과 교체 해제 알림 대상 판단에 사용
	UPRItemInstance_Weapon* CurrentWeaponInstance = GetWeaponInstanceBySlotType(WeaponSlot);

	// 같은 Item이 이미 장착 슬롯에 연결된 경우
	if (CurrentWeaponInstance == WeaponItem)
	{
		// 장착 실패. 중복 장착 요청
		return false;
	}

	// 장착 슬롯이 비어 있었는지. 슬롯 자원 최초 초기화 여부를 결정
	const bool bWeaponSlotWasEmpty = !IsValid(CurrentWeaponInstance);

	// 장착 슬롯이 활성 슬롯이었는지. 기존 무기 해제 알림 필요 여부를 결정
	const bool bWasWeaponSlotCurrent = CurrentWeaponSlot == WeaponSlot;

	// 장착 슬롯이 장착 전 활성 슬롯이었던 경우
	if (bWasWeaponSlotCurrent)
	{
		// 교체 대상인 기존 무기 Item이 있는 경우
		if (IsValid(CurrentWeaponInstance))
		{
			// 기존 활성 무기 해제 알림 전달
			CurrentWeaponInstance->OnUnequipped(GetOwner());
		}
	}

	// 장착 검증이 끝난 뒤 장착 슬롯 원본을 새 무기로 확정
	GetMutableWeaponInstanceBySlot(WeaponSlot) = WeaponItem;

	// 장착 슬롯이 비어 있었고 플레이어 슬롯 자원이 연결된 경우
	if (bWeaponSlotWasEmpty && IsValid(CachedPlayerSet))
	{
		// 빈 슬롯에 최초 장착되는 경우에만 슬롯별 탄약과 Mod 자원 초기화
		CachedPlayerSet->InitializeSlotResources(WeaponSlot, WeaponData, WeaponItem->GetModData());
	}

	// 현재 활성 슬롯이 비어 있거나 이미 활성 중인 슬롯에 다시 장착하는 경우
	if (CurrentWeaponSlot == EPRWeaponSlotType::None || CurrentWeaponSlot == WeaponSlot)
	{
		// 활성 무기 사용 흐름으로 진입
		// 장착 슬롯을 활성 슬롯으로 지정
		CurrentWeaponSlot = WeaponSlot;

		// 무기의 어빌리티셋을 플레이어에게 부여
		WeaponItem->OnEquipped(GetOwner());
	}

	// 원본 슬롯 데이터를 기준으로 두 슬롯의 공개 비주얼 정보를 다시 구성
	RefreshVisualInfosFromCurrentState();

	// 서버 로컬도 복제 콜백과 같은 경로로 슬롯별 Actor를 즉시 최신화
	RefreshAllWeaponActors();

	// 애님 레이어 교체
	RefreshAnimLayer();

	// 서버 장착 처리가 확정된 뒤 Owner, 슬롯, ItemId, 무기 데이터, 활성 슬롯을 남겨 장착 흐름 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][Server] 무기 장착 완료. EquipWeaponInternal | Owner = %s | Slot = %s | ItemId = %s | Weapon = %s | CurrentWeaponSlot = %s"),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(WeaponSlot),
		*WeaponItem->GetItemId().ToString(),
		*GetNameSafe(WeaponData),
		*UEnum::GetValueAsString(CurrentWeaponSlot));
	
	// 장착 성공. 원본 슬롯, 활성 슬롯, 공개 비주얼, 로컬 Actor 갱신 마침
	return true;
}

bool UPRWeaponManagerComponent::UnequipWeaponFromSlotInternal(EPRWeaponSlotType TargetSlot)
{
	// 해제 타겟 슬롯의 원본 Item. 해제 알림과 로그 기록에 사용
	UPRItemInstance_Weapon* CurrentWeaponInstance = GetWeaponInstanceBySlotType(TargetSlot);

	// 지원하지 않는 슬롯이거나 비어 있는 슬롯인 경우
	if (!IsSupportedSlot(TargetSlot) || !IsValid(CurrentWeaponInstance))
	{
		// 해제 실패. 검증 실패
		return false;
	}

	// 해제 전 활성 슬롯. 해제 이후 다음 활성 슬롯 선정 기준으로 사용
	const EPRWeaponSlotType PreWeaponSlot = CurrentWeaponSlot;

	// 해제 타겟 슬롯이 활성 슬롯이었는지. 활성 무기 해제 알림 필요 여부를 결정
	const bool bWasTargetSlotCurrent = PreWeaponSlot == TargetSlot;

	// 해제 대상이 활성 슬롯이었던 경우
	if (bWasTargetSlotCurrent)
	{
		// 현재 활성 무기 해제 알림 전달
		CurrentWeaponInstance->OnUnequipped(GetOwner());
	}

	// 슬롯 해제를 서버 원본 상태에 먼저 반영
	GetMutableWeaponInstanceBySlot(TargetSlot) = nullptr;

	// 활성 슬롯을 해제한 경우
	if (bWasTargetSlotCurrent)
	{
		// 남은 슬롯 중 다음 활성 슬롯 선정
		CurrentWeaponSlot = ResolveNextWeaponSlotAfterUnequip(TargetSlot);

		// 다음 활성 슬롯이 존재하는 경우
		if (CurrentWeaponSlot != EPRWeaponSlotType::None)
		{
			// 다음 활성 슬롯의 원본 Item을 찾을 수 있는 경우
			if (UPRItemInstance_Weapon* NextWeaponInstance = GetWeaponInstanceBySlotType(CurrentWeaponSlot))
			{
				// 다음 활성 무기 장착 알림 전달
				NextWeaponInstance->OnEquipped(GetOwner());
			}
		}
		else
		{
			// 활성 슬롯 후보가 없으므로 비무장 상태로 변경
			ArmedState = EPRArmedState::Unarmed;
		}
	}

	// 해제 결과를 공개 비주얼 정보에 반영
	RefreshVisualInfosFromCurrentState();

	// 서버 로컬 Actor도 해제 결과에 맞춰 즉시 최신화
	RefreshAllWeaponActors();

	// 서버 해제 처리가 확정된 뒤 Owner, 해제 슬롯, ItemId, 무기 데이터, 다음 활성 슬롯을 남겨 해제 흐름 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][Server] 무기 해제 완료. UnequipWeaponFromSlotInternal | Owner = %s | Slot = %s | ItemId = %s | Weapon = %s | NextWeaponSlot = %s"),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(TargetSlot),
		*CurrentWeaponInstance->GetItemId().ToString(),
		*GetNameSafe(CurrentWeaponInstance->GetWeaponData()),
		*UEnum::GetValueAsString(CurrentWeaponSlot));
	
	// 해제 성공. 원본 슬롯, 활성 슬롯, 공개 비주얼, 로컬 Actor 갱신 마침
	return true;
}

bool UPRWeaponManagerComponent::AttachModToSlotInternal(EPRWeaponSlotType TargetSlot, UPRWeaponModDataAsset* NewModData)
{
	// Mod 장착 타겟 슬롯의 원본 Item. 호환성 검증과 자원 재초기화 기준으로 사용
	UPRItemInstance_Weapon* TargetWeaponInstance = GetWeaponInstanceBySlotType(TargetSlot);

	// 지원하지 않는 슬롯이거나 무기 원본 또는 무기 데이터가 없는 경우
	if (!IsSupportedSlot(TargetSlot) || !IsValid(TargetWeaponInstance) || !IsValid(TargetWeaponInstance->GetWeaponData()))
	{
		// Mod 장착 실패. 검증 실패
		return false;
	}

	// 이미 같은 Mod가 장착된 경우
	if (TargetWeaponInstance->GetModData() == NewModData)
	{
		// 중복 장착 실패 사유 기록 // TODO: 이건주, 실패 사유 기록보다 로그 추가 필요
		TargetWeaponInstance->LastModFailReason = EPRWeaponModFailReason::AlreadyActive;

		// Mod 장착 실패
		return false;
	}

	// 새 Mod가 있고 무기와 태그 호환되지 않는 경우
	if (IsValid(NewModData) && !IsModCompatible(TargetWeaponInstance->GetWeaponData(), NewModData))
	{
		// 상태 차단 실패 사유 기록
		TargetWeaponInstance->LastModFailReason = EPRWeaponModFailReason::BlockedByState;

		// Mod 장착 실패
		return false;
	}

	// 플레이어 슬롯 자원이 연결된 경우
	if (IsValid(CachedPlayerSet))
	{
		// Mod 교체 전 탄약 상태. 자원 재초기화 뒤 탄약량 보존에 사용
		const FPRWeaponSlotResourceState PreResourceState = CachedPlayerSet->BuildSlotResourceState(TargetSlot);

		// 새 Mod 기준으로 슬롯 자원 구조 재초기화
		CachedPlayerSet->InitializeSlotResources(TargetSlot, TargetWeaponInstance->GetWeaponData(), NewModData);

		// 재초기화로 변경된 탄약량을 이전 상태에 맞추기 위한 보정값
		FPRWeaponSlotResourceDelta PreserveAmmoDelta;
		PreserveAmmoDelta.MagazineDelta = PreResourceState.MagazineAmmo - CachedPlayerSet->BuildSlotResourceState(TargetSlot).MagazineAmmo;
		PreserveAmmoDelta.ReserveDelta = PreResourceState.ReserveAmmo - CachedPlayerSet->BuildSlotResourceState(TargetSlot).ReserveAmmo;

		// Mod 교체가 탄약 수량을 임의로 바꾸지 않도록 보정 적용
		CachedPlayerSet->ApplySlotResourceDelta(TargetSlot, PreserveAmmoDelta);
	}

	// 무기 Item에 Mod 변경을 반영하고 장착 효과 갱신
	TargetWeaponInstance->OnModChanged(GetOwner(), NewModData);

	// Mod 장착 또는 빈 Mod 상태를 마지막 처리 결과로 기록
	TargetWeaponInstance->LastModFailReason = IsValid(NewModData) ? EPRWeaponModFailReason::None : EPRWeaponModFailReason::MissingMod;

	// Mod 변경 결과를 공개 비주얼 정보에 반영
	RefreshVisualInfosFromCurrentState();

	// 서버 로컬 Actor도 Mod 변경 결과에 맞춰 즉시 최신화
	RefreshAllWeaponActors();
	RefreshAnimLayer();

	// 서버 Mod 장착 처리가 확정된 뒤 Owner, 슬롯, 무기 데이터, Mod 데이터를 남겨 Mod 변경 흐름 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][Server] Mod 장착 완료 | Owner = %s | Slot = %d | Weapon = %s | Mod = %s"),
		*GetNameSafe(GetOwner()),
		static_cast<int32>(TargetSlot),
		*GetNameSafe(TargetWeaponInstance->GetWeaponData()),
		*GetNameSafe(NewModData));

	// Mod 상태, 슬롯 자원, 공개 비주얼, 로컬 Actor 갱신 마침
	// Mod 장착 성공
	return true;
}

void UPRWeaponManagerComponent::SetWeaponArmedStateInternal(EPRArmedState NewArmedState)
{
	// 무장 상태가 이미 같은 경우
	if (ArmedState == NewArmedState)
	{
		// 부착 갱신 불필요
		// 함수 조기 종료
		return;
	}

	// 서버 기준 무장 상태 갱신
	ArmedState = NewArmedState;

	// 서버 로컬 Actor도 무장 상태에 맞춰 즉시 최신화
	RefreshAllWeaponActors();
}

void UPRWeaponManagerComponent::SetCurrentWeaponSlotInternal(EPRWeaponSlotType TargetSlot)
{
	// 지원하지 않는 슬롯 요청인 경우
	if (!IsSupportedSlot(TargetSlot))
	{
		// 활성 슬롯 전환 불가
		// 함수 조기 종료
		return;
	}

	// 전환 타겟 슬롯의 원본 Item. 활성 슬롯 구성과 장착 알림 대상 판단에 사용
	UPRItemInstance_Weapon* TargetWeaponInstance = GetWeaponInstanceBySlotType(TargetSlot);

	// 전환 대상 무기 Item 또는 무기 데이터가 없는 경우
	if (!IsValid(TargetWeaponInstance) || !IsValid(TargetWeaponInstance->GetWeaponData()))
	{
		// 빈 슬롯으로 전환 불가
		// 함수 조기 종료
		return;
	}

	// 이미 같은 슬롯이 활성 상태인 경우
	if (CurrentWeaponSlot == TargetSlot)
	{
		// 중복 전환 요청
		// 함수 조기 종료
		return;
	}

	// 전환 전 활성 슬롯. 이전 활성 무기 해제 알림 대상 판단에 사용
	const EPRWeaponSlotType PreWeaponSlot = CurrentWeaponSlot;

	// 이전 활성 슬롯이 있었던 경우
	if (PreWeaponSlot != EPRWeaponSlotType::None)
	{
		// 이전 활성 슬롯의 원본 Item을 찾을 수 있는 경우
		if (UPRItemInstance_Weapon* PreWeaponInstance = GetWeaponInstanceBySlotType(PreWeaponSlot))
		{
			// 이전 활성 무기 해제 알림 전달
			PreWeaponInstance->OnUnequipped(GetOwner());
		}
	}

	// 활성 무기 사용 흐름을 타겟 슬롯으로 전환
	// 타겟 슬롯을 활성 슬롯으로 지정
	CurrentWeaponSlot = TargetSlot;

	// 새 활성 무기의 어빌리티셋을 플레이어에게 부여
	TargetWeaponInstance->OnEquipped(GetOwner());

	// 슬롯 전환은 Actor를 재생성하지 않고 소켓 부착만 최신화하는 경로 유지
	RefreshAllWeaponActors();
	
	RefreshAnimLayer();
}

void UPRWeaponManagerComponent::RefreshWeaponActorForSlot(EPRWeaponSlotType SlotType)
{
	// 지원하지 않는 슬롯 요청인 경우
	if (!IsSupportedSlot(SlotType))
	{
		// 로컬 Actor 갱신 불가
		// 함수 조기 종료
		return;
	}

	// 슬롯 공개 비주얼 정보. Actor 생성, 제거, 부착 위치 판단에 사용
	const FPRWeaponVisualInfo& VisualInfo = GetVisualInfoBySlotType(SlotType);

	// 슬롯별 로컬 Actor 참조. 필요 시 제거 또는 재생성 대상으로 사용
	TObjectPtr<APRWeaponActor>& WeaponActor = GetMutableWeaponActorBySlot(SlotType);

	// 슬롯이 비어 있는 경우
	if (VisualInfo.IsEmpty())
	{
		// 해당 슬롯 Actor 제거
		DestroyWeaponActorForSlot(SlotType);

		// 빈 슬롯 갱신 마침
		// 함수 조기 종료
		return;
	}

	// 월드, 소유자, 무기 데이터, Actor 클래스 중 하나라도 준비되지 않은 경우
	if (!IsValid(GetWorld()) || !IsValid(GetOwner()) || !IsValid(VisualInfo.WeaponData) || !IsValid(VisualInfo.WeaponData->WeaponActorClass))
	{
		// Actor 생성 보류 사유를 Owner, Slot, Weapon, ActorClass 기준으로 추적
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[WeaponManagerComponent][%s] 무기 Actor 생성 보류 | Owner = %s | Slot = %s | Weapon = %s | ActorClass = %s"),
			*GetNetModeLogName(this),
			*GetNameSafe(GetOwner()),
			*UEnum::GetValueAsString(SlotType),
			*GetNameSafe(VisualInfo.WeaponData.Get()),
			IsValid(VisualInfo.WeaponData) ? *GetNameSafe(VisualInfo.WeaponData->WeaponActorClass.Get()) : TEXT("None"));

		// 공개 Actor 생성 조건 미충족
		// 함수 조기 종료
		return;
	}

	// Actor가 없거나 공개 무기 데이터의 Actor 클래스와 다르면 재생성 필요
	const bool bNeedsRespawn = !IsValid(WeaponActor)
		|| WeaponActor->GetClass() != VisualInfo.WeaponData->WeaponActorClass;

	// 같은 슬롯에 다른 무기를 다시 장착했거나 Actor가 없는 경우
	if (bNeedsRespawn)
	{
		// 기존 Actor가 있으면 새 Actor 생성 전에 제거
		DestroyWeaponActorForSlot(SlotType);

		// 소유자와 충돌 정책을 고정해 장착 비주얼 Actor를 항상 생성
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = GetOwner();
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// 공개 무기 데이터의 Actor 클래스로 슬롯 Actor 생성
		WeaponActor = GetWorld()->SpawnActor<APRWeaponActor>(
			VisualInfo.WeaponData->WeaponActorClass,
			FTransform::Identity,
			SpawnParameters);

		// 슬롯 Actor 생성 결과를 Owner, Slot, Weapon, Actor, Class 기준으로 추적
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[WeaponManagerComponent][%s] 무기 Actor 생성 완료 | Owner = %s | Slot = %s | Weapon = %s | Actor = %s | Class = %s"),
			*GetNetModeLogName(this),
			*GetNameSafe(GetOwner()),
			*UEnum::GetValueAsString(SlotType),
			*GetNameSafe(VisualInfo.WeaponData.Get()),
			*GetNameSafe(WeaponActor.Get()),
			*GetNameSafe(VisualInfo.WeaponData->WeaponActorClass.Get()));
	}

	// Actor 생성에 실패한 경우
	if (!IsValid(WeaponActor))
	{
		// 부착 갱신 불가
		// 함수 조기 종료
		return;
	}

	// 생성되었거나 재사용 가능한 Actor를 현재 carry state에 맞춰 부착
	RefreshWeaponAttachmentForSlot(SlotType);
}

void UPRWeaponManagerComponent::RefreshAllWeaponActors()
{
	// 주무기 슬롯 Actor 갱신
	RefreshWeaponActorForSlot(EPRWeaponSlotType::Primary);

	// 보조무기 슬롯 Actor 갱신
	RefreshWeaponActorForSlot(EPRWeaponSlotType::Secondary);
}

void UPRWeaponManagerComponent::RefreshWeaponAttachmentForSlot(EPRWeaponSlotType SlotType)
{
	// 지원하지 않는 슬롯 요청인 경우
	if (!IsSupportedSlot(SlotType))
	{
		// 함수 조기 종료. 부착 갱신 불가
		return;
	}

	// 부착 갱신 대상 Actor와 캐릭터 소유자. 둘 다 있어야 Mesh 소켓 부착 가능
	APRWeaponActor* WeaponActor = GetWeaponActorBySlot(SlotType);
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());

	// 장착된 슬롯 Actor 또는 캐릭터 소유자가 없는 경우
	if (!IsValid(WeaponActor) || !IsValid(OwnerCharacter))
	{
		// 함수 조기 종료. 부착 대상 조건 미충족
		return;
	}

	// 현재 무장 상태와 활성 슬롯을 기준으로 비무장 또는 무장 부착 소켓 결정
	const EPRArmedState SlotArmedState = GetSlotWeaponArmedState(SlotType);
	const FName AttachSocketName = GetAttachTargetSocketName(SlotType, SlotArmedState);

	// 슬롯 Actor 부착 요청을 Owner, Slot, Actor, Socket, SlotArmedState 기준으로 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][%s] 무기 Actor 부착 요청 | Owner = %s | Slot = %s | Actor = %s | Socket = %s | SlotArmedState = %s"),
		*GetNetModeLogName(this),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(SlotType),
		*GetNameSafe(WeaponActor),
		*AttachSocketName.ToString(),
		*UEnum::GetValueAsString(SlotArmedState));

	// 결정된 소켓으로 무기 Actor를 캐릭터 Mesh에 부착
	WeaponActor->AttachToOwnerMesh(OwnerCharacter, AttachSocketName);
}

void UPRWeaponManagerComponent::OnRep_CurrentWeaponSlot(EPRWeaponSlotType OldCurrentWeaponSlot)
{
	// 활성 슬롯 복제 값이 이전 값과 같은 경우
	if (OldCurrentWeaponSlot == CurrentWeaponSlot)
	{
		// 함수 조기 종료. 로컬 Actor 갱신 필요없음
		return;
	}

	const FPRWeaponVisualInfo& CurrentVisualInfo = GetCurrentWeaponVisualInfo();

	// 활성 슬롯 동기화를 Owner, 이전 슬롯, 새 슬롯, 무기 데이터 기준으로 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][%s] 활성 슬롯 동기화 | Owner = %s | OldSlot = %s | NewSlot = %s | Weapon = %s"),
		*GetNetModeLogName(this),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(OldCurrentWeaponSlot),
		*UEnum::GetValueAsString(CurrentWeaponSlot),
		*GetNameSafe(CurrentVisualInfo.WeaponData.Get()));

	// 활성 슬롯 변화에 맞춰 두 슬롯의 Actor와 부착 상태 갱신
	RefreshAllWeaponActors();
	
	RefreshAnimLayer();
}

void UPRWeaponManagerComponent::OnRep_PrimaryVisualInfo(FPRWeaponVisualInfo OldVisualInfo)
{
	// 주무기 공개 비주얼 복제 값이 이전 값과 같은 경우
	if (OldVisualInfo == PrimaryVisualInfo)
	{
		// 함수 조기 종료. 주무기 Actor 갱신 불필요
		return;
	}

	// 주무기 공개 비주얼 동기화를 Owner, 이전 무기, 새 무기, SlotArmedState 기준으로 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][%s] 주무기 비주얼 동기화 | Owner = %s | OldWeapon = %s | NewWeapon = %s | SlotArmedState = %s"),
		*GetNetModeLogName(this),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(OldVisualInfo.WeaponData.Get()),
		*GetNameSafe(PrimaryVisualInfo.WeaponData.Get()),
		*UEnum::GetValueAsString(GetSlotWeaponArmedState(EPRWeaponSlotType::Primary)));

	// 주무기 공개 비주얼 변화에 맞춰 주무기 Actor 갱신
	RefreshWeaponActorForSlot(EPRWeaponSlotType::Primary);
}

void UPRWeaponManagerComponent::OnRep_SecondaryVisualInfo(FPRWeaponVisualInfo OldVisualInfo)
{
	// 보조무기 공개 비주얼 복제 값이 이전 값과 같은 경우
	if (OldVisualInfo == SecondaryVisualInfo)
	{
		// 함수 조기 종료. 보조무기 Actor 갱신 불필요
		return;
	}

	// 보조무기 공개 비주얼 동기화를 Owner, 이전 무기, 새 무기, SlotArmedState 기준으로 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][%s] 보조무기 비주얼 동기화 | Owner = %s | OldWeapon = %s | NewWeapon = %s | SlotArmedState = %s"),
		*GetNetModeLogName(this),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(OldVisualInfo.WeaponData.Get()),
		*GetNameSafe(SecondaryVisualInfo.WeaponData.Get()),
		*UEnum::GetValueAsString(GetSlotWeaponArmedState(EPRWeaponSlotType::Secondary)));

	// 보조무기 공개 비주얼 변화에 맞춰 보조무기 Actor 갱신
	RefreshWeaponActorForSlot(EPRWeaponSlotType::Secondary);
}

void UPRWeaponManagerComponent::OnRep_ArmedState(EPRArmedState OldArmedState)
{
	// 무장 상태 복제 값이 이전 값과 같은 경우
	if (OldArmedState == ArmedState)
	{
		// 함수 조기 종료. 로컬 Actor 부착 갱신 불필요
		return;
	}

	// 무장 상태 동기화를 Owner, 이전 상태, 새 상태 기준으로 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][%s] 무장 상태 동기화 | Owner = %s | OldState = %s | NewState = %s"),
		*GetNetModeLogName(this),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(OldArmedState),
		*UEnum::GetValueAsString(ArmedState));

	// 무장 상태 변화에 맞춰 두 슬롯의 Actor와 부착 상태 갱신
	RefreshAllWeaponActors();
}

void UPRWeaponManagerComponent::Server_EquipWeaponItemById_Implementation(const FGuid& ItemId)
{
	// 서버 RPC 진입 시 PlayerState 기반 런타임 링크 갱신
	InitializeRuntimeLinks();

	// 서버 권위에서 ItemId 기반 장착 처리
	EquipWeaponItemByIdInternal(ItemId);
}

void UPRWeaponManagerComponent::Server_UnequipWeaponSlot_Implementation(EPRWeaponSlotType TargetSlot)
{
	// 서버 권위에서 슬롯 해제 처리
	UnequipWeaponFromSlotInternal(TargetSlot);
}

void UPRWeaponManagerComponent::Server_SetCurrentWeaponSlot_Implementation(EPRWeaponSlotType TargetSlot)
{
	// 서버 권위에서 활성 슬롯 전환 처리
	SetCurrentWeaponSlotInternal(TargetSlot);
}

void UPRWeaponManagerComponent::Server_SetWeaponArmedState_Implementation(EPRArmedState NewArmedState)
{
	// 서버 권위에서 무장 상태 변경 처리
	SetWeaponArmedStateInternal(NewArmedState);
}

void UPRWeaponManagerComponent::Server_AttachModToSlot_Implementation(EPRWeaponSlotType TargetSlot, UPRWeaponModDataAsset* NewModData)
{
	// 서버 RPC 진입 시 PlayerState 기반 런타임 링크 갱신
	InitializeRuntimeLinks();

	// 서버 권위에서 Mod 장착 처리
	AttachModToSlotInternal(TargetSlot, NewModData);
}

FPRWeaponVisualInfo UPRWeaponManagerComponent::BuildVisualInfo(EPRWeaponSlotType SlotType, const UPRItemInstance_Weapon* WeaponItem) const
{
	// 기본 공개 비주얼 정보는 슬롯 타입을 먼저 반영
	FPRWeaponVisualInfo NewVisualInfo;
	NewVisualInfo.SlotType = SlotType;

	// 무기 Item 또는 무기 데이터가 없는 경우
	if (!IsValid(WeaponItem) || !IsValid(WeaponItem->GetWeaponData()))
	{
		// 슬롯 타입은 유지하되 무기와 Mod 공개 데이터 초기화
		NewVisualInfo.Reset(SlotType);

		// 빈 공개 비주얼 정보 리턴
		return NewVisualInfo;
	}

	// 원본 Item 기준으로 공개 가능한 무기와 Mod 데이터를 구성
	NewVisualInfo.WeaponData = WeaponItem->GetWeaponData();
	NewVisualInfo.ModData = WeaponItem->GetModData();

	// 공개 비주얼 정보 구성 마침
	return NewVisualInfo;
}

EPRArmedState UPRWeaponManagerComponent::GetSlotWeaponArmedState(EPRWeaponSlotType SlotType) const
{
	// 현재 무장 상태가 Armed이고 활성 슬롯과 일치하는 슬롯인 경우
	if (ArmedState == EPRArmedState::Armed
		&& CurrentWeaponSlot == SlotType)
	{
		// 무장 상태로 리턴
		return EPRArmedState::Armed;
	}

	// 활성 슬롯이 아니거나 비무장 상태인 슬롯은 비무장 상태로 리턴
	return EPRArmedState::Unarmed;
}

FName UPRWeaponManagerComponent::GetAttachTargetSocketName(EPRWeaponSlotType SlotType, EPRArmedState SlotArmedState) const
{
	// 손에 든 무기인 경우
	if (SlotArmedState == EPRArmedState::Armed)
	{
		// 손 무기 소켓 리턴
		return TEXT("Gun_Attach");
	}

	// 수납 상태의 주무기인 경우
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		// 주무기 수납 소켓 리턴
		return TEXT("LongGun_Stow");
	}

	// 수납 상태의 보조무기인 경우
	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		// 보조무기 수납 소켓 리턴
		return TEXT("Pistol_Stow");
	}

	// 지원하지 않는 슬롯은 부착 소켓 없음으로 리턴
	return NAME_None;
}

void UPRWeaponManagerComponent::RefreshVisualInfosFromCurrentState()
{
	// 서버 원본 주무기 상태를 공개 비주얼 정보로 재구성
	PrimaryVisualInfo = BuildVisualInfo(EPRWeaponSlotType::Primary, PrimaryWeaponInstance);

	// 서버 원본 보조무기 상태를 공개 비주얼 정보로 재구성
	SecondaryVisualInfo = BuildVisualInfo(EPRWeaponSlotType::Secondary, SecondaryWeaponInstance);
}

EPRWeaponSlotType UPRWeaponManagerComponent::ResolveNextWeaponSlotAfterUnequip(EPRWeaponSlotType UnequippedSlot) const
{
	// 기본값은 빈 활성 슬롯. 남은 무기가 없으면 그대로 리턴
	EPRWeaponSlotType NextWeaponSlot = EPRWeaponSlotType::None;

	// 해제된 슬롯이 주무기가 아니고 주무기 원본이 남아 있는 경우
	if (UnequippedSlot != EPRWeaponSlotType::Primary && IsValid(PrimaryWeaponInstance))
	{
		// 주무기를 다음 활성 슬롯으로 리턴
		return EPRWeaponSlotType::Primary;
	}

	// 해제된 슬롯이 보조무기가 아니고 보조무기 원본이 남아 있는 경우
	if (UnequippedSlot != EPRWeaponSlotType::Secondary && IsValid(SecondaryWeaponInstance))
	{
		// 보조무기를 다음 활성 슬롯으로 리턴
		return EPRWeaponSlotType::Secondary;
	}

	// 빈 활성 슬롯 리턴. 남은 장착 무기 없음
	return NextWeaponSlot;
}

bool UPRWeaponManagerComponent::IsSupportedSlot(EPRWeaponSlotType SlotType) const
{
	// 무기 매니저가 관리하는 주무기와 보조무기 슬롯만 지원
	return SlotType == EPRWeaponSlotType::Primary || SlotType == EPRWeaponSlotType::Secondary;
}

void UPRWeaponManagerComponent::RefreshAnimLayer()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!IsValid(OwnerCharacter))
	{
		// 주무기를 다음 활성 슬롯으로 리턴
		return EPRWeaponSlotType::Primary;
	}

	USkeletalMeshComponent* MeshComp = OwnerCharacter->GetMesh();
	if (!IsValid(MeshComp))
	{
		return;
	}

	// 다음에 장착해야 할 무기의 레이어 클래스를 파악한다
	TSubclassOf<UAnimInstance> TargetAnimLayerClass = nullptr;
                                                                                      
	const FPRWeaponVisualInfo& CurrentWeaponVisualInfo = GetCurrentWeaponVisualInfo();    
	if (!CurrentWeaponVisualInfo.IsEmpty() && IsValid(CurrentWeaponVisualInfo.WeaponData))
	{
		TargetAnimLayerClass = CurrentWeaponVisualInfo.WeaponData->WeaponAnimLayerClass;
	}                                                                                     

	// 이미 목표 레이어가 링크되어 있다면 작업을 무시한다
	if (CurrentLinkedAnimLayerClass == TargetAnimLayerClass)
	{
		// 보조무기를 다음 활성 슬롯으로 리턴
		return EPRWeaponSlotType::Secondary;
	}

	// 기존에 링크된 레이어가 있다면 언링크(Unlink) 하여 맨손 상태로 되돌린다.
	if (IsValid(CurrentLinkedAnimLayerClass))
	{
		MeshComp->UnlinkAnimClassLayers(CurrentLinkedAnimLayerClass);
		CurrentLinkedAnimLayerClass = nullptr;
	}

	// 새로운 무기 레이어가 지정되어 있다면 새롭게 링크(Link) 한다.
	if (IsValid(TargetAnimLayerClass))
	{
		MeshComp->LinkAnimClassLayers(TargetAnimLayerClass);
		CurrentLinkedAnimLayerClass = TargetAnimLayerClass;
	}
	// 만약 Unlink 시 레이어가 아예 없어져서 T자 포즈가 된다면, 주석을 풀고 PRPlayerCharacter의 DefaultAnimLayerClass를 public으로 변환
	// else
	// {
	// 	MeshComp->LinkAnimClassLayers(OwnerCharacter->DefaultAnimLayerClass);
	// }
}


TObjectPtr<UPRItemInstance_Weapon>& UPRWeaponManagerComponent::GetMutableWeaponInstanceBySlot(EPRWeaponSlotType SlotType)
{
	// 주무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		// 주무기 원본 Item 참조 리턴
		return PrimaryWeaponInstance;
	}

	// 보조무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		// 보조무기 원본 Item 참조 리턴
		return SecondaryWeaponInstance;
	}

	// 지원하지 않는 슬롯은 호출 경로 오류로 간주
	checkNoEntry();

	// 빌드 구성을 위해 주무기 원본 참조 리턴
	return PrimaryWeaponInstance;
}

APRWeaponActor* UPRWeaponManagerComponent::GetWeaponActorBySlot(EPRWeaponSlotType SlotType) const
{
	// 주무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		// 주무기 Actor 리턴
		return PrimaryWeaponActor;
	}

	// 보조무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		// 보조무기 Actor 리턴
		return SecondaryWeaponActor;
	}

	// 지원하지 않는 슬롯은 Actor 없음으로 리턴
	return nullptr;
}

TObjectPtr<APRWeaponActor>& UPRWeaponManagerComponent::GetMutableWeaponActorBySlot(EPRWeaponSlotType SlotType)
{
	// 주무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		// 주무기 Actor 참조 리턴
		return PrimaryWeaponActor;
	}

	// 보조무기 슬롯 요청인 경우
	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		// 보조무기 Actor 참조 리턴
		return SecondaryWeaponActor;
	}

	// 지원하지 않는 슬롯은 호출 경로 오류로 간주
	checkNoEntry();

	// 빌드 구성을 위해 주무기 Actor 참조 리턴
	return PrimaryWeaponActor;
}

void UPRWeaponManagerComponent::DestroyWeaponActorForSlot(EPRWeaponSlotType SlotType)
{
	// 슬롯별 로컬 Actor 참조. 제거 후 참조 초기화에 사용
	TObjectPtr<APRWeaponActor>& WeaponActor = GetMutableWeaponActorBySlot(SlotType);

	// 제거할 Actor가 없는 경우
	if (!IsValid(WeaponActor))
	{
		// 함수 조기 종료. Actor 제거 불필요
		return;
	}

	// 슬롯 Actor 제거를 Owner, Slot, Actor 기준으로 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponManagerComponent][%s] 무기 Actor 제거. DestroyWeaponActorForSlot | Owner = %s | Slot = %s | Actor = %s"),
		*GetNetModeLogName(this),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(SlotType),
		*GetNameSafe(WeaponActor.Get()));

	// 로컬 무기 Actor 제거
	WeaponActor->Destroy();

	// 제거된 Actor 참조 초기화
	WeaponActor = nullptr;
}
