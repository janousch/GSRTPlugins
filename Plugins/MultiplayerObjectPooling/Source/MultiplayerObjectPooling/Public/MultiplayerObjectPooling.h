// Copyright 2019 (C) Ramón Janousch

#pragma once

#include "CoreMinimal.h"
#include "ModuleManager.h"

class FMultiplayerObjectPoolingModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};