// Copyright ProjectR. All Rights Reserved.

#include "PRFXCue.h"

#include "PRFXSubsystem.h"
#include "Engine/World.h"
#include "ProjectR/ItemSystem/Actors/PRWeaponActor.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "ProjectR/Utils/PRGameplayStatics.h"

DEFINE_LOG_CATEGORY(LogPRFX);

namespace PRFXCuePrivate
{
	template <typename TPayload>
	const TPayload* ResolveTypedPayload(const UObject* CueObject, const FInstancedStruct& Payload, const FPRFXCueContext& Context)
	{
		// 역할별 Cue가 요구하는 구체 Payload 타입 확인
		const TPayload* TypedPayload = Payload.GetPtr<TPayload>();
		if (TypedPayload)
		{
			return TypedPayload;
		}

		const UScriptStruct* ActualStruct = Payload.GetScriptStruct();
		const FString ExpectedName = TPayload::StaticStruct()->GetName();
		const FString ActualName = ActualStruct ? ActualStruct->GetName() : TEXT("None");

		// Registry 태그와 Cue 클래스가 요구하는 Payload 타입 불일치 감지
		ensureMsgf(false,
			TEXT("FX Cue Payload 타입 불일치. Cue=%s, FXTag=%s, Expected=%s, Actual=%s"),
			*GetNameSafe(CueObject),
			*Context.FXTag.ToString(),
			*ExpectedName,
			*ActualName);

		UE_LOG(LogPRFX, Warning,
			TEXT("FX Cue Payload 타입 불일치. Cue=%s, FXTag=%s, Expected=%s, Actual=%s"),
			*GetNameSafe(CueObject),
			*Context.FXTag.ToString(),
			*ExpectedName,
			*ActualName);

		return nullptr;
	}
}

/*~ UPRFXCue ~*/

void UPRFXCue::NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload)
{
	// 역할별 Cue 파생 클래스를 거치지 않은 기반 클래스 직접 실행 감지
	UE_LOG(LogPRFX, Warning,
		TEXT("기반 FX Cue가 직접 실행됨. Cue=%s, FXTag=%s"),
		*GetNameSafe(this),
		*Context.FXTag.ToString());
}

const UScriptStruct* UPRFXCue::GetExpectedPayloadType() const
{
	return nullptr;
}

EPRFXCueInstancingPolicy UPRFXCue::GetInstancingPolicy() const
{
	return InstancingPolicy;
}

void UPRFXSimpleCue::NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload)
{
	if (const FPRFXPayloadBase* TypedPayload = PRFXCuePrivate::ResolveTypedPayload<FPRFXPayloadBase>(this, Payload, Context))
	{
		ExecuteFX(Context, *TypedPayload);
	}
}

const UScriptStruct* UPRFXSimpleCue::GetExpectedPayloadType() const
{
	return FPRFXPayloadBase::StaticStruct();
}

void UPRFXSimpleCue::ExecuteFX_Implementation(const FPRFXCueContext& Context, const FPRFXPayloadBase& Payload)
{
}

/*~ UPRFXLocationCue ~*/

void UPRFXLocationCue::NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload)
{
	// FInstancedStruct Payload를 위치 기반 Payload로 확인한 뒤 BlueprintNativeEvent 호출
	if (const FPRFXLocationPayload* TypedPayload = PRFXCuePrivate::ResolveTypedPayload<FPRFXLocationPayload>(this, Payload, Context))
	{
		ExecuteFX(Context, *TypedPayload);
	}
}

const UScriptStruct* UPRFXLocationCue::GetExpectedPayloadType() const
{
	return FPRFXLocationPayload::StaticStruct();
}

void UPRFXLocationCue::ExecuteFX_Implementation(const FPRFXCueContext& Context, const FPRFXLocationPayload& Payload)
{
}

/*~ UPRFXAttachCue ~*/

void UPRFXAttachCue::NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload)
{
	// FInstancedStruct Payload를 부착 기반 Payload로 확인한 뒤 BlueprintNativeEvent 호출
	if (const FPRFXAttachPayload* TypedPayload = PRFXCuePrivate::ResolveTypedPayload<FPRFXAttachPayload>(this, Payload, Context))
	{
		ExecuteFX(Context, *TypedPayload);
	}
}

const UScriptStruct* UPRFXAttachCue::GetExpectedPayloadType() const
{
	return FPRFXAttachPayload::StaticStruct();
}

void UPRFXAttachCue::ExecuteFX_Implementation(const FPRFXCueContext& Context, const FPRFXAttachPayload& Payload)
{
}

/*~ UPRFXTrailCue ~*/

void UPRFXTrailCue::NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload)
{
	// FInstancedStruct Payload를 Trail 기반 Payload로 확인한 뒤 BlueprintNativeEvent 호출
	if (const FPRFXTrailPayload* TypedPayload = PRFXCuePrivate::ResolveTypedPayload<FPRFXTrailPayload>(this, Payload, Context))
	{
		ExecuteFX(Context, *TypedPayload);
	}
}

const UScriptStruct* UPRFXTrailCue::GetExpectedPayloadType() const
{
	return FPRFXTrailPayload::StaticStruct();
}

void UPRFXTrailCue::ExecuteFX_Implementation(const FPRFXCueContext& Context, const FPRFXTrailPayload& Payload)
{
	// Payload 무기 데이터와 현재 무기 데이터 일치 확인
	APRWeaponActor* WeaponActor = ResolveMatchingWeaponActor(Payload);
	if (!IsValid(WeaponActor))
	{
		return;
	}

	// FX 시스템 Payload를 무기 Actor 전용 파라미터로 변환
	FPRWeaponFireFXParams FireFXParams;
	FireFXParams.StartLocation = Payload.StartLocation;
	FireFXParams.TrailEnds = Payload.TrailEnds;
	FireFXParams.bBlockingHit = Payload.bBlockingHit;
	FireFXParams.Direction = Payload.Direction;

	// 무기 Actor 소유 발사 FX 재생
	WeaponActor->TriggerFireFX(FireFXParams);
}

