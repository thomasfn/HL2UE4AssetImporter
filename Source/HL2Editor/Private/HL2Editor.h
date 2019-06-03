#pragma once

#include "ModuleManager.h"
#include "AssetRegistryModule.h"

#ifdef WITH_EDITOR
#include "LevelEditor.h"
#endif

#include "IHL2Editor.h"

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

private:

	const FString hl2BasePath = "/Game/hl2/";
	const FString pluginBasePath = "/HL2AssetImporter/";

	const FString hl2TextureBasePath = hl2BasePath + "textures/";
	const FString hl2MaterialBasePath = hl2BasePath + "materials/";
	const FString hl2ShaderBasePath = pluginBasePath + "Shaders/";

#ifdef WITH_EDITOR

	bool isLoading;

	TSharedPtr<FUICommandList> utilMenuCommandList;
	TSharedPtr<FExtender> myExtender;

	FDelegateHandle handleFilesLoaded;
	FDelegateHandle handleAssetAdded;

#endif

public:

	virtual FName HL2TexturePathToAssetPath(const FString& hl2TexturePath) const override;
	virtual FName HL2MaterialPathToAssetPath(const FString& hl2MaterialPath) const override;
	virtual FName HL2ShaderPathToAssetPath(const FString& hl2ShaderPath, bool translucent = false) const override;
	
	virtual UTexture* TryResolveHL2Texture(const FString& hl2TexturePath) const override;
	virtual UVMTMaterial* TryResolveHL2Material(const FString& hl2TexturePath) const override;
	virtual UMaterial* TryResolveHL2Shader(const FString& hl2ShaderPath, bool translucent = false) const override;
	
	virtual void FindAllMaterialsThatReferenceTexture(const FString& hl2TexturePath, TArray<UVMTMaterial*>& out) const override;
	virtual void FindAllMaterialsThatReferenceTexture(FName assetPath, TArray<UVMTMaterial*>& out) const override;

private:

#ifdef WITH_EDITOR

	void AddToolbarExtension(FToolBarBuilder& builder);
	static TSharedRef<SWidget> GenerateUtilityMenu(TSharedRef<FUICommandList> InCommandList);

	void HandleFilesLoaded();
	void HandleAssetAdded(const FAssetData& assetData);

	void BulkImportTexturesClicked();

	static void GroupFileListByDirectory(const TArray<FString>& files, TMap<FString, TArray<FString>>& outMap);

#endif

};