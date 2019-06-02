#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/Classes/Materials/MaterialInstanceDynamic.h"
#include "VTFLib/VMTGroupNode.h"

#include "VMTMaterial.generated.h"

UCLASS()
class UVMTMaterial : public UMaterialInstanceDynamic
{
	GENERATED_BODY()

public:
	
	bool SetFromVMT(const VTFLib::Nodes::CVMTGroupNode& groupNode);
	bool DoesReferenceTexture(FName assetPath) const;
	void TryResolveTextures();

private:

	UPROPERTY()
	TMap<FName, FString> vmtTextures;

	UPROPERTY()
	TArray<FString> vmtKeywords;

	UPROPERTY()
	FString vmtSurfaceProp;

	static FName GetVMTKeyAsParameterName(const char* key);

	static bool GetMaterialParameterByKey(const TArray<FMaterialParameterInfo>& materialParameters, const char* key, FMaterialParameterInfo& out);
	static bool GetMaterialParameterByKey(const TArray<FMaterialParameterInfo>& materialParameters, FName key, FMaterialParameterInfo& out);

};