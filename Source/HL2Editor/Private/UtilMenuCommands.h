#pragma once

#include "SlateBasics.h"

class FUtilMenuCommands : public TCommands<FUtilMenuCommands>
{
public:

	FUtilMenuCommands();

	static void BindGlobalStaticToInstancedActions();

#if WITH_EDITOR
	virtual void RegisterCommands() override;
#endif

	TSharedPtr<FUICommandInfo> BulkImportTextures;
	TSharedPtr<FUICommandInfo> BulkImportMaterials;
};