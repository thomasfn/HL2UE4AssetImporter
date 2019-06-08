#pragma once

#include "CoreMinimal.h"
#include "Engine/Classes/Materials/MaterialInstanceConstant.h"

#include "VMTMaterial.generated.h"

UCLASS()
class HL2RUNTIME_API UVMTMaterial : public UMaterialInstanceConstant
{
	GENERATED_BODY()

public:
	
	bool DoesReferenceTexture(FName assetPath) const;
	void TryResolveTextures();

	virtual void PostInitProperties() override;

#if WITH_EDITORONLY_DATA
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	virtual void Serialize(FArchive& Ar) override;
#endif

	UPROPERTY()
	TMap<FName, FName> vmtTextures;

	UPROPERTY()
	TArray<FString> vmtKeywords;

	UPROPERTY()
	FString vmtSurfaceProp;

};