// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "PRPlayerController.generated.h"

class UAbilitySystemComponent;
class UPRInputConfigDataAsset;
class UPRAbilitySystemComponent;
struct FInputActionValue;

// 플레이어 입력·UI 소유. Join 시 캐릭터 페이로드를 서버로 전송하고,
// 인게임 중 발생한 보상 Grant를 연결이 살아있는 동안 즉시 수령하여 GameInstance에 반영한다
UCLASS()
class PROJECTR_API APRPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/*~ APlayerController Interface ~*/
	virtual void AcknowledgePossession(APawn* InPawn) override;
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

public:
	// 로컬 클라에서 호출. GameInstance의 LocalCharacter를 서버로 제출
	// 자동 호출(BeginPlay)과 수동 호출(재제출) 모두 허용
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Session")
	void SubmitLocalCharacterToServer();

	// 서버 -> 본인 클라. 캐릭터 페이로드 검증 결과. 거부 시 Detail에 사유
	UFUNCTION(Client, Reliable)
	void ClientCharacterAccepted(bool bAccepted, const FString& Detail);

	// 서버 -> 본인 클라. 보상 발생 이벤트 시점에 즉시 호출
	// 수신 즉시 GameInstance.ApplyRewardGrant로 반영. 세션 종료 정산 경로는 사용하지 않는다
	UFUNCTION(Client, Reliable)
	void ClientGrantReward(const FPRRewardGrant& Grant);

protected:
	// 클라이언트 -> 서버. 로컬 캐릭터 페이로드 제출
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSubmitCharacter(const FPRCharacterSaveData& Payload);

	// IA에 대한 InputTag 매핑을 담고 있는 InputConfig. 각 IA에 Pressed/Released 콜백을 라우팅
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input")
	TObjectPtr<UPRInputConfigDataAsset> InputConfig;

	// IA Pressed 콜백. InputTag를 ASC로 전달
	void OnAbilityInputPressed(FGameplayTag InputTag);

	// IA Released 콜백
	void OnAbilityInputReleased(FGameplayTag InputTag);

private:
	// 폰 -> PlayerState 경로로 ASC 조회
	UPRAbilitySystemComponent* GetASC() const;

	// 캐릭터 페이로드를 이미 제출했는지 여부. 중복 제출 방지
	bool bCharacterSubmitted = false;
	
	mutable TWeakObjectPtr<UPRAbilitySystemComponent> CachedASC;
};