APRWeaponActor* UPRFXTrailCue::ResolveMatchingWeaponActor(const FPRFXTrailPayload& Payload) const
{
	if (!IsValid(Payload.SourceActor) || !IsValid(Payload.WeaponData))
	{
		return nullptr;
	}

	// 발사자 기준 무기 매니저 조회
	UPRWeaponManagerComponent* WeaponManager = UPRGameplayStatics::GetWeaponManagerComponent(Payload.SourceActor);
	if (!IsValid(WeaponManager))
	{
		return nullptr;
	}

	// 지연 도착 FX의 무기 교체 오재생 방지
	const FPRWeaponVisualInfo& VisualInfo = WeaponManager->GetCurrentWeaponVisualInfo();
	if (VisualInfo.WeaponData.Get() != Payload.WeaponData.Get())
	{
		return nullptr;
	}

	// 현재 손에 든 로컬 무기 Actor 반환
	return WeaponManager->GetActiveWeaponActor();
}

/*~ UPRFXWeaponFireTrailCue ~*/
void UPRFXWeaponFireTrailCue::ExecuteFX_Implementation(const FPRFXCueContext& Context, const FPRFXTrailPayload& Payload)
{
}

/*~ UPRFXHitCue ~*/

void UPRFXHitCue::NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload)
{
	// FInstancedStruct Payload를 탄착 기반 Payload로 확인한 뒤 BlueprintNativeEvent 호출
	if (const FPRFXHitPayload* TypedPayload = PRFXCuePrivate::ResolveTypedPayload<FPRFXHitPayload>(this, Payload, Context))
	{
		ExecuteFX(Context, *TypedPayload);
	}
}

const UScriptStruct* UPRFXHitCue::GetExpectedPayloadType() const
{
	return FPRFXHitPayload::StaticStruct();
}

void UPRFXHitCue::ExecuteFX_Implementation(const FPRFXCueContext& Context, const FPRFXHitPayload& Payload)
{
}

/*~ UPRFXActorSpawnCue ~*/

void UPRFXActorSpawnCue::NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload)
{
	// FInstancedStruct Payload를 Actor Spawn 기반 Payload로 확인한 뒤 BlueprintNativeEvent 호출
	if (const FPRFXActorSpawnPayload* TypedPayload = PRFXCuePrivate::ResolveTypedPayload<FPRFXActorSpawnPayload>(this, Payload, Context))
	{
		ExecuteFX(Context, *TypedPayload);
	}
}

const UScriptStruct* UPRFXActorSpawnCue::GetExpectedPayloadType() const
{
	return FPRFXActorSpawnPayload::StaticStruct();
}

void UPRFXActorSpawnCue::ExecuteFX_Implementation(const FPRFXCueContext& Context, const FPRFXActorSpawnPayload& Payload)
{
}

/*~ UPRFXPersistentCue ~*/

void UPRFXPersistentCue::NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload)
{
	// 지속 FX 시작과 종료에 필요한 Persistent Payload 타입 확인
	const FPRFXPersistentPayload* TypedPayload = PRFXCuePrivate::ResolveTypedPayload<FPRFXPersistentPayload>(this, Payload, Context);
	if (!TypedPayload)
	{
		return;
	}

	if (!TypedPayload->PersistentId.IsValid())
	{
		ensureMsgf(false,
			TEXT("지속 FX 식별자가 유효하지 않음. Cue=%s, FXTag=%s"),
			*GetNameSafe(this),
			*Context.FXTag.ToString());
		return;
	}

	UWorld* World = GetWorld();
	UPRFXSubsystem* FXSubsystem = World ? World->GetSubsystem<UPRFXSubsystem>() : nullptr;
	if (!IsValid(FXSubsystem))
	{
		UE_LOG(LogPRFX, Warning,
			TEXT("지속 FX Subsystem 조회 실패. Cue=%s, FXTag=%s"),
			*GetNameSafe(this),
			*Context.FXTag.ToString());
		return;
	}

	if (TypedPayload->Action == EPRFXPersistentAction::Start)
	{
		// Cue가 생성한 지속 Actor를 PersistentId로 다시 찾기 위한 Subsystem 등록
		AActor* PersistentActor = StartFX(Context, *TypedPayload);
		if (IsValid(PersistentActor))
		{
			FXSubsystem->RegisterPersistentFX(TypedPayload->PersistentId, PersistentActor);
		}
		return;
	}

	// StartFX에서 생성된 Actor를 PersistentId로 찾아 StopFX에 전달
	AActor* PersistentActor = FXSubsystem->FindPersistentFX(TypedPayload->PersistentId);
	StopFX(Context, *TypedPayload, PersistentActor);
	FXSubsystem->UnregisterPersistentFX(TypedPayload->PersistentId);
}

const UScriptStruct* UPRFXPersistentCue::GetExpectedPayloadType() const
{
	return FPRFXPersistentPayload::StaticStruct();
}

AActor* UPRFXPersistentCue::StartFX_Implementation(const FPRFXCueContext& Context, const FPRFXPersistentPayload& Payload)
{
	return nullptr;
}

void UPRFXPersistentCue::StopFX_Implementation(const FPRFXCueContext& Context, const FPRFXPersistentPayload& Payload, AActor* PersistentActor)
{
	if (IsValid(PersistentActor))
	{
		PersistentActor->Destroy();
	}
}
