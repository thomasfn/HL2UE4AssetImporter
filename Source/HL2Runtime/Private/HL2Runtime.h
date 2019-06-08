#pragma once

#include "ModuleManager.h"
#include "AssetRegistryModule.h"

#include "IHL2Runtime.h"

class UTexture;
class UVMTMaterial;
class UMaterial;

class HL2RuntimeImpl : public IHL2Runtime
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

public:

	/** Begin IHL2Runtime implementation */

	virtual FString GetHL2BasePath() const override { return hl2BasePath; }
	virtual FString GetPluginBasePath() const override { return pluginBasePath; }

	virtual FString GetHL2TextureBasePath() const override { return hl2TextureBasePath; }
	virtual FString GetHL2MaterialBasePath() const override { return hl2MaterialBasePath; }
	virtual FString GetHL2ShaderBasePath() const override { return hl2ShaderBasePath; }

	virtual FName HL2TexturePathToAssetPath(const FString& hl2TexturePath) const override;
	virtual FName HL2MaterialPathToAssetPath(const FString& hl2MaterialPath) const override;
	virtual FName HL2ShaderPathToAssetPath(const FString& hl2ShaderPath, EHL2BlendMode blendMode = EHL2BlendMode::Opaque) const override;
	
	virtual UTexture* TryResolveHL2Texture(const FString& hl2TexturePath) const override;
	virtual UVMTMaterial* TryResolveHL2Material(const FString& hl2TexturePath) const override;
	virtual UMaterial* TryResolveHL2Shader(const FString& hl2ShaderPath, EHL2BlendMode blendMode = EHL2BlendMode::Opaque) const override;
	
	virtual void FindAllMaterialsThatReferenceTexture(const FString& hl2TexturePath, TArray<UVMTMaterial*>& out) const override;
	virtual void FindAllMaterialsThatReferenceTexture(FName assetPath, TArray<UVMTMaterial*>& out) const override;

	/** End IHL2Runtime implementation */

};