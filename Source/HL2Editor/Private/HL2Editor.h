#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IHL2Editor.h"
#include "AssetRegistryModule.h"
#include "LevelEditor.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHL2Editor, Log, All);

class UTexture;
class UVMTMaterial;
class UMaterial;

class HL2EditorImpl : public IHL2Editor
{
public:

	/** Begin IModuleInterface implementation */
	void StartupModule();
	void ShutdownModule();
	/** End IModuleInterface implementation */

	/** Begin IHL2Editor implementation */
	const FHL2EditorConfig& GetConfig() const;
	/** End IHL2Editor implementation */

private:

	TSharedPtr<FUICommandList> utilMenuCommandList;
	TSharedPtr<FExtender> myExtender;

	FHL2EditorConfig editorConfig;

private:

	void AddToolbarExtension(FToolBarBuilder& builder);
	static TSharedRef<SWidget> GenerateUtilityMenu(TSharedRef<FUICommandList> InCommandList);

	void BulkImportTexturesClicked();
	void BulkImportMaterialsClicked();
	void BulkImportModelsClicked();
	void ConvertSkyboxes();
	void ImportBSPClicked();

	static void GroupFileListByDirectory(const TArray<FString>& files, TMap<FString, TArray<FString>>& outMap);

};