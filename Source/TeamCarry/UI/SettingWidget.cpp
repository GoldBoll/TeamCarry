// Fill out your copyright notice in the Description page of Project Settings.

#include "TeamCarry/UI/SettingWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "GameFramework/GameUserSettings.h"
#include "Blueprint/UserWidget.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"

void USettingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);

	if (ApplyButton)
	{
		ApplyButton->OnClicked.AddUniqueDynamic(this, &USettingWidget::ApplySettings);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &USettingWidget::CloseSettings);
	}
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &USettingWidget::HandleMasterVolumeChanged);
	}
}

FReply USettingWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		CloseSettings();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void USettingWidget::ApplySettings()
{
	if (UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		if (GraphicsQualityComboBox)
		{
			const int32 Quality = FMath::Clamp(GraphicsQualityComboBox->GetSelectedIndex(), 0, 4);
			UserSettings->SetOverallScalabilityLevel(Quality);
		}

		if (FullscreenCheckBox)
		{
			UserSettings->SetFullscreenMode(FullscreenCheckBox->IsChecked() ? EWindowMode::Fullscreen : EWindowMode::Windowed);
		}

		UserSettings->ApplySettings(false);
		UserSettings->SaveSettings();
	}
}

void USettingWidget::CloseSettings()
{
	OnClosed.Broadcast();
	RemoveFromParent();
}

void USettingWidget::HandleMasterVolumeChanged(float Value)
{
	OnMasterVolumeChanged.Broadcast(Value);
}