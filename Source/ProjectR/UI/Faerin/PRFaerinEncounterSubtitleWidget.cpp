// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 인카운터 하단 자막 위젯 구현)
#include "ProjectR/UI/Faerin/PRFaerinEncounterSubtitleWidget.h"

#include "Components/TextBlock.h"

void UPRFaerinEncounterSubtitleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 입력을 절대 가로채지 않는다(선택 버튼/이동 입력 보호).
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ClearSubtitle();
}

void UPRFaerinEncounterSubtitleWidget::SetSubtitle(const FText& InSpeakerText, const FText& InBodyText)
{
	if (SpeakerText)
	{
		SpeakerText->SetText(InSpeakerText);
	}
	if (BodyText)
	{
		BodyText->SetText(InBodyText);
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UPRFaerinEncounterSubtitleWidget::ClearSubtitle()
{
	if (SpeakerText)
	{
		SpeakerText->SetText(FText::GetEmpty());
	}
	if (BodyText)
	{
		BodyText->SetText(FText::GetEmpty());
	}

	SetVisibility(ESlateVisibility::Collapsed);
}
