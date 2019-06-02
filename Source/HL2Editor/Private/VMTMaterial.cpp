#include "HL2EditorPrivatePCH.h"

#include "HL2Editor.h"
#include "VMTMaterial.h"
#include "EditorFramework/AssetImportData.h"

void UVMTMaterial::PostInitProperties()
{
#if WITH_EDITORONLY_DATA
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
#endif
	Super::PostInitProperties();
}

#if WITH_EDITORONLY_DATA
void UVMTMaterial::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	if (AssetImportData)
	{
		OutTags.Add(FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden));
	}

	Super::GetAssetRegistryTags(OutTags);
}
void UVMTMaterial::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_ASSET_IMPORT_DATA_AS_JSON && !AssetImportData)
	{
		// AssetImportData should always be valid
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
}
#endif

bool UVMTMaterial::SetFromVMT(const VTFLib::Nodes::CVMTGroupNode& groupNode)
{
	IHL2Editor& hl2Editor = IHL2Editor::Get();

	// Try resolve shader
	Parent = hl2Editor.TryResolveHL2Shader(groupNode.GetName());
	if (Parent == nullptr)
	{
		UE_LOG(LogHL2Editor, Error, TEXT("Shader '%s' not found"), groupNode.GetName());
		return false;
	}

	TArray<FMaterialParameterInfo> materialParams;
	TArray<FGuid> materialIDs;

	// Set all textures
	vmtTextures.Empty();
	Parent->GetAllTextureParameterInfo(materialParams, materialIDs);
	for (uint32 i = 0; i < groupNode.GetNodeCount(); ++i)
	{
		const auto node = groupNode.GetNode(i);
		FMaterialParameterInfo info;
		if (!GetMaterialParameterByKey(materialParams, node->GetName(), info)) { continue; }
		switch (node->GetType())
		{
		case VMTNodeType::NODE_TYPE_STRING:
		{
			FString value = ((VTFLib::Nodes::CVMTStringNode*)node)->GetValue();
			vmtTextures.Add(info.Name, hl2Editor.HL2TexturePathToAssetPath(value));
			break;
		}
		default:
			UE_LOG(LogHL2Editor, Warning, TEXT("Material key '%s' for shader '%s' was of unexpected type (expecting texture path as string)"), node->GetName(), groupNode.GetName());
			break;
		}
	}
	TryResolveTextures();

	// Set all scalars
	materialParams.Empty();
	materialIDs.Empty();
	Parent->GetAllScalarParameterInfo(materialParams, materialIDs);
	for (uint32 i = 0; i < groupNode.GetNodeCount(); ++i)
	{
		const auto node = groupNode.GetNode(i);
		FMaterialParameterInfo info;
		if (!GetMaterialParameterByKey(materialParams, node->GetName(), info)) { continue; }
		switch (node->GetType())
		{
			case VMTNodeType::NODE_TYPE_INTEGER:
			{
				int value = ((VTFLib::Nodes::CVMTIntegerNode*)node)->GetValue();
				SetScalarParameterValueEditorOnly(info, (float)value);
				break;
			}
			case VMTNodeType::NODE_TYPE_STRING:
			{
				FString value = ((VTFLib::Nodes::CVMTStringNode*)node)->GetValue();
				SetScalarParameterValueEditorOnly(info, FCString::Atof(*value));
				break;
			}
			case VMTNodeType::NODE_TYPE_SINGLE:
			{
				float value = ((VTFLib::Nodes::CVMTSingleNode*)node)->GetValue();
				SetScalarParameterValueEditorOnly(info, value);
				break;
			}
			default:
				UE_LOG(LogHL2Editor, Warning, TEXT("Material key '%s' for shader '%s' was of unexpected type (expecting some kind of scalar)"), node->GetName(), groupNode.GetName());
				break;
		}
	}

	// TODO: Deal with matrices and vectors

	// Read keywords
	vmtKeywords.Empty();
	const auto keywordsNode = groupNode.GetNode("$keywords");
	if (keywordsNode != nullptr)
	{
		if (keywordsNode->GetType() != VMTNodeType::NODE_TYPE_STRING)
		{
			UE_LOG(LogHL2Editor, Warning, TEXT("Material key '%s' was of unexpected type (expecting string)"), keywordsNode->GetName());
		}
		else
		{
			FString value = ((VTFLib::Nodes::CVMTStringNode*)keywordsNode)->GetValue();
			value.ParseIntoArray(vmtKeywords, TEXT(","), true);
			for (FString& str : vmtKeywords)
			{
				str.TrimStartAndEndInline();
			}
		}
	}

	// Read surface prop
	vmtSurfaceProp.Empty();
	const auto surfacePropNode = groupNode.GetNode("$surfaceprop");
	if (surfacePropNode != nullptr)
	{
		if (surfacePropNode->GetType() != VMTNodeType::NODE_TYPE_STRING)
		{
			UE_LOG(LogHL2Editor, Warning, TEXT("Material key '%s' was of unexpected type (expecting string)"), surfacePropNode->GetName());
		}
		else
		{
			vmtSurfaceProp = ((VTFLib::Nodes::CVMTStringNode*)surfacePropNode)->GetValue();
		}
	}

	// Shader found
	return true;
}

bool UVMTMaterial::DoesReferenceTexture(FName assetPath) const
{
	IHL2Editor& hl2Editor = IHL2Editor::Get();
	for (const auto pair : vmtTextures)
	{
		if (pair.Value == assetPath) { return true; }
	}
	return false;
}

void UVMTMaterial::TryResolveTextures()
{
	if (Parent == nullptr) { return; }
	TArray<FMaterialParameterInfo> materialParams;
	TArray<FGuid> materialIDs;
	Parent->GetAllTextureParameterInfo(materialParams, materialIDs);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	for (const auto pair : vmtTextures)
	{
		FAssetData assetData = assetRegistry.GetAssetByObjectPath(pair.Value);
		FMaterialParameterInfo info;
		if (GetMaterialParameterByKey(materialParams, pair.Key, info))
		{
			UTexture* texture = nullptr;
			if (assetData.IsValid())
			{
				texture = Cast<UTexture>(assetData.GetAsset());
			}
			if (texture == nullptr)
			{
				GetTextureParameterDefaultValue(info, texture);
			}
			SetTextureParameterValueEditorOnly(info, texture);
		}
	}
}

FName UVMTMaterial::GetVMTKeyAsParameterName(const char* key)
{
	FString keyAsString(key);
	keyAsString.RemoveFromStart("$");
	return FName(*keyAsString);
}

bool UVMTMaterial::GetMaterialParameterByKey(const TArray<FMaterialParameterInfo>& materialParams, const char* key, FMaterialParameterInfo& out)
{
	return GetMaterialParameterByKey(materialParams, GetVMTKeyAsParameterName(key), out);
}

bool UVMTMaterial::GetMaterialParameterByKey(const TArray<FMaterialParameterInfo>& materialParams, FName key, FMaterialParameterInfo& out)
{
	for (const FMaterialParameterInfo& info : materialParams)
	{
		if (info.Name == key)
		{
			out = info;
			return true;
		}
	}
	return false;
}