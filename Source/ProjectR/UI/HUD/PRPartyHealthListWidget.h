// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRPartyHealthListWidget.generated.h"

class APRPlayerState;
class UPRPartyMemberHealthWidget;

// 파티원 체력 목록을 표시하는 부모 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRPartyHealthListWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRPartyHealthListWidget(const FObjectInitializer& ObjectInitializer);

	// 현재 GameState PlayerArray를 읽어 파티원 체력 슬롯을 갱신한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Party")
	void RefreshPartyMembers();

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	APRPlayerState* GetOwningPRPlayerState() const;
	void ApplyPartyMembers(const TArray<APRPlayerState*>& PartyMembers);

private:
	// 첫 번째 파티원 체력 슬롯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<UPRPartyMemberHealthWidget> PartyMemberSlot0;

	// 두 번째 파티원 체력 슬롯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<UPRPartyMemberHealthWidget> PartyMemberSlot1;

	// 파티원 목록을 다시 확인할 로컬 타이머 간격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0.1"), Category = "HUD")
	float PartyRefreshInterval = 0.5f;

	FTimerHandle PartyRefreshTimerHandle;
};
