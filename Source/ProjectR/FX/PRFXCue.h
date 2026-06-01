// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRFXTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Object.h"
#include "PRFXCue.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPRFX, Log, All);

// 모든 FX Cue의 공통 타입 소거 진입점
UCLASS(Abstract, Blueprintable, EditInlineNew)
class PROJECTR_API UPRFXCue : public UObject
{
	GENERATED_BODY()

public:
	// FX 시스템이 호출하는 공통 실행 함수
	virtual void NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload);

	// Cue 클래스가 처리할 수 있는 Payload 구조체 타입 반환
	virtual const UScriptStruct* GetExpectedPayloadType() const;

	// Cue UObject의 재사용 또는 생성 기준 반환
	EPRFXCueInstancingPolicy GetInstancingPolicy() const;

public:
	// Cue UObject의 재사용 또는 생성 기준
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|FX")
	EPRFXCueInstancingPolicy InstancingPolicy = EPRFXCueInstancingPolicy::NonInstanced;
};

// 월드 위치 기반 Cue
UCLASS(Abstract, Blueprintable, EditInlineNew)
class PROJECTR_API UPRFXLocationCue : public UPRFXCue
{
	GENERATED_BODY()

public:
	// 위치 Payload 검증 후 typed ExecuteFX로 전달
	virtual void NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload) override;

	// Cue 클래스가 처리할 수 있는 Payload 구조체 타입 반환
	virtual const UScriptStruct* GetExpectedPayloadType() const override;

	// Blueprint와 C++ 파생 클래스가 구현하는 위치 기반 재생 함수
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|FX")
	void ExecuteFX(const FPRFXCueContext& Context, const FPRFXLocationPayload& Payload);
};

// 액터 또는 컴포넌트 부착 기반 Cue
UCLASS(Abstract, Blueprintable, EditInlineNew)
class PROJECTR_API UPRFXAttachCue : public UPRFXCue
{
	GENERATED_BODY()

public:
	// 부착 Payload 검증 후 typed ExecuteFX로 전달
	virtual void NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload) override;

	// Cue 클래스가 처리할 수 있는 Payload 구조체 타입 반환
	virtual const UScriptStruct* GetExpectedPayloadType() const override;

	// Blueprint와 C++ 파생 클래스가 구현하는 부착 기반 재생 함수
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|FX")
	void ExecuteFX(const FPRFXCueContext& Context, const FPRFXAttachPayload& Payload);
};

// Trail 기반 Cue
UCLASS(Abstract, Blueprintable, EditInlineNew)
class PROJECTR_API UPRFXTrailCue : public UPRFXCue
{
	GENERATED_BODY()

public:
	// Trail Payload 검증 후 typed ExecuteFX로 전달
	virtual void NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload) override;

	// Cue 클래스가 처리할 수 있는 Payload 구조체 타입 반환
	virtual const UScriptStruct* GetExpectedPayloadType() const override;

	// Blueprint와 C++ 파생 클래스가 구현하는 Trail 재생 함수
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|FX")
	void ExecuteFX(const FPRFXCueContext& Context, const FPRFXTrailPayload& Payload);
};

// 탄착과 피격 기반 Cue
UCLASS(Abstract, Blueprintable, EditInlineNew)
class PROJECTR_API UPRFXHitCue : public UPRFXCue
{
	GENERATED_BODY()

public:
	// Hit Payload 검증 후 typed ExecuteFX로 전달
	virtual void NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload) override;

	// Cue 클래스가 처리할 수 있는 Payload 구조체 타입 반환
	virtual const UScriptStruct* GetExpectedPayloadType() const override;

	// Blueprint와 C++ 파생 클래스가 구현하는 탄착과 피격 재생 함수
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|FX")
	void ExecuteFX(const FPRFXCueContext& Context, const FPRFXHitPayload& Payload);
};

// Actor Spawn 기반 Cue
UCLASS(Abstract, Blueprintable, EditInlineNew)
class PROJECTR_API UPRFXActorSpawnCue : public UPRFXCue
{
	GENERATED_BODY()

public:
	// Actor Spawn Payload 검증 후 typed ExecuteFX로 전달
	virtual void NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload) override;

	// Cue 클래스가 처리할 수 있는 Payload 구조체 타입 반환
	virtual const UScriptStruct* GetExpectedPayloadType() const override;

	// Blueprint와 C++ 파생 클래스가 구현하는 Actor Spawn 재생 함수
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|FX")
	void ExecuteFX(const FPRFXCueContext& Context, const FPRFXActorSpawnPayload& Payload);
};

// 상태 지속 기반 Cue
UCLASS(Abstract, Blueprintable, EditInlineNew)
class PROJECTR_API UPRFXPersistentCue : public UPRFXCue
{
	GENERATED_BODY()

public:
	// 지속 FX Payload의 Action을 읽고 시작 또는 종료 함수로 분기
	virtual void NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload) override;

	// Cue 클래스가 처리할 수 있는 Payload 구조체 타입 반환
	virtual const UScriptStruct* GetExpectedPayloadType() const override;

	// Blueprint와 C++ 파생 클래스가 구현하는 지속 FX Actor 생성 함수
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|FX")
	AActor* StartFX(const FPRFXCueContext& Context, const FPRFXPersistentPayload& Payload);

	// Blueprint와 C++ 파생 클래스가 구현하는 지속 FX Actor 종료 함수
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|FX")
	void StopFX(const FPRFXCueContext& Context, const FPRFXPersistentPayload& Payload, AActor* PersistentActor);
};
