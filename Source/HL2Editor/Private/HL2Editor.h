#pragma once

#include "ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHL2Editor, Log, All);

class HL2EditorImpl : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	void StartupModule();
	void ShutdownModule();
};