#include "MaterialUtils.h"
#include "IHL2Runtime.h"
#include "Internationalization/Regex.h"
#include "AssetRegistryModule.h"
#include "IHL2Editor.h"
#include "EditorAssetLibrary.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "AdvancedCopyCustomization.h"

bool FMaterialUtils::SetFromVMT(UMaterialInstanceConstant* mtl, const UValveDocument* document)
{
	IHL2Runtime& hl2Runtime = IHL2Runtime::Get();

	const UValveGroupValue* rootGroupValue = CastChecked<UValveGroupValue>(document->Root);
	check(rootGroupValue->Items.Num() == 1);
	FString shaderName = rootGroupValue->Items[0].Key.ToString();
	const UValveGroupValue* groupValue = CastChecked<UValveGroupValue>(rootGroupValue->Items[0].Value);
	UVMTMaterial* vmtMaterial = Cast<UVMTMaterial>(mtl);
	const FHL2EditorMaterialConfig& config = IHL2Editor::Get().GetConfig().Material;
	const FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	const IAssetRegistry& assetRegistry = assetRegistryModule.Get();

	// Check for subrect
	if (shaderName.Equals(TEXT("Subrect"), ESearchCase::IgnoreCase))
	{
		// TODO: This
		UE_LOG(LogHL2Editor, Warning, TEXT("Shader 'Subrect' not yet supported"));
		return false;
	}

	// Check for decal
	bool isDecal;
	const static FName fnDecal(TEXT("$decal"));
	if (!groupValue->GetBool(fnDecal, isDecal))
	{
		isDecal = false;
	}
	if (shaderName.Equals(TEXT("DecalModulate"), ESearchCase::IgnoreCase))
	{
		isDecal = true;
	}

	// Special case: handle detail sprite
	const static FName fnBaseTexture(TEXT("$basetexture"));
	bool isDetailSprites = false;
	FString baseTexture;
	if (groupValue->GetString(fnBaseTexture, baseTexture) && baseTexture == TEXT("detail/detailsprites"))
	{
		isDetailSprites = true;
	}

	// Try resolve shader
	const FString lookupShaderName =
		isDecal ? TEXT("Decal") :
		isDetailSprites ? TEXT("DetailSprites") :
		shaderName;
	UMaterialInterface* pluginShader = hl2Runtime.TryResolveHL2Shader(lookupShaderName, false);
	UMaterialInterface* gameContentShader = hl2Runtime.TryResolveHL2Shader(lookupShaderName, true);
	if (config.Portable)
	{
		if (gameContentShader != nullptr && gameContentShader != pluginShader)
		{
			mtl->Parent = gameContentShader;
		}
		else if (pluginShader != nullptr)
		{
			CopyShadersToGame();
			gameContentShader = hl2Runtime.TryResolveHL2Shader(lookupShaderName, true);
			check(gameContentShader != pluginShader);
			mtl->Parent = gameContentShader;
		}
		else
		{
			mtl->Parent = nullptr;
		}
	}
	else
	{
		mtl->Parent = pluginShader;
	}
	
	if (mtl->Parent == nullptr)
	{
		UE_LOG(LogHL2Editor, Error, TEXT("Shader '%s' not found"), *lookupShaderName);
		return false;
	}

	// Set blend mode on the material
	if (!isDetailSprites)
	{
		FMaterialInstanceBasePropertyOverrides& overrides = mtl->BasePropertyOverrides;
		const static FName fnAdditive(TEXT("$additive"));
		const static FName fnAlphaTest(TEXT("$alphatest"));
		const static FName fnTranslucent(TEXT("$translucent"));
		const static FName fnNoCull(TEXT("$nocull"));
		const static FName fnAlphaTestReference(TEXT("$alphatestreference"));
		bool tmp;
		if (groupValue->GetBool(fnAdditive, tmp) && tmp)
		{
			overrides.BlendMode = EBlendMode::BLEND_Additive;
			overrides.bOverride_BlendMode = true;
		}
		else if (groupValue->GetBool(fnAlphaTest, tmp) && tmp)
		{
			overrides.BlendMode = EBlendMode::BLEND_Masked;
			overrides.bOverride_BlendMode = true;
			float value;
			if (groupValue->GetFloat(fnAlphaTestReference, value))
			{
				overrides.OpacityMaskClipValue = value;
				overrides.bOverride_OpacityMaskClipValue = true;
			}
		}
		else if (groupValue->GetBool(fnTranslucent, tmp) && tmp)
		{
			overrides.BlendMode = EBlendMode::BLEND_Translucent;
			overrides.bOverride_BlendMode = true;
		}
		if (groupValue->GetBool(fnNoCull, tmp) && tmp)
		{
			overrides.TwoSided = true;
			overrides.bOverride_TwoSided = true;
		}
		if (isDecal && shaderName.Equals(TEXT("DecalModulate"), ESearchCase::IgnoreCase))
		{
			overrides.BlendMode = EBlendMode::BLEND_Modulate;
			overrides.bOverride_BlendMode = true;
		}
		mtl->UpdateOverridableBaseProperties();
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
	TMap<FName, FName> vmtTextures;
	for (const FValveGroupKeyValue& kv : groupValue->Items)
	{
		ProcessVMTNode(mtl, vmtTextures, textureParams, scalarParams, vectorParams, staticSwitchParams, staticParamSet, kv);
	}

	// See if there's a dx9/hdr override for us to use
	const UValveGroupValue* overrideGroupValue = nullptr;
	{
		const FName fnHDRDX9(*(shaderName + TEXT("_HDR_dx9")));
		const FName fnDX9(*(shaderName + TEXT("_DX9")));
		if ((overrideGroupValue = groupValue->GetGroup(fnHDRDX9)) != nullptr)
		{
			for (const FValveGroupKeyValue& kv : overrideGroupValue->Items)
			{
				ProcessVMTNode(mtl, vmtTextures, textureParams, scalarParams, vectorParams, staticSwitchParams, staticParamSet, kv);
			}
		}
		else if ((overrideGroupValue = groupValue->GetGroup(fnDX9)) != nullptr)
		{
			for (const FValveGroupKeyValue& kv : overrideGroupValue->Items)
			{
				ProcessVMTNode(mtl, vmtTextures, textureParams, scalarParams, vectorParams, staticSwitchParams, staticParamSet, kv);
			}
		}
	}

	// Special case: handle normalmapalphaenvmapmask
	{
		const static FName fnNormalMapAlphaEnvMapMask(TEXT("$normalmapalphaenvmapmask"));
		bool enabled;
		if ((groupValue->GetBool(fnNormalMapAlphaEnvMapMask, enabled) && enabled) || (overrideGroupValue != nullptr && overrideGroupValue->GetBool(fnNormalMapAlphaEnvMapMask, enabled) && enabled))
		{
			const static FName fnBumpMap(TEXT("bumpmap"));
			const static FName fnEnvMapMask(TEXT("envmapmask"));
			const FName* bumpMapName = vmtTextures.Find(fnBumpMap);
			FMaterialParameterInfo info;
			if (GetMaterialParameterByKey(textureParams, TEXT("envmapmask"), info) && bumpMapName != nullptr)
			{
				const FString bumpMapPath = bumpMapName->ToString();
				const FString envMapBaseName = FPaths::GetBaseFilename(bumpMapPath) + TEXT("_a");
				const FString envMapPathStr = FPaths::GetPath(bumpMapPath) / envMapBaseName + TEXT(".") + envMapBaseName;
				const FName envMapPath = vmtTextures.FindOrAdd(info.Name, NAME_None) = FName(*envMapPathStr);
				const FAssetData assetData = assetRegistry.GetAssetByObjectPath(envMapPath);
				UTexture* texture = assetData.IsValid() ? CastChecked<UTexture>(assetData.GetAsset()) : nullptr;
				if (texture == nullptr)
				{
					mtl->GetTextureParameterDefaultValue(info, texture);
				}
				if (GetMaterialParameterByKey(staticSwitchParams, GetVMTKeyAsParameterName("envmapmask_present"), info))
				{
					FStaticSwitchParameter staticSwitchParam;
					staticSwitchParam.bOverride = true;
					staticSwitchParam.ParameterInfo = info;
					staticSwitchParam.Value = true;
					staticParamSet.StaticSwitchParameters.Add(staticSwitchParam);
				}
				mtl->SetTextureParameterValueEditorOnly(info, texture);
			}
		}
	}

	// Read keywords
	if (vmtMaterial != nullptr)
	{
		vmtMaterial->vmtKeywords.Empty();
		const static FName fnKeywords(TEXT("$keywords"));
		FString keywordsRaw;
		if (groupValue->GetString(fnKeywords, keywordsRaw))
		{
			keywordsRaw.ParseIntoArray(vmtMaterial->vmtKeywords, TEXT(","), true);
			for (FString& str : vmtMaterial->vmtKeywords)
			{
				str.TrimStartAndEndInline();
			}
		}
	}

	// Read surface prop
	if (vmtMaterial != nullptr)
	{
		vmtMaterial->vmtSurfaceProp.Empty();
		const static FName fnSurfaceProp(TEXT("$surfaceprop"));
		groupValue->GetString(fnSurfaceProp, vmtMaterial->vmtSurfaceProp);
	}

	// Read detail
	if (vmtMaterial != nullptr)
	{
		const static FName fnDetailType(TEXT("%detailtype"));
		groupValue->GetString(fnDetailType, vmtMaterial->vmtDetailType);
	}

	// Update textures
	if (vmtMaterial != nullptr)
	{
		vmtMaterial->vmtTextures = vmtTextures;
	}

	// Update static parameters
	mtl->UpdateStaticPermutation(staticParamSet);

	// Shader found
	return true;
}

void FMaterialUtils::ProcessVMTNode(
	UMaterialInstanceConstant* mtl,
	TMap<FName, FName>& vmtTextures,
	const TArray<FMaterialParameterInfo>& textureParams,
	const TArray<FMaterialParameterInfo>& scalarParams,
	const TArray<FMaterialParameterInfo>& vectorParams,
	const TArray<FMaterialParameterInfo>& staticSwitchParams,
	FStaticParameterSet& staticParamSet,
	const FValveGroupKeyValue& kv
)
{
	FMaterialParameterInfo info;
	const FName key = GetVMTKeyAsParameterName(kv.Key);

	const UValvePrimitiveValue* valueAsPrim = Cast<UValvePrimitiveValue>(kv.Value);

	if (valueAsPrim != nullptr)
	{
		const FString& value = valueAsPrim->Value;

		// Scalar
		if (GetMaterialParameterByKey(scalarParams, key, info))
		{
			mtl->SetScalarParameterValueEditorOnly(info, FCString::Atof(*value));
		}
		// Texture
		else if (GetMaterialParameterByKey(textureParams, key, info))
		{
			vmtTextures.FindOrAdd(key, NAME_None) = IHL2Runtime::Get().HL2TexturePathToAssetPath(value);
			UTexture* texture = IHL2Runtime::Get().TryResolveHL2Texture(value);
			if (texture == nullptr)
			{
				mtl->GetTextureParameterDefaultValue(info, texture);
			}
			mtl->SetTextureParameterValueEditorOnly(info, texture);
			if (GetMaterialParameterByKey(staticSwitchParams, GetVMTKeyAsParameterName(kv.Key, "_present"), info))
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
				UE_LOG(LogHL2Editor, Warning, TEXT("Material key '%s' ('%s') was of unexpected type (expecting vector)"), *kv.Key.ToString(), *value);
			}
		}
		// Matrix
		else if (GetMaterialParameterByKey(vectorParams, GetVMTKeyAsParameterName(kv.Key, "_v0"), info))
		{
			FMatrix44f tmp;
			if (ParseTransform(value, tmp))
			{
				mtl->SetVectorParameterValueEditorOnly(info, tmp.GetScaledAxis(EAxis::X));
				GetMaterialParameterByKey(vectorParams, GetVMTKeyAsParameterName(kv.Key, "_v1"), info);
				mtl->SetVectorParameterValueEditorOnly(info, tmp.GetScaledAxis(EAxis::Y));
				GetMaterialParameterByKey(vectorParams, GetVMTKeyAsParameterName(kv.Key, "_v2"), info);
				mtl->SetVectorParameterValueEditorOnly(info, tmp.GetOrigin());
			}
			else
			{
				UE_LOG(LogHL2Editor, Warning, TEXT("Material key '%s' ('%s') was of unexpected type (expecting transform)"), *kv.Key.ToString(), *value);
			}
		}
	}
}

