#pragma once

#include "Modules/ModuleManager.h"
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
	const FString hl2ModelBasePath = hl2BasePath + "models/";
	const FString hl2SurfacePropBasePath = hl2BasePath + "surfaceprops/";
	const FString hl2ShaderBasePath = pluginBasePath + "Shaders/";
	const FString hl2EntityBasePath = pluginBasePath + "Entities/";

public:

	/** Begin IHL2Runtime implementation */

	virtual FString GetHL2BasePath() const override { return hl2BasePath; }
	virtual FString GetPluginBasePath() const override { return pluginBasePath; }

	virtual FString GetHL2TextureBasePath() const override { return hl2TextureBasePath; }
	virtual FString GetHL2MaterialBasePath() const override { return hl2MaterialBasePath; }
	virtual FString GetHL2ModelBasePath() const override { return hl2ModelBasePath; }
	virtual FString GetHL2SurfacePropBasePath() const override { return hl2SurfacePropBasePath; }
	virtual FString GetHL2ShaderBasePath() const override { return hl2ShaderBasePath; }
	virtual FString GetHL2EntityBasePath() const override { return hl2EntityBasePath; }

	virtual FName HL2TexturePathToAssetPath(const FString& hl2TexturePath) const override;
	virtual FName HL2MaterialPathToAssetPath(const FString& hl2MaterialPath) const override;
	virtual FName HL2ModelPathToAssetPath(const FString& hl2ModelPath) const override;
	virtual FName HL2SurfacePropToAssetPath(const FName& surfaceProp) const override;
	virtual FName HL2ShaderPathToAssetPath(const FString& hl2ShaderPath) const override;
	
	virtual UTexture* TryResolveHL2Texture(const FString& hl2TexturePath) const override;
	virtual UVMTMaterial* TryResolveHL2Material(const FString& hl2TexturePath) const override;
	virtual UStaticMesh* TryResolveHL2StaticProp(const FString& hl2ModelPath) const override;
	virtual USkeletalMesh* TryResolveHL2AnimatedProp(const FString& hl2ModelPath) const override;
	virtual USurfaceProp* TryResolveHL2SurfaceProp(const FName& surfaceProp) const override;
	virtual UMaterial* TryResolveHL2Shader(const FString& hl2ShaderPath) const override;
	
	virtual void FindAllMaterialsThatReferenceTexture(const FString& hl2TexturePath, TArray<UVMTMaterial*>& out) const override;
	virtual void FindAllMaterialsThatReferenceTexture(FName assetPath, TArray<UVMTMaterial*>& out) const override;

	/* Supports wildcards and classnames. */
	virtual void FindEntitiesByTargetName(UWorld* world, const FName targetName, TArray<ABaseEntity*>& outEntities) const override;

	/** End IHL2Runtime implementation */

};