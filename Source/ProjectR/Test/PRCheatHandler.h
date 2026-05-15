// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ActiveGameplayEffectHandle.h"
#include "PRCheatHandler.generated.h"

class APRPlayerController;
class UGameplayEffect;
class UPRAbilitySystemComponent;

/**
 * 치트 명령 수신 및 서버 권위에서 실제 작업 수행
 * PlayerController의 ReplicatedSubObject로 등록되어 Server RPC 라우팅 지원
 */
UCLASS(Blueprintable)
class PROJECTR_API UPRCheatHandler : public UObject
{
	GENERATED_BODY()

public:
	/*~ UObject Interface ~*/
	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;
	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack) override;
	virtual bool IsSupportedForNetworking() const override { return true; }

	/*~ UPRCheatHandler Interface ~*/
	// 클라 요청 수신 후 서버에서 본인 폰을 재스폰
	UFUNCTION(Server, Reliable)
	void ServerCheatRespawn();

	// 클라 요청 수신 후 서버에서 양쪽 무기 슬롯 탄약을 최대치로 세팅
	UFUNCTION(Server, Reliable)
	void ServerCheatFillAmmo();

	// 클라 요청 수신 후 서버에서 무한 모드 GE 적용 또는 회수
	UFUNCTION(Server, Reliable)
	void ServerCheatInfiniteMode(bool bEnable);

private:
	// 컨트롤러 소유 PlayerState의 ASC 조회
	UPRAbilitySystemComponent* GetOwningPlayerASC() const;

protected:
	// 무한 모드용 GE 클래스. 에디터에서 Infinite Duration 타입으로 지정 필요
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Cheat")
	TSubclassOf<UGameplayEffect> InfiniteModeEffectClass;

private:
	// 현재 적용 중인 무한 모드 GE 핸들 (서버 권위 상태)
	FActiveGameplayEffectHandle InfiniteModeEffectHandle;
};
