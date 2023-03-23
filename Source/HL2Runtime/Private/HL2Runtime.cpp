#include "HL2Runtime.h"

#include "Misc/Paths.h"
#include "Engine/Texture.h"
#include "VMTMaterial.h"
#include "EngineUtils.h"

void HL2RuntimeImpl::StartupModule()
{

}

void HL2RuntimeImpl::ShutdownModule()
{
	
}

FSoftObjectPath HL2RuntimeImpl::HL2TexturePathToAssetPath(const FString& hl2TexturePath) const
{
	// e.g. "Brick/brickfloor001a" -> "/Content/hl2/materials/Brick/brickfloor001a.brickfloor001a"
	return SourceToUnrealPath(GetHL2TextureBasePath(), hl2TexturePath);
}

FSoftObjectPath HL2RuntimeImpl::HL2MaterialPathToAssetPath(const FString& hl2MaterialPath) const
{
	// e.g. "Brick/brickfloor001a" -> "/Content/hl2/materials/Brick/brickfloor001a_vmt.brickfloor001a_vmt"
	return SourceToUnrealPath(GetHL2MaterialBasePath(), hl2MaterialPath);
}

FSoftObjectPath HL2RuntimeImpl::HL2ModelPathToAssetPath(const FString& hl2ModelPath) const
{
	// e.g. "models/props_borealis/mooring_cleat01.mdl" -> "/Content/hl2/models/props_borealis/mooring_cleat01.mooring_cleat01"
	FString tmp = hl2ModelPath;
	tmp.ReplaceCharInline('\\', '/');
	FPaths::MakePathRelativeTo(tmp, TEXT("models/"));
	return SourceToUnrealPath(GetHL2ModelBasePath(), tmp);
}

FSoftObjectPath HL2RuntimeImpl::HL2SoundPathToAssetPath(const FString& hl2SoundPath) const
{
	// e.g. "physics/metal/weapon_impact_hard1.wav" -> "/Content/hl2/sounds/physics/metal/weapon_impact_hard1.wav"
	return SourceToUnrealPath(GetHL2SoundBasePath(), hl2SoundPath);
}

FSoftObjectPath HL2RuntimeImpl::HL2ScriptPathToAssetPath(const FString& hl2ScriptPath) const
{
	return SourceToUnrealPath(GetHL2ScriptBasePath(), hl2ScriptPath);
}

FSoftObjectPath HL2RuntimeImpl::HL2SurfacePropToAssetPath(const FName& surfaceProp) const
{
	// e.g. "metal" -> "/Content/hl2/surfaceprops/metal.metal"
	return SourceToUnrealPath(GetHL2SurfacePropBasePath(), surfaceProp.ToString());
}

FSoftObjectPath HL2RuntimeImpl::HL2ShaderPathToAssetPath(const FString& hl2ShaderPath, bool pluginContent) const
{
	// e.g. "VertexLitGeneric" -> "/HL2Editor/Shaders/VertexLitGeneric.VertexLitGeneric" when pluginContent = true,
	//      "VertexLitGeneric" -> "/Content/hl2/Shaders/VertexLitGeneric.VertexLitGeneric" when pluginContent = false
	return SourceToUnrealPath(GetHL2ShaderBasePath(pluginContent), hl2ShaderPath);
}

UTexture* HL2RuntimeImpl::TryResolveHL2Texture(const FString& hl2TexturePath) const
{
	const FSoftObjectPath assetPath = HL2TexturePathToAssetPath(hl2TexturePath);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	const FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
	if (!assetData.IsValid()) { return nullptr; }
	return CastChecked<UTexture>(assetData.GetAsset());
}

UMaterialInterface* HL2RuntimeImpl::TryResolveHL2Material(const FString& hl2MaterialPath) const
{
	const FSoftObjectPath assetPath = HL2MaterialPathToAssetPath(hl2MaterialPath);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	const FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
	if (!assetData.IsValid()) { return nullptr; }
	return CastChecked<UMaterialInterface>(assetData.GetAsset());
}

UStaticMesh* HL2RuntimeImpl::TryResolveHL2StaticProp(const FString& hl2ModelPath) const
{
	const FSoftObjectPath assetPath = HL2ModelPathToAssetPath(hl2ModelPath);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	const FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
	if (!assetData.IsValid()) { return nullptr; }
	// It might not be a UStaticMesh if the model is animated, so let Cast just return nullptr in this case
	return Cast<UStaticMesh>(assetData.GetAsset());
}

