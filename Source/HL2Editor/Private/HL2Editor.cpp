#include "HL2EditorPrivatePCH.h"

#include "IHL2Editor.h"
#include "HL2Editor.h"

#include "Misc/Paths.h"
#include "Engine/Texture.h"
#include "VMTMaterial.h"

DEFINE_LOG_CATEGORY(LogHL2Editor);

void HL2EditorImpl::StartupModule()
{
	isLoading = true;

	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	handleFilesLoaded = assetRegistry.OnFilesLoaded().AddRaw(this, &HL2EditorImpl::HandleFilesLoaded);
	handleAssetAdded = assetRegistry.OnAssetAdded().AddRaw(this, &HL2EditorImpl::HandleAssetAdded);
}

void HL2EditorImpl::ShutdownModule()
{
	handleAssetAdded.Reset();
	handleFilesLoaded.Reset();
}

void HL2EditorImpl::HandleFilesLoaded()
{
	isLoading = false;
	handleFilesLoaded.Reset();
}

void HL2EditorImpl::HandleAssetAdded(const FAssetData& assetData)
{
	if (isLoading) { return; }
	UE_LOG(LogHL2Editor, Log, TEXT("HandleAssetAdded: %s"), *assetData.AssetName.ToString());
}

FName HL2EditorImpl::HL2TexturePathToAssetPath(const FString& hl2TexturePath) const
{
	// e.g. "Brick/brickfloor001a" -> "/Content/hl2/materials/Brick/brickfloor001a.brickfloor001a"
	FString tmp = hl2TexturePath;
	tmp.ReplaceCharInline('\\', '/');
	return FName(*(hl2TextureBasePath + tmp + '.' + FPaths::GetCleanFilename(tmp)));
}

FName HL2EditorImpl::HL2MaterialPathToAssetPath(const FString& hl2MaterialPath) const
{
	// e.g. "Brick/brickfloor001a" -> "/Content/hl2/materials/Brick/brickfloor001a_vmt.brickfloor001a_vmt"
	FString tmp = hl2MaterialPath + hl2MaterialPostfix;
	tmp.ReplaceCharInline('\\', '/');
	return FName(*(hl2TextureBasePath + tmp + '.' + FPaths::GetCleanFilename(tmp)));
}

FName HL2EditorImpl::HL2ShaderPathToAssetPath(const FString& hl2ShaderPath) const
{
	// e.g. "VertexLitGeneric" -> "/HL2Editor/Shaders/VertexLitGeneric.VertexLitGeneric"
	FString tmp = hl2ShaderPath;
	tmp.ReplaceCharInline('\\', '/');
	return FName(*(hl2ShaderBasePath + tmp + '.' + FPaths::GetCleanFilename(tmp)));
}

UTexture* HL2EditorImpl::TryResolveHL2Texture(const FString& hl2TexturePath) const
{
	FName assetPath = HL2TexturePathToAssetPath(hl2TexturePath);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
	if (!assetData.IsValid()) { return nullptr; }
	return CastChecked<UTexture>(assetData.GetAsset());
}

UVMTMaterial* HL2EditorImpl::TryResolveHL2Material(const FString& hl2MaterialPath) const
{
	FName assetPath = HL2MaterialPathToAssetPath(hl2MaterialPath);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
	if (!assetData.IsValid()) { return nullptr; }
	return CastChecked<UVMTMaterial>(assetData.GetAsset());
}

UMaterial* HL2EditorImpl::TryResolveHL2Shader(const FString& hl2ShaderPath) const
{
	FName assetPath = HL2ShaderPathToAssetPath(hl2ShaderPath);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
	if (!assetData.IsValid()) { return nullptr; }
	return CastChecked<UMaterial>(assetData.GetAsset());
}

void HL2EditorImpl::FindAllMaterialsThatReferenceTexture(const FString& hl2TexturePath, TArray<UVMTMaterial*>& out) const
{
	FName assetPath = HL2TexturePathToAssetPath(hl2TexturePath);
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

IMPLEMENT_MODULE(HL2EditorImpl, HL2Editor)