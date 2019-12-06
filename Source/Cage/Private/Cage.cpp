// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Cage.h"
#include "Developer/Settings/Public/ISettingsModule.h"
#include "LIDAR.h"

#define LOCTEXT_NAMESPACE "FCageModule"

void FCageModule::StartupModule()
{
  ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
  if (SettingsModule != nullptr)
  {
    SettingsModule->RegisterSettings(
      "Project",
      "Cage Simulator",
      "LIDAR",
      LOCTEXT("LIDAR", "LIDAR"),
      LOCTEXT("ProjectWideLidarSettings", "Project wide LIDAR settings"),
      GetMutableDefault<UIntensityResponseParams>()
    );
  }
}

void FCageModule::ShutdownModule()
{
  ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
  if (SettingsModule != nullptr)
  {
    SettingsModule->UnregisterSettings(
      "Project",
      "Cage Simulator",
      "LIDAR"
    );
  }
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCageModule, Cage)
