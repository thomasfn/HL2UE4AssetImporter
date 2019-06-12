#include "HL2EditorPrivatePCH.h"

#include "MaterialUtils.h"

bool FMaterialUtils::SetFromVMT(UVMTMaterial* mtl, const VTFLib::Nodes::CVMTGroupNode& groupNode)
{
	IHL2Runtime& hl2Runtime = IHL2Runtime::Get();

	// Special case: identify translucency
	const bool translucent = GetVMTKeyAsBool(groupNode, "$translucent");
	const bool alphatest = GetVMTKeyAsBool(groupNode, "$alphatest");
	const bool additive = GetVMTKeyAsBool(groupNode, "$additive");

	// Try resolve shader
	mtl->Parent = hl2Runtime.TryResolveHL2Shader(groupNode.GetName(), additive ? EHL2BlendMode::Additive : alphatest ? EHL2BlendMode::AlphaTest : translucent ? EHL2BlendMode::Translucent : EHL2BlendMode::Opaque);
	if (mtl->Parent == nullptr)
	{
		UE_LOG(LogHL2Editor, Error, TEXT("Shader '%s' not found"), groupNode.GetName());
		return false;
	}

	// Cache all material parameters from the shader
	TArray<FMaterialParameterInfo> textureParams, scalarParams, vectorParams, staticSwitchParams;
	TArray<FGuid> textureParamIDs, scalarParamIDs, vectorParamIDs, staticSwitchParamIDs;
	mtl->Parent->GetAllTextureParameterInfo(textureParams, textureParamIDs);
	mtl->Parent->GetAllScalarParameterInfo(scalarParams, scalarParamIDs);
	mtl->Parent->GetAllVectorParameterInfo(vectorParams, vectorParamIDs);
	mtl->Parent->GetAllStaticSwitchParameterInfo(staticSwitchParams, staticSwitchParamIDs);
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
					mtl->SetScalarParameterValueEditorOnly(info, (float)value);
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
					mtl->SetVectorParameterValueEditorOnly(info, tmp);
				}
				break;
			}
			case VMTNodeType::NODE_TYPE_STRING:
			{
				FString value = ((VTFLib::Nodes::CVMTStringNode*)node)->GetValue();

				// Scalar
				if (GetMaterialParameterByKey(scalarParams, key, info))
				{
					mtl->SetScalarParameterValueEditorOnly(info, FCString::Atof(*value));
				}
				// Texture
				else if (GetMaterialParameterByKey(textureParams, key, info))
				{
					mtl->vmtTextures.Add(info.Name, hl2Runtime.HL2TexturePathToAssetPath(value));
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
						mtl->SetVectorParameterValueEditorOnly(info, tmp);
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
						mtl->SetVectorParameterValueEditorOnly(info, tmp.GetScaledAxis(EAxis::X));
						GetMaterialParameterByKey(vectorParams, GetVMTKeyAsParameterName(node->GetName(), "_v1"), info);
						mtl->SetVectorParameterValueEditorOnly(info, tmp.GetScaledAxis(EAxis::Y));
						GetMaterialParameterByKey(vectorParams, GetVMTKeyAsParameterName(node->GetName(), "_v2"), info);
						mtl->SetVectorParameterValueEditorOnly(info, tmp.GetOrigin());
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
					mtl->SetScalarParameterValueEditorOnly(info, value);
				}
				// Vector
				else if (GetMaterialParameterByKey(staticSwitchParams, key, info))
				{
					FLinearColor tmp;
					tmp.R = tmp.G = tmp.G = value;
					tmp.A = 1.0f;
					mtl->SetVectorParameterValueEditorOnly(info, tmp);
				}
				break;
			}
		}
	}

	// Read keywords
	mtl->vmtKeywords.Empty();
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
			value.ParseIntoArray(mtl->vmtKeywords, TEXT(","), true);
			for (FString& str : mtl->vmtKeywords)
			{
				str.TrimStartAndEndInline();
			}
		}
	}

	// Read surface prop
	mtl->vmtSurfaceProp.Empty();
	const auto surfacePropNode = groupNode.GetNode("$surfaceprop");
	if (surfacePropNode != nullptr)
	{
		if (surfacePropNode->GetType() != VMTNodeType::NODE_TYPE_STRING)
		{
			UE_LOG(LogHL2Editor, Warning, TEXT("Material key '%s' was of unexpected type (expecting string)"), surfacePropNode->GetName());
		}
		else
		{
			mtl->vmtSurfaceProp = ((VTFLib::Nodes::CVMTStringNode*)surfacePropNode)->GetValue();
		}
	}

	// Update textures
	TryResolveTextures(mtl);

	// Update static parameters
	mtl->UpdateStaticPermutation(staticParamSet);

	// Shader found
	return true;
}

void FMaterialUtils::TryResolveTextures(UVMTMaterial* mtl)
{
	if (mtl->Parent == nullptr) { return; }
	TArray<FMaterialParameterInfo> materialParams;
	TArray<FGuid> materialIDs;
	mtl->Parent->GetAllTextureParameterInfo(materialParams, materialIDs);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	for (const auto pair : mtl->vmtTextures)
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
				mtl->GetTextureParameterDefaultValue(info, texture);
			}
			mtl->SetTextureParameterValueEditorOnly(info, texture);
		}
	}
}

FName FMaterialUtils::GetVMTKeyAsParameterName(const char* key)
{
	FString keyAsString(key);
	keyAsString.RemoveFromStart("$");
	return FName(*keyAsString);
}

FName FMaterialUtils::GetVMTKeyAsParameterName(const char* key, const FString& postfix)
{
	FString keyAsString(key);
	keyAsString.RemoveFromStart("$");
	keyAsString.Append(postfix);
	return FName(*keyAsString);
}

bool FMaterialUtils::GetMaterialParameterByKey(const TArray<FMaterialParameterInfo>& materialParams, const char* key, FMaterialParameterInfo& out)
{
	return GetMaterialParameterByKey(materialParams, GetVMTKeyAsParameterName(key), out);
}

bool FMaterialUtils::GetMaterialParameterByKey(const TArray<FMaterialParameterInfo>& materialParams, FName key, FMaterialParameterInfo& out)
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

bool FMaterialUtils::GetVMTKeyAsBool(const VTFLib::Nodes::CVMTGroupNode& groupNode, const char* key, bool defaultValue)
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

bool FMaterialUtils::ParseFloatVec3(const FString& value, FVector& out)
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

bool FMaterialUtils::ParseFloatVec3(const FString& value, FLinearColor& out)
{
	FVector tmp;
	if (!ParseFloatVec3(value, tmp)) { return false; }
	out.R = tmp.X;
	out.G = tmp.Y;
	out.B = tmp.Z;
	out.A = 1.0f;
	return true;
}

bool FMaterialUtils::ParseIntVec3(const FString& value, FIntVector& out)
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

bool FMaterialUtils::ParseIntVec3(const FString& value, FVector& out)
{
	FIntVector tmp;
	if (!ParseIntVec3(value, tmp)) { return false; }
	out.X = tmp.X;
	out.Y = tmp.Y;
	out.Z = tmp.Z;
	return true;
}

bool FMaterialUtils::ParseIntVec3(const FString& value, FLinearColor& out)
{
	FIntVector tmp;
	if (!ParseIntVec3(value, tmp)) { return false; }
	out.R = tmp.X / 255.0f;
	out.G = tmp.Y / 255.0f;
	out.B = tmp.Z / 255.0f;
	out.A = 1.0f;
	return true;
}

bool FMaterialUtils::ParseTransform(const FString& value, FMatrix& out)
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