USkeletalMesh* HL2RuntimeImpl::TryResolveHL2AnimatedProp(const FString& hl2ModelPath) const
{
	const FSoftObjectPath assetPath = HL2ModelPathToAssetPath(hl2ModelPath);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	const FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
	if (!assetData.IsValid()) { return nullptr; }
	// It might not be a USkeletalMesh if the model is not animated, so let Cast just return nullptr in this case
	return Cast<USkeletalMesh>(assetData.GetAsset());
}

USoundWave* HL2RuntimeImpl::TryResolveHL2Sound(const FString& hl2SoundPath) const
{
	const FSoftObjectPath assetPath = HL2SoundPathToAssetPath(hl2SoundPath);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	const FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
	if (!assetData.IsValid()) { return nullptr; }
	return Cast<USoundWave>(assetData.GetAsset());
}

UObject* HL2RuntimeImpl::TryResolveHL2Script(const FString& hl2ScriptPath) const
{
	const FSoftObjectPath assetPath = HL2ScriptPathToAssetPath(hl2ScriptPath);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	const FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
	if (!assetData.IsValid()) { return nullptr; }
	return assetData.GetAsset();
}

USurfaceProp* HL2RuntimeImpl::TryResolveHL2SurfaceProp(const FName& surfaceProp) const
{
	const FSoftObjectPath assetPath = HL2SurfacePropToAssetPath(surfaceProp);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	const FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
	if (!assetData.IsValid()) { return nullptr; }
	return Cast<USurfaceProp>(assetData.GetAsset());
}

UMaterial* HL2RuntimeImpl::TryResolveHL2Shader(const FString& hl2ShaderPath, bool searchGameFirst) const
{
	UObject* asset = nullptr;
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	if (searchGameFirst)
	{
		const FSoftObjectPath assetPath = HL2ShaderPathToAssetPath(hl2ShaderPath, false);
		const FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
		if (assetData.IsValid()) { asset = assetData.GetAsset(); }
	}
	if (asset == nullptr)
	{
		const FSoftObjectPath assetPath = HL2ShaderPathToAssetPath(hl2ShaderPath, true);
		const FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
		if (assetData.IsValid()) { asset = assetData.GetAsset(); }
	}
	return asset != nullptr ? CastChecked<UMaterial>(asset) : nullptr;
}

void HL2RuntimeImpl::FindAllMaterialsThatReferenceTexture(const FString& hl2TexturePath, TArray<UMaterialInterface*>& out) const
{
	FindAllMaterialsThatReferenceTexture(HL2TexturePathToAssetPath(hl2TexturePath), out);
}

void HL2RuntimeImpl::FindAllMaterialsThatReferenceTexture(FSoftObjectPath assetPath, TArray<UMaterialInterface*>& out) const
{
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	TArray<FAssetData> assets;
	assetRegistry.GetAssetsByClass(UVMTMaterial::StaticClass()->GetClassPathName(), assets);
	for (const FAssetData& assetData : assets)
	{
		UVMTMaterial* material = Cast<UVMTMaterial>(assetData.GetAsset());
		if (material && material->DoesReferenceTexture(assetPath))
		{
			out.Add(CastChecked<UMaterialInterface>(material));
		}
	}
}

void HL2RuntimeImpl::FindEntitiesByTargetName(UWorld* world, const FName targetName, TArray<ABaseEntity*>& outEntities) const
{
	// Determine if wildcard
	FString targetNameStr;
	targetName.ToString(targetNameStr);
	bool isWildcard;
	if (targetNameStr.EndsWith(TEXT("*")))
	{
		targetNameStr.RemoveAt(targetNameStr.Len() - 1);
		isWildcard = true;
	}
	else
	{
		isWildcard = false;
	}

	// Iterate all entities
	FString tmpName;
	for (TActorIterator<ABaseEntity> it(world); it; ++it)
	{
		ABaseEntity* entity = *it;

		// Handle simple equals case first
		if (entity->TargetName == targetName)
		{
			outEntities.Add(entity);
		}
		else if (entity->EntityData.Classname == targetName)
		{
			outEntities.Add(entity);
		}
		// Handle wildcard case
		else if (isWildcard)
		{
			entity->TargetName.ToString(tmpName);
			if (tmpName.Len() > targetNameStr.Len())
			{
				tmpName.RemoveAt(targetNameStr.Len(), tmpName.Len() - targetNameStr.Len());
				if (tmpName.Equals(targetNameStr, ESearchCase::IgnoreCase))
				{
					outEntities.Add(entity);
				}
			}
			
		}
	}
}

IMPLEMENT_MODULE(HL2RuntimeImpl, HL2Runtime)