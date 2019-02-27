// Copyright 2019 (C) Ram�n Janousch

#include "MultiplayerObjectPooling.h"

#define LOCTEXT_NAMESPACE "FMultiplayerObjectPoolingModule"

void FMultiplayerObjectPoolingModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FMultiplayerObjectPoolingModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMultiplayerObjectPoolingModule, MultiplayerObjectPooling)