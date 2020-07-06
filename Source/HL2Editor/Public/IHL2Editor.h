#pragma once

#include "Modules/ModuleManager.h"
#include "HL2EditorConfig.h"

class HL2EDITOR_API IHL2Editor : public IModuleInterface
{
public:

	/**
	* Singleton-like access to this module's interface.  This is just for convenience!
	* Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	*
	* @return Returns singleton instance, loading the module on demand if needed
	*/
	static inline IHL2Editor& Get()
	{
		return FModuleManager::LoadModuleChecked<IHL2Editor>("HL2Editor");
	}

	/**
	* Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	*
	* @return True if the module is loaded and ready to use
	*/
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("HL2Editor");
	}

	virtual const FHL2EditorConfig& GetConfig() const = 0;

};