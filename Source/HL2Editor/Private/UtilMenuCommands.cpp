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
	UI_COMMAND(BulkImportTextures, "Bulk Import Textures", "Imports all VTFs from the HL2 materials directory at once.", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(BulkImportMaterials, "Bulk Import Materials", "Imports all VMTs from the HL2 materials directory at once.", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(BulkImportModels, "Bulk Import Models", "Imports all MDLs from the HL2 models directory at once.", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(BulkImportSounds, "Bulk Import Sounds", "Imports all WAV/MP3s from the HL2 sounds directory at once.", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(ConvertSkyboxes, "Convert Skyboxes", "Converts all skybox textures into cubemaps.", EUserInterfaceActionType::Button, FInputGesture());

	UI_COMMAND(ImportBSP, "Import BSP", "Imports a BSP map to the currently open level.", EUserInterfaceActionType::Button, FInputGesture());

	UI_COMMAND(TraceTerrain, "Trace Terrain", "Attempts to map an unreal landscape to world geometry.", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE

#endif