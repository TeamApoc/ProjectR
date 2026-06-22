// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 협상 전투 선택 대화 상호작용 액션 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Interaction/PRInteractionAction.h"
#include "PRInteraction_FaerinEncounterChoice.generated.h"

class APRFaerinEncounterDirector;

// Faerin 협상 상태에서 전투 선택 대화를 여는 상호작용 액션이다.
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class PROJECTR_API UPRInteraction_FaerinEncounterChoice : public UPRInteractionAction
{
	GENERATED_BODY()

public:
	UPRInteraction_FaerinEncounterChoice();

	/*~ UPRInteractionAction Interface ~*/
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Execute_Implementation(AActor* Interactor) override;

public:
	// 선택 대화 상태를 관리하는 Faerin EncounterDirector
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	TObjectPtr<APRFaerinEncounterDirector> EncounterDirector = nullptr;
};