void FMaterialUtils::CopyShadersToGame()
{
	const IHL2Runtime& hl2Runtime = IHL2Runtime::Get();
	const FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	const IAssetRegistry& assetRegistry = assetRegistryModule.Get();

	const FAssetToolsModule& assetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	IAssetTools& assetTools = assetToolsModule.Get();
	
	TArray<FName> selectedPackageNames;
	{
		FARFilter filter;
		filter.bRecursivePaths = true;
		filter.ClassNames.Add(UMaterial::StaticClass()->GetFName());
		FString path = hl2Runtime.GetHL2ShaderBasePath(true);
		FPaths::NormalizeDirectoryName(path);
		filter.PackagePaths.Add(FName(*path));
		TArray<FAssetData> assetDatas;
		assetRegistry.GetAssets(filter, assetDatas);
		for (const FAssetData& assetData : assetDatas)
		{
			if (!assetData.IsValid()) { continue; }
			assetData.GetPackage()->FullyLoad();
			selectedPackageNames.Add(assetData.PackageName);
		}
	}

	FAdvancedCopyParams copyParams(selectedPackageNames, hl2Runtime.GetHL2ShaderBasePath(false));
	copyParams.bShouldSuppressUI = true;
	copyParams.bCopyOverAllDestinationOverlaps = true;

	UAdvancedCopyCustomization* copyCustomisation = UAdvancedCopyCustomization::StaticClass()->GetDefaultObject<UAdvancedCopyCustomization>();

	TArray<TMap<FString, FString>> packagesAndDestinations;
	for (const FName packageName : selectedPackageNames)
	{
		TArray<FName> packagesToCopy;
		TMap<FName, FName> depMap;
		assetTools.GetAllAdvancedCopySources(packageName, copyParams, packagesToCopy, depMap, copyCustomisation);
		TMap<FString, FString>& localPackagesAndDestinations = packagesAndDestinations.AddDefaulted_GetRef();
		for (const FName packagePath : packagesToCopy)
		{
			FString relativePath = packagePath.ToString();
			const FString objectPath = relativePath + '.' + FPaths::GetBaseFilename(relativePath);
			FPaths::MakePathRelativeTo(relativePath, *hl2Runtime.GetHL2ShaderBasePath(true));
			const FAssetData assetData = assetRegistry.GetAssetByObjectPath(FName(*objectPath));
			check(assetData.IsValid());
			if (assetData.GetClass()->IsChildOf(UTexture::StaticClass()))
			{
				// Remap this to the game's hl2 textures folder
				relativePath = packagePath.ToString();
				FPaths::MakePathRelativeTo(relativePath, TEXT("/HL2AssetImporter/Textures/"));
				localPackagesAndDestinations.Add(packagePath.ToString(), hl2Runtime.GetHL2TextureBasePath() / relativePath);
			}
			else
			{
				localPackagesAndDestinations.Add(packagePath.ToString(), hl2Runtime.GetHL2ShaderBasePath(false) / relativePath);
			}
		}
	}

	assetTools.AdvancedCopyPackages(copyParams, packagesAndDestinations);
}

