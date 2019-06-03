#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/Classes/Materials/MaterialInstanceConstant.h"
#include "VTFLib/VTFLib.h"
#include "Internationalization/Regex.h"

#include "VMTMaterial.generated.h"

UCLASS()
class UVMTMaterial : public UMaterialInstanceConstant
{
	GENERATED_BODY()

public:
	
	bool SetFromVMT(const VTFLib::Nodes::CVMTGroupNode& groupNode);
	bool DoesReferenceTexture(FName assetPath) const;
	void TryResolveTextures();

	virtual void PostInitProperties() override;

#if WITH_EDITORONLY_DATA
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	virtual void Serialize(FArchive& Ar) override;
#endif

private:

	UPROPERTY()
	TMap<FName, FName> vmtTextures;

	UPROPERTY()
	TArray<FString> vmtKeywords;

	UPROPERTY()
	FString vmtSurfaceProp;

	static FName GetVMTKeyAsParameterName(const char* key);
	static FName GetVMTKeyAsParameterName(const char* key, const FString& postfix);

	static bool GetMaterialParameterByKey(const TArray<FMaterialParameterInfo>& materialParameters, const char* key, FMaterialParameterInfo& out);
	static bool GetMaterialParameterByKey(const TArray<FMaterialParameterInfo>& materialParameters, FName key, FMaterialParameterInfo& out);

	static bool GetVMTKeyAsBool(const VTFLib::Nodes::CVMTGroupNode& groupNode, const char* key, bool defaultValue = false);

	static bool ParseFloatVec3(const FString& value, FVector& out);
	static bool ParseFloatVec3(const FString& value, FLinearColor& out);

	static bool ParseIntVec3(const FString& value, FIntVector& out);
	static bool ParseIntVec3(const FString& value, FVector& out);
	static bool ParseIntVec3(const FString& value, FLinearColor& out);
};