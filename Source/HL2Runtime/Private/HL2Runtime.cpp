#include "HL2RuntimePrivatePCH.h"

#include "HL2Runtime.h"

#include "Misc/Paths.h"
#include "Engine/Texture.h"
#include "VMTMaterial.h"

void HL2RuntimeImpl::StartupModule()
{

}

void HL2RuntimeImpl::ShutdownModule()
{
	
}

FName HL2RuntimeImpl::HL2TexturePathToAssetPath(const FString& hl2TexturePath) const
{
	// e.g. "Brick/brickfloor001a" -> "/Content/hl2/materials/Brick/brickfloor001a.brickfloor001a"
	FString tmp = hl2TexturePath;
	tmp.ReplaceCharInline('\\', '/');
	return FName(*(hl2TextureBasePath + tmp + '.' + FPaths::GetCleanFilename(tmp)));
}

FName HL2RuntimeImpl::HL2MaterialPathToAssetPath(const FString& hl2MaterialPath) const
{
	// e.g. "Brick/brickfloor001a" -> "/Content/hl2/materials/Brick/brickfloor001a_vmt.brickfloor001a_vmt"
	FString tmp = hl2MaterialPath;
	tmp.ReplaceCharInline('\\', '/');
	return FName(*(hl2MaterialBasePath + tmp + '.' + FPaths::GetCleanFilename(tmp)));
}

FName HL2RuntimeImpl::HL2ModelPathToAssetPath(const FString& hl2ModelPath) const
{
	// e.g. "props_borealis/mooring_cleat01" -> "/Content/hl2/models/props_borealis/mooring_cleat01.mooring_cleat01"
	FString tmp = hl2ModelPath;
	tmp.ReplaceCharInline('\\', '/');
	return FName(*(hl2ModelPath + tmp + '.' + FPaths::GetCleanFilename(tmp)));
}

FName HL2RuntimeImpl::HL2SurfacePropToAssetPath(const FName& surfaceProp) const
{
	// e.g. "metal" -> "/Content/hl2/surfaceprops/metal.metal"
	return FName(*(hl2SurfacePropBasePath + surfaceProp.ToString() + '.' + surfaceProp.ToString()));
}

FName HL2RuntimeImpl::HL2ShaderPathToAssetPath(const FString& hl2ShaderPath, EHL2BlendMode blendMode) const
{
	// e.g. "VertexLitGeneric" -> "/HL2Editor/Shaders/VertexLitGeneric.VertexLitGeneric"
	FString tmp = hl2ShaderPath;
	tmp.ReplaceCharInline('\\', '/');
	switch (blendMode)
	{
		case EHL2BlendMode::Translucent: tmp = tmp.Append("_translucent"); break;
		case EHL2BlendMode::AlphaTest: tmp = tmp.Append("_alphatest"); break;
	}
	return FName(*(hl2ShaderBasePath + tmp + '.' + FPaths::GetCleanFilename(tmp)));
}

UTexture* HL2RuntimeImpl::TryResolveHL2Texture(const FString& hl2TexturePath) const
{
	FName assetPath = HL2TexturePathToAssetPath(hl2TexturePath);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
	if (!assetData.IsValid()) { return nullptr; }
	return CastChecked<UTexture>(assetData.GetAsset());
}

UVMTMaterial* HL2RuntimeImpl::TryResolveHL2Material(const FString& hl2MaterialPath) const
{
	FName assetPath = HL2MaterialPathToAssetPath(hl2MaterialPath);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
	if (!assetData.IsValid()) { return nullptr; }
	return CastChecked<UVMTMaterial>(assetData.GetAsset());
}

UStaticMesh* HL2RuntimeImpl::TryResolveHL2StaticProp(const FString& hl2ModelPath) const
{
	FName assetPath = HL2ModelPathToAssetPath(hl2ModelPath);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
	if (!assetData.IsValid()) { return nullptr; }
	// It might not be a UStaticMesh if the model is animated, so let Cast just return nullptr in this case
	return Cast<UStaticMesh>(assetData.GetAsset());
}

USurfaceProp* HL2RuntimeImpl::TryResolveHL2SurfaceProp(const FName& surfaceProp) const
{
	FName assetPath = HL2SurfacePropToAssetPath(surfaceProp);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
	if (!assetData.IsValid()) { return nullptr; }
	return Cast<USurfaceProp>(assetData.GetAsset());
}

UMaterial* HL2RuntimeImpl::TryResolveHL2Shader(const FString& hl2ShaderPath, EHL2BlendMode blendMode) const
{
	FName assetPath = HL2ShaderPathToAssetPath(hl2ShaderPath, blendMode);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
	if (!assetData.IsValid())
	{
		// Fall back to opaque
		if (blendMode == EHL2BlendMode::Opaque) { return nullptr; }
		return TryResolveHL2Shader(hl2ShaderBasePath);
	}
	return CastChecked<UMaterial>(assetData.GetAsset());
}

void HL2RuntimeImpl::FindAllMaterialsThatReferenceTexture(const FString& hl2TexturePath, TArray<UVMTMaterial*>& out) const
{
	FindAllMaterialsThatReferenceTexture(HL2TexturePathToAssetPath(hl2TexturePath), out);
}

void HL2RuntimeImpl::FindAllMaterialsThatReferenceTexture(FName assetPath, TArray<UVMTMaterial*>& out) const
{
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	TArray<FAssetData> assets;
	assetRegistry.GetAssetsByClass(UVMTMaterial::StaticClass()->GetFName(), assets);
	for (const FAssetData& assetData : assets)
	{
		UVMTMaterial* material = Cast<UVMTMaterial>(assetData.GetAsset());
		if (material && material->DoesReferenceTexture(assetPath))
		{
			out.Add(material);
		}
	}
}

IMPLEMENT_MODULE(HL2RuntimeImpl, HL2Runtime)