FName FMaterialUtils::GetVMTKeyAsParameterName(const FName key)
{
	FString keyAsString = key.ToString();
	keyAsString.RemoveFromStart("$");
	return FName(*keyAsString);
}

FName FMaterialUtils::GetVMTKeyAsParameterName(const FName key, const FString& postfix)
{
	FString keyAsString = key.ToString();
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

bool FMaterialUtils::ParseFloatVec3(const FString& value, FVector3f& out)
{
	const static FRegexPattern patternFloatVector(TEXT("^\\s*\\[?\\s*((?:\\d*\\.)?\\d+)\\s+((?:\\d*\\.)?\\d+)\\s+((?:\\d*\\.)?\\d+)\\s*\\]?\\s*$"));
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
	FVector3f tmp;
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

bool FMaterialUtils::ParseIntVec3(const FString& value, FVector3f& out)
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

bool FMaterialUtils::ParseTransform(const FString& value, FMatrix44f& out)
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
	FTransform3f transform = FTransform3f::Identity;
	FVector3f translation = FVector3f::ZeroVector;
	translation.X = (cX - 0.5f) + cX * FMath::Cos(rD) * 2.0f;
	translation.Y = (cY - 0.5f) + cY * FMath::Sin(rD) * 2.0f;
	transform.SetTranslation(translation);
	FQuat4f quat(FVector3f::UpVector, rD);
	transform.SetRotation(quat);
	FVector3f scale;
	scale.X = sX;
	scale.Y = sY;
	scale.Z = 1.0f;
	transform.SetScale3D(scale);
	out = transform.ToMatrixWithScale();
	return true;
}