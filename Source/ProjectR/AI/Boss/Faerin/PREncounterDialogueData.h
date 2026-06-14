// Copyright ProjectR. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PREncounterDialogueData.generated.h"

class USoundBase;

// Faerin 인카운터 컷신/선택지 대사 한 줄의 표시 데이터를 보관한다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinDialogueLine
{
	GENERATED_BODY()

	// 대사 라인을 외부 컷신/자막 데이터와 매칭하기 위한 식별자
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	FName LineId = NAME_None;

	// 자막에 표시할 화자 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	FText SpeakerName;

	// 자막에 표시할 본문 텍스트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	FText SubtitleText;

	// 선택적으로 재생할 음성 에셋
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	TObjectPtr<USoundBase> Voice = nullptr;

	// 음성이 없거나 짧을 때 보장할 최소 표시 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue", meta = (ClampMin = "0.0"))
	float MinDisplayTime = 1.0f;

	// 호스트가 해당 라인을 스킵할 수 있는지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	bool bHostCanSkip = true;
};

// Faerin 인카운터 대사 묶음을 DataAsset으로 보관한다.
UCLASS(BlueprintType)
class PROJECTR_API UPRFaerinEncounterDialogueData : public UDataAsset
{
	GENERATED_BODY()

public:
	// 최초 조우 컷신에서 재생할 대사 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	TArray<FPRFaerinDialogueLine> IntroLines;

	// 상호작용 선택지 UI 진입 시 사용할 대사 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	TArray<FPRFaerinDialogueLine> ChoiceLines;

	// 전투 거절 후 사용할 대사 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	TArray<FPRFaerinDialogueLine> DeclineLines;

	// 전투 시작 컷신 직전에 사용할 대사 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	TArray<FPRFaerinDialogueLine> PreFightLines;
};
