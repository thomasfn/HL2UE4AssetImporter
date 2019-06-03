#pragma once

#include "UtilMenuCommands.h"
#include "LevelEditor.h"

FUtilMenuCommands::FUtilMenuCommands()
	: TCommands<FUtilMenuCommands>(TEXT("HL2AssetImporterPlugin"), NSLOCTEXT("Contexts", "HL2AssetImporterPlugin", "HL2AssetImporterPlugin Plugin"), NAME_None, FEditorStyle::GetStyleSetName())
{

}

#ifdef WITH_EDITOR

#define LOCTEXT_NAMESPACE ""

void FUtilMenuCommands::RegisterCommands()
{
	UI_COMMAND(BulkImportTextures, "Bulk Import Textures", "Imports multiple VTFs at once.", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE

#endif