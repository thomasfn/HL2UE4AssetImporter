// Fill out your copyright notice in the Description page of Project Settings.

#include "HL2Utils.h"

UTexture* UHL2Utils::TryResolveHL2Texture(const FString& hl2TexturePath, bool& outSuccess)
{
	UTexture* texture = IHL2Runtime::Get().TryResolveHL2Texture(hl2TexturePath);
	outSuccess = texture != nullptr;
	return texture;
}

UVMTMaterial* UHL2Utils::TryResolveHL2Material(const FString& hl2TexturePath, bool& outSuccess)
{
	UVMTMaterial* material = IHL2Runtime::Get().TryResolveHL2Material(hl2TexturePath);
	outSuccess = material != nullptr;
	return material;
}

UStaticMesh* UHL2Utils::TryResolveHL2StaticProp(const FString& hl2ModelPath, bool& outSuccess)
{
	UStaticMesh* staticMesh = IHL2Runtime::Get().TryResolveHL2StaticProp(hl2ModelPath);
	outSuccess = staticMesh != nullptr;
	return staticMesh;
}

USurfaceProp* UHL2Utils::TryResolveHL2SurfaceProp(const FName& surfaceProp, bool& outSuccess)
{
	USurfaceProp* surfacePropObj = IHL2Runtime::Get().TryResolveHL2SurfaceProp(surfaceProp);
	outSuccess = surfacePropObj != nullptr;
	return surfacePropObj;
}

UMaterial* UHL2Utils::TryResolveHL2Shader(const FString& hl2ShaderPath, EHL2BlendMode blendMode, bool& outSuccess)
{
	UMaterial* material = IHL2Runtime::Get().TryResolveHL2Shader(hl2ShaderPath, blendMode);
	outSuccess = material != nullptr;
	return material;
}

void UHL2Utils::FindEntitiesByTargetName(UObject* worldContextObject, const FName targetName, TArray<ABaseEntity*>& outEntities)
{
	IHL2Runtime::Get().FindEntitiesByTargetName(worldContextObject->GetWorld(), targetName, outEntities);
}

TArray<ABaseEntity*> UHL2Utils::GetEntitiesByTargetName(UObject* worldContextObject, const FName targetName)
{
	TArray<ABaseEntity*> result;
	FindEntitiesByTargetName(worldContextObject, targetName, result);
	return result;
}

ABaseEntity* UHL2Utils::GetEntityByTargetName(UObject* worldContextObject, const FName targetName, bool& outSuccess, bool& outMultiple)
{
	TArray<ABaseEntity*> result;
	result.Reserve(1);
	FindEntitiesByTargetName(worldContextObject, targetName, result);
	outMultiple = result.Num() > 1;
	if (outSuccess = result.Num() > 0)
	{
		return result[0];
	}
	else
	{
		return nullptr;
	}
}