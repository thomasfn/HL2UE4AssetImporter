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

	// Special case: identify translucency
	const bool translucent = GetVMTKeyAsBool(groupNode, "$translucent");

	// Try resolve shader
	Parent = hl2Editor.TryResolveHL2Shader(groupNode.GetName(), translucent);
	if (Parent == nullptr)
	{
		UE_LOG(LogHL2Editor, Error, TEXT("Shader '%s' not found"), groupNode.GetName());
		return false;
	}

	// Cache all material parameters from the shader
	TArray<FMaterialParameterInfo> textureParams, scalarParams, vectorParams, staticSwitchParams;
	TArray<FGuid> textureParamIDs, scalarParamIDs, vectorParamIDs, staticSwitchParamIDs;
	Parent->GetAllTextureParameterInfo(textureParams, textureParamIDs);
	Parent->GetAllScalarParameterInfo(scalarParams, scalarParamIDs);
	Parent->GetAllVectorParameterInfo(vectorParams, vectorParamIDs);
	Parent->GetAllStaticSwitchParameterInfo(staticSwitchParams, staticSwitchParamIDs);
	FStaticParameterSet staticParamSet;

	// Iterate all vmt params and try to push them to the material
	for (uint32 i = 0; i < groupNode.GetNodeCount(); ++i)
	{
		const auto node = groupNode.GetNode(i);
		FMaterialParameterInfo info;
		FName key = GetVMTKeyAsParameterName(node->GetName());
		switch (node->GetType())
		{
			case VMTNodeType::NODE_TYPE_INTEGER:
			{
				int value = ((VTFLib::Nodes::CVMTIntegerNode*)node)->GetValue();

				// Scalar
				if (GetMaterialParameterByKey(scalarParams, key, info))
				{
					SetScalarParameterValueEditorOnly(info, (float)value);
				}
				// Static switch
				else if (GetMaterialParameterByKey(staticSwitchParams, key, info))
				{
					FStaticSwitchParameter staticSwitchParam;
					staticSwitchParam.bOverride = true;
					staticSwitchParam.ParameterInfo = info;
					staticSwitchParam.Value = value != 0;
					staticParamSet.StaticSwitchParameters.Add(staticSwitchParam);
				}
				// Vector
				else if (GetMaterialParameterByKey(vectorParams, key, info))
				{
					FLinearColor tmp;
					tmp.R = tmp.G = tmp.G = value / 255.0f;
					tmp.A = 1.0f;
					SetVectorParameterValueEditorOnly(info, tmp);
				}
				break;
			}
			case VMTNodeType::NODE_TYPE_STRING:
			{
				FString value = ((VTFLib::Nodes::CVMTStringNode*)node)->GetValue();

				// Scalar
				if (GetMaterialParameterByKey(scalarParams, key, info))
				{
					SetScalarParameterValueEditorOnly(info, FCString::Atof(*value));
				}
				// Texture
				else if (GetMaterialParameterByKey(textureParams, key, info))
				{
					vmtTextures.Add(info.Name, hl2Editor.HL2TexturePathToAssetPath(value));
					if (GetMaterialParameterByKey(staticSwitchParams, GetVMTKeyAsParameterName(node->GetName(), "_present"), info))
					{
						FStaticSwitchParameter staticSwitchParam;
						staticSwitchParam.bOverride = true;
						staticSwitchParam.ParameterInfo = info;
						staticSwitchParam.Value = true;
						staticParamSet.StaticSwitchParameters.Add(staticSwitchParam);
					}
				}
				// Static switch
				else if (GetMaterialParameterByKey(staticSwitchParams, key, info))
				{
					FStaticSwitchParameter staticSwitchParam;
					staticSwitchParam.bOverride = true;
					staticSwitchParam.ParameterInfo = info;
					staticSwitchParam.Value = value.ToBool();
					staticParamSet.StaticSwitchParameters.Add(staticSwitchParam);
				}
				// Vector
				else if (GetMaterialParameterByKey(vectorParams, key, info))
				{
					FLinearColor tmp;
					if (ParseFloatVec3(value, tmp) || ParseIntVec3(value, tmp))
					{
						SetVectorParameterValueEditorOnly(info, tmp);
					}
					else
					{
						UE_LOG(LogHL2Editor, Warning, TEXT("Material key '%s' ('%s') was of unexpected type (expecting vector)"), node->GetName(), *value);
					}
				}
				// Matrix
				else if (GetMaterialParameterByKey(vectorParams, GetVMTKeyAsParameterName(node->GetName(), "_v0"), info))
				{
					FMatrix tmp;
					if (ParseTransform(value, tmp))
					{
						SetVectorParameterValueEditorOnly(info, tmp.GetScaledAxis(EAxis::X));
						GetMaterialParameterByKey(vectorParams, GetVMTKeyAsParameterName(node->GetName(), "_v1"), info);
						SetVectorParameterValueEditorOnly(info, tmp.GetScaledAxis(EAxis::Y));
						GetMaterialParameterByKey(vectorParams, GetVMTKeyAsParameterName(node->GetName(), "_v2"), info);
						SetVectorParameterValueEditorOnly(info, tmp.GetOrigin());
					}
					else
					{
						UE_LOG(LogHL2Editor, Warning, TEXT("Material key '%s' ('%s') was of unexpected type (expecting transform)"), node->GetName(), *value);
					}
				}
				break;
			}
			case VMTNodeType::NODE_TYPE_SINGLE:
			{
				float value = ((VTFLib::Nodes::CVMTSingleNode*)node)->GetValue();

				// Scalar
				if (GetMaterialParameterByKey(scalarParams, node->GetName(), info))
				{
					SetScalarParameterValueEditorOnly(info, value);
				}
				// Vector
				else if (GetMaterialParameterByKey(staticSwitchParams, key, info))
				{
					FLinearColor tmp;
					tmp.R = tmp.G = tmp.G = value;
					tmp.A = 1.0f;
					SetVectorParameterValueEditorOnly(info, tmp);
				}
				break;
			}
		}
	}

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

	// Update textures
	TryResolveTextures();

	// Update static parameters
	UpdateStaticPermutation(staticParamSet);

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

