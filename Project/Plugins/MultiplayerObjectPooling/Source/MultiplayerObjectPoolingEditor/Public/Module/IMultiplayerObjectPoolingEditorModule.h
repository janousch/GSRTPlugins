#pragma once


#include "ModuleManager.h"


class IMultiplayerObjectPoolingEditorModule : public IModuleInterface
{
public:
	
	static inline IMultiplayerObjectPoolingEditorModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IMultiplayerObjectPoolingEditorModule>("MultiplayerObjectPoolingEditor");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("MultiplayerObjectPoolingEditor");
	}
};
