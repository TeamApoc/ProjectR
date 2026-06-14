// Copyright ProjectR. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRFaerinEncounterChoiceWidget.generated.h"

class APRFaerinEncounterDirector;

// Faerin 협상 선택지를 owning client에서 표시하고 서버 선택 요청을 전달한다.
UCLASS(Blueprintable)
class PROJECTR_API UPRFaerinEncounterChoiceWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRFaerinEncounterChoiceWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 선택 UI가 사용할 Director 컨텍스트를 갱신한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void InitializeChoice(APRFaerinEncounterDirector* InDirector);

	// 전투 시작 선택을 서버에 요청한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void RequestFight();

	// 전투 거절 선택을 서버에 요청한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void RequestDecline();

protected:
	/*~ UUserWidget Interface ~*/
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

protected:
	// Blueprint 위젯이 선택지 텍스트와 버튼 상태를 초기화할 수 있도록 호출된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void BP_OnChoiceInitialized(APRFaerinEncounterDirector* InDirector);

private:
	// 선택 결과를 전달할 서버 권위 Director
	UPROPERTY(Transient)
	TObjectPtr<APRFaerinEncounterDirector> EncounterDirector = nullptr;
};
