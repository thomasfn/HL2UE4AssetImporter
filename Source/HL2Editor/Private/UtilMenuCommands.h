#pragma once

#include "SlateBasics.h"

class FUtilMenuCommands : public TCommands<FUtilMenuCommands>
{
public:

	FUtilMenuCommands();

#if WITH_EDITOR
	virtual void RegisterCommands() override;
#endif

	TSharedPtr<FUICommandInfo> BulkImportTextures;
	TSharedPtr<FUICommandInfo> BulkImportMaterials;
	TSharedPtr<FUICommandInfo> BulkImportModels;
	TSharedPtr<FUICommandInfo> ConvertSkyboxes;

	TSharedPtr<FUICommandInfo> ImportBSP;

	TSharedPtr<FUICommandInfo> TraceTerrain;
};