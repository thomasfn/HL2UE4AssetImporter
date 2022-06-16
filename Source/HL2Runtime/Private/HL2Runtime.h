#pragma once

#include "Modules/ModuleManager.h"
#include "AssetRegistryModule.h"

#include "IHL2Runtime.h"

class UTexture;
class UMaterial;
class UMaterialInterface;

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
	const FString hl2ModelBasePath = hl2BasePath + "models/";
	const FString hl2SoundBasePath = hl2BasePath + "sounds/";
	const FString hl2ScriptBasePath = hl2BasePath + "scripts/";
	const FString hl2SurfacePropBasePath = hl2BasePath + "surfaceprops/";
	const FString hl2ShaderBasePath = hl2BasePath + "Shaders/";
	const FString hl2EntityBasePath = hl2BasePath + "Entities/";
	const FString pluginShaderBasePath = pluginBasePath + "Shaders/";
	const FString pluginEntityBasePath = pluginBasePath + "Entities/";

private:

	static inline FName SourceToUnrealPath(const FString& basePath, const FString& sourcePath);

public:

	/** Begin IHL2Runtime implementation */

	virtual const FString& GetHL2BasePath() const override { return hl2BasePath; }
	virtual const FString& GetPluginBasePath() const override { return pluginBasePath; }

	virtual const FString& GetHL2TextureBasePath() const override { return hl2TextureBasePath; }
	virtual const FString& GetHL2MaterialBasePath() const override { return hl2MaterialBasePath; }
	virtual const FString& GetHL2ModelBasePath() const override { return hl2ModelBasePath; }
	virtual const FString& GetHL2SoundBasePath() const override { return hl2SoundBasePath; }
	virtual const FString& GetHL2ScriptBasePath() const override { return hl2ScriptBasePath; }
	virtual const FString& GetHL2SurfacePropBasePath() const override { return hl2SurfacePropBasePath; }
	virtual const FString& GetHL2ShaderBasePath(bool pluginContent = true) const override { return pluginContent ? pluginShaderBasePath : hl2ShaderBasePath; }
	virtual const FString& GetHL2EntityBasePath(bool pluginContent = true) const override { return pluginContent ? pluginEntityBasePath : hl2EntityBasePath; }

	virtual FName HL2TexturePathToAssetPath(const FString& hl2TexturePath) const override;
	virtual FName HL2MaterialPathToAssetPath(const FString& hl2MaterialPath) const override;
	virtual FName HL2ModelPathToAssetPath(const FString& hl2ModelPath) const override;
	virtual FName HL2SoundPathToAssetPath(const FString& hl2SoundPath) const override;
	virtual FName HL2ScriptPathToAssetPath(const FString& hl2ScriptPath) const override;
	virtual FName HL2SurfacePropToAssetPath(const FName& surfaceProp) const override;
	virtual FName HL2ShaderPathToAssetPath(const FString& hl2ShaderPath, bool pluginContent = true) const override;
	
	virtual UTexture* TryResolveHL2Texture(const FString& hl2TexturePath) const override;
	virtual UMaterialInterface* TryResolveHL2Material(const FString& hl2TexturePath) const override;
	virtual UStaticMesh* TryResolveHL2StaticProp(const FString& hl2ModelPath) const override;
	virtual USkeletalMesh* TryResolveHL2AnimatedProp(const FString& hl2ModelPath) const override;
	virtual USoundWave* TryResolveHL2Sound(const FString& hl2SoundPath) const override;
	virtual UObject* TryResolveHL2Script(const FString& hl2ScriptPath) const override;
	virtual USurfaceProp* TryResolveHL2SurfaceProp(const FName& surfaceProp) const override;
	virtual UMaterial* TryResolveHL2Shader(const FString& hl2ShaderPath, bool searchGameFirst = true) const override;
	
	virtual void FindAllMaterialsThatReferenceTexture(const FString& hl2TexturePath, TArray<UMaterialInterface*>& out) const override;
	virtual void FindAllMaterialsThatReferenceTexture(FName assetPath, TArray<UMaterialInterface*>& out) const override;

	/* Supports wildcards and classnames. */
	virtual void FindEntitiesByTargetName(UWorld* world, const FName targetName, TArray<ABaseEntity*>& outEntities) const override;

	/** End IHL2Runtime implementation */

};

inline FName HL2RuntimeImpl::SourceToUnrealPath(const FString& basePath, const FString& sourcePath)
{
	// e.g. "path/to/asset" -> "{basePath}/path/to/asset.asset"
	FString tmp = sourcePath;
	tmp.ReplaceCharInline('\\', '/');
	return FName(*(basePath / FPaths::GetPath(tmp) / FPaths::GetBaseFilename(tmp) + '.' + FPaths::GetBaseFilename(tmp)));
}