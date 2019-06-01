#pragma once

#include "ModuleManager.h"

class HL2EditorImpl : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	void StartupModule();
	void ShutdownModule();
};