#pragma once

#include "CoreMinimal.h"
#include "ModuleManager.h"
#include "AssetRegistryModule.h"
#include "LevelEditor.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHL2Editor, Log, All);

class UTexture;
class UVMTMaterial;
class UMaterial;

class HL2EditorImpl : public IModuleInterface
{
public:

	/** Begin IModuleInterface implementation */
	void StartupModule();
	void ShutdownModule();
	/** End IModuleInterface implementation */

private:

	bool isLoading;

	TSharedPtr<FUICommandList> utilMenuCommandList;
	TSharedPtr<FExtender> myExtender;

	FDelegateHandle handleFilesLoaded;
	FDelegateHandle handleAssetAdded;

private:

	void AddToolbarExtension(FToolBarBuilder& builder);
	static TSharedRef<SWidget> GenerateUtilityMenu(TSharedRef<FUICommandList> InCommandList);

	void HandleFilesLoaded();
	void HandleAssetAdded(const FAssetData& assetData);

	void BulkImportTexturesClicked();
	void BulkImportMaterialsClicked();
	void ConvertSkyboxes();
	void ImportBSPClicked();

	void ConvertSkybox(UTextureCube* texture, const FString& skyboxName);

	static uint64 HDRDecompress(uint32 pixel);

	static void GroupFileListByDirectory(const TArray<FString>& files, TMap<FString, TArray<FString>>& outMap);

};