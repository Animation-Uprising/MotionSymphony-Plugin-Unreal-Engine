// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "MotionSymphony.h"
#include "MotionSymphonySettings.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "ISettingsContainer.h"

#define LOCTEXT_NAMESPACE "FMotionSymphonyModule"

void FMotionSymphonyModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	RegisterSettings();
}

void FMotionSymphonyModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	if (UObjectInitialized())
	{
		UnRegisterSettings();
	}
}

bool FMotionSymphonyModule::HandleSettingsSaved()
{
	UMotionSymphonySettings* Settings = GetMutableDefault<UMotionSymphonySettings>();
	bool ResaveSettings = false;

	//Validate settings here and make changes. If so, set ResaveSettings = true

	if (ResaveSettings)
	{
		Settings->SaveConfig();
	}

	return true;
}

void FMotionSymphonyModule::RegisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		//Create a new category
		ISettingsContainerPtr SettingsContainer = SettingsModule->GetContainer("Project");

		SettingsContainer->DescribeCategory("MotionSymphonySettings",
			LOCTEXT("RuntimeWDCategoryName", "MotionSymphonySettings"),
			LOCTEXT("RuntimeWDCategoryDescription", "Game configuration for motion symphony plugin module"));

		//Register settings
		ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings("Project", "Plugins", "MotionSymphony",
			LOCTEXT("RuntimeGeneralSettingsName", "MotionSymphony"),
			LOCTEXT("RuntimeGeneratlSettingsDescription", "Base configuration for motion symphony plugin module"),
			GetMutableDefault<UMotionSymphonySettings>());

		//Register the save handler to your settings for validation checks
		if (SettingsSection.IsValid())
		{
			SettingsSection->OnModified().BindRaw(this, &FMotionSymphonyModule::HandleSettingsSaved);
		}
	}
}

void FMotionSymphonyModule::UnRegisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "MotionSymphony");
	}
}
	
IMPLEMENT_MODULE(FMotionSymphonyModule, MotionSymphony)

#undef LOCTEXT_NAMESPACE