FName UVMTMaterial::GetVMTKeyAsParameterName(const char* key, const FString& postfix)
{
	FString keyAsString(key);
	keyAsString.RemoveFromStart("$");
	keyAsString.Append(postfix);
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

bool UVMTMaterial::GetVMTKeyAsBool(const VTFLib::Nodes::CVMTGroupNode& groupNode, const char* key, bool defaultValue)
{
	const auto node = groupNode.GetNode(key);
	if (node != nullptr)
	{
		switch (node->GetType())
		{
			case VMTNodeType::NODE_TYPE_INTEGER:
			{
				int value = ((VTFLib::Nodes::CVMTIntegerNode*)node)->GetValue();
				return value != 0;
			}
			case VMTNodeType::NODE_TYPE_STRING:
			{
				FString value = ((VTFLib::Nodes::CVMTStringNode*)node)->GetValue();
				return value.ToBool();
			}
			default: return defaultValue;
		}
	}
	else
	{
		return defaultValue;
	}
}

bool UVMTMaterial::ParseFloatVec3(const FString& value, FVector& out)
{
	const static FRegexPattern patternFloatVector(TEXT("^\\s*\\[\\s*((?:\\d*\\.)?\\d+)\\s+((?:\\d*\\.)?\\d+)\\s+((?:\\d*\\.)?\\d+)\\s*\\]\\s*$"));
	FRegexMatcher matchFloatVector(patternFloatVector, value);
	if (matchFloatVector.FindNext())
	{
		out.X = FCString::Atof(*matchFloatVector.GetCaptureGroup(1));
		out.Y = FCString::Atof(*matchFloatVector.GetCaptureGroup(2));
		out.Z = FCString::Atof(*matchFloatVector.GetCaptureGroup(3));
		return true;
	}
	else
	{
		return false;
	}
}

bool UVMTMaterial::ParseFloatVec3(const FString& value, FLinearColor& out)
{
	FVector tmp;
	if (!ParseFloatVec3(value, tmp)) { return false; }
	out.R = tmp.X;
	out.G = tmp.Y;
	out.B = tmp.Z;
	out.A = 1.0f;
	return true;
}

bool UVMTMaterial::ParseIntVec3(const FString& value, FIntVector& out)
{
	const static FRegexPattern patternIntVector(TEXT("^\\s*\\{\\s*(\\d+)\\s+(\\d+)\\s+(\\d+)\\s*\\}\\s*$"));
	FRegexMatcher matchIntVector(patternIntVector, value);
	if (matchIntVector.FindNext())
	{
		out.X = FCString::Atoi(*matchIntVector.GetCaptureGroup(1));
		out.Y = FCString::Atoi(*matchIntVector.GetCaptureGroup(2));
		out.Z = FCString::Atoi(*matchIntVector.GetCaptureGroup(3));
		return true;
	}
	else
	{
		return false;
	}
}

bool UVMTMaterial::ParseIntVec3(const FString& value, FVector& out)
{
	FIntVector tmp;
	if (!ParseIntVec3(value, tmp)) { return false; }
	out.X = tmp.X;
	out.Y = tmp.Y;
	out.Z = tmp.Z;
	return true;
}

bool UVMTMaterial::ParseIntVec3(const FString& value, FLinearColor& out)
{
	FIntVector tmp;
	if (!ParseIntVec3(value, tmp)) { return false; }
	out.R = tmp.X / 255.0f;
	out.G = tmp.Y / 255.0f;
	out.B = tmp.Z / 255.0f;
	out.A = 1.0f;
	return true;
}

bool UVMTMaterial::ParseTransform(const FString& value, FMatrix& out)
{
	const static FRegexPattern patternCenter(TEXT("center\\s*((?:\\d*\\.)?\\d+)\\s+((?:\\d*\\.)?\\d+)"));
	const static FRegexPattern patternScale(TEXT("scale\\s*((?:\\d*\\.)?\\d+)\\s+((?:\\d*\\.)?\\d+)"));
	const static FRegexPattern patternRotate(TEXT("rotate\\s*((?:\\d*\\.)?\\d+)"));
	const static FRegexPattern patternTranslate(TEXT("translate\\s*((?:\\d*\\.)?\\d+)\\s+((?:\\d*\\.)?\\d+)"));
	// default: "center .5 .5 scale 1 1 rotate 0 translate 0 0"
	float cX = 0.5f, cY = 0.5f;
	float sX = 1.0f, sY = 1.0f;
	float r = 0.0f;
	float tX = 0.0f, tY = 0.0f;
	bool cF = false, sF = false, rF = false, tF = false;
	FRegexMatcher matchCenter(patternCenter, value);
	if (matchCenter.FindNext())
	{
		cX = FCString::Atof(*matchCenter.GetCaptureGroup(1));
		cY = FCString::Atof(*matchCenter.GetCaptureGroup(2));
		cF = true;
	}
	FRegexMatcher matchScale(patternScale, value);
	if (matchScale.FindNext())
	{
		sX = FCString::Atof(*matchScale.GetCaptureGroup(1));
		sY = FCString::Atof(*matchScale.GetCaptureGroup(2));
		sF = true;
	}
	FRegexMatcher matchRotate(patternRotate, value);
	if (matchRotate.FindNext())
	{
		r = FCString::Atof(*matchRotate.GetCaptureGroup(1));
		rF = true;
	}
	FRegexMatcher matchTranslate(patternTranslate, value);
	if (matchTranslate.FindNext())
	{
		tX = FCString::Atof(*matchTranslate.GetCaptureGroup(1));
		tY = FCString::Atof(*matchTranslate.GetCaptureGroup(2));
		tF = true;
	}
	if (!(cF && sF && rF && tF)) { return false; }
	const float rD = FMath::DegreesToRadians(r);
	FTransform transform = FTransform::Identity;
	FVector translation = FVector::ZeroVector;
	translation.X = (cX - 0.5f) + cX * FMath::Cos(rD) * 2.0f;
	translation.Y = (cY - 0.5f) + cY * FMath::Sin(rD) * 2.0f;
	transform.SetTranslation(translation);
	FQuat quat(FVector::UpVector, rD);
	transform.SetRotation(quat);
	FVector scale;
	scale.X = sX;
	scale.Y = sY;
	scale.Z = 1.0f;
	transform.SetScale3D(scale);
	out = transform.ToMatrixWithScale();
	return true;
}