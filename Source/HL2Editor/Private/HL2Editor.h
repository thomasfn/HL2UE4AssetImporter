#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IHL2Editor.h"
#include "AssetRegistryModule.h"
#include "LevelEditor.h"
#include "Interfaces/IPluginManager.h"

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

	void* vtfLibDllHandle;

private:

	inline FString GetVTFLibDllPath() const;

	void AddToolbarExtension(FToolBarBuilder& builder);
	static TSharedRef<SWidget> GenerateUtilityMenu(TSharedRef<FUICommandList> InCommandList);

	void BulkImportTexturesClicked();
	void BulkImportMaterialsClicked();
	void BulkImportModelsClicked();
	void ConvertSkyboxes();
	void ImportBSPClicked();
	void TraceTerrainClicked();

	static void GroupFileListByDirectory(const TArray<FString>& files, TMap<FString, TArray<FString>>& outMap);

};

inline FString HL2EditorImpl::GetVTFLibDllPath() const
{
	const FString pluginBaseDir = IPluginManager::Get().FindPlugin("HL2AssetImporter")->GetBaseDir();
	const FString Win64(TEXT("Win64"));
	const FString Win32(TEXT("Win32"));
	// Note: if you get a compile error here, you're probably trying to build for linux or mac - we need to add support for these platforms here
	return pluginBaseDir / TEXT("Source/VTFLib/bin") / UBT_COMPILED_PLATFORM / TEXT("VTFLib.dll");
	return TEXT("");
}