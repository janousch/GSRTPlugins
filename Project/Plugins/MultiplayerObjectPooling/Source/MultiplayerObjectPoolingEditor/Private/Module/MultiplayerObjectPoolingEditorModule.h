#pragma once


#include "IMultiplayerObjectPoolingEditorModule.h"


class FMultiplayerObjectPoolingEditorModule : public IMultiplayerObjectPoolingEditorModule
{
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
