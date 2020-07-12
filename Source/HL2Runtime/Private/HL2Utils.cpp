// Fill out your copyright notice in the Description page of Project Settings.

#include "HL2Utils.h"
#include "HL2ModelData.h"

UTexture* UHL2Utils::TryResolveHL2Texture(const FString& hl2TexturePath, bool& outSuccess)
{
	UTexture* texture = IHL2Runtime::Get().TryResolveHL2Texture(hl2TexturePath);
	outSuccess = texture != nullptr;
	return texture;
}

UMaterialInterface* UHL2Utils::TryResolveHL2Material(const FString& hl2TexturePath, bool& outSuccess)
{
	UMaterialInterface* material = IHL2Runtime::Get().TryResolveHL2Material(hl2TexturePath);
	outSuccess = material != nullptr;
	return material;
}

UStaticMesh* UHL2Utils::TryResolveHL2StaticProp(const FString& hl2ModelPath, bool& outSuccess)
{
	UStaticMesh* staticMesh = IHL2Runtime::Get().TryResolveHL2StaticProp(hl2ModelPath);
	outSuccess = staticMesh != nullptr;
	return staticMesh;
}

USkeletalMesh* UHL2Utils::TryResolveHL2AnimatedProp(const FString& hl2ModelPath, bool& outSuccess)
{
	USkeletalMesh* skeletalMesh = IHL2Runtime::Get().TryResolveHL2AnimatedProp(hl2ModelPath);
	outSuccess = skeletalMesh != nullptr;
	return skeletalMesh;
}

USurfaceProp* UHL2Utils::TryResolveHL2SurfaceProp(const FName& surfaceProp, bool& outSuccess)
{
	USurfaceProp* surfacePropObj = IHL2Runtime::Get().TryResolveHL2SurfaceProp(surfaceProp);
	outSuccess = surfacePropObj != nullptr;
	return surfacePropObj;
}

UMaterial* UHL2Utils::TryResolveHL2Shader(const FString& hl2ShaderPath, bool& outSuccess)
{
	UMaterial* material = IHL2Runtime::Get().TryResolveHL2Shader(hl2ShaderPath);
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
	outSuccess = result.Num() > 0;
	if (outSuccess)
	{
		return result[0];
	}
	else
	{
		return nullptr;
	}
}

bool UHL2Utils::ApplyBodygroupToStaticMesh(UStaticMeshComponent* target, const FName bodygroupName, int index)
{
	UStaticMesh* staticMesh = target->GetStaticMesh();
	if (staticMesh == nullptr) { return false; }
	UHL2ModelData* modelData = staticMesh->GetAssetUserDataChecked<UHL2ModelData>();
	return modelData->ApplyBodygroupToStaticMesh(target, bodygroupName, index);
}

bool UHL2Utils::ApplySkinToStaticMesh(UStaticMeshComponent* target, int index)
{
	UStaticMesh* staticMesh = target->GetStaticMesh();
	if (staticMesh == nullptr) { return false; }
	UHL2ModelData* modelData = staticMesh->GetAssetUserDataChecked<UHL2ModelData>();
	return modelData->ApplySkinToStaticMesh(target, index);
}

bool UHL2Utils::ApplyBodygroupToSkeletalMesh(USkeletalMeshComponent* target, const FName bodygroupName, int index)
{
	USkeletalMesh* skeletalMesh = target->SkeletalMesh;
	if (skeletalMesh == nullptr) { return false; }
	UHL2ModelData* modelData = skeletalMesh->GetAssetUserDataChecked<UHL2ModelData>();
	return modelData->ApplyBodygroupToSkeletalMesh(target, bodygroupName, index);
}

bool UHL2Utils::ApplySkinToSkeletalMesh(USkeletalMeshComponent* target, int index)
{
	USkeletalMesh* skeletalMesh = target->SkeletalMesh;
	if (skeletalMesh == nullptr) { return false; }
	UHL2ModelData* modelData = skeletalMesh->GetAssetUserDataChecked<UHL2ModelData>();
	return modelData->ApplySkinToSkeletalMesh(target, index);
}