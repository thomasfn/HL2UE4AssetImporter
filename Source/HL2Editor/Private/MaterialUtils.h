#pragma once

#include "CoreMinimal.h"
#include "VMTMaterial.h"
#include "ValveKeyValues.h"

class FMaterialUtils
{
private:

	FMaterialUtils();

public:
	
	static bool SetFromVMT(UVMTMaterial* mtl, const UValveDocument* document);

	static void TryResolveTextures(UVMTMaterial* mtl);

private:

	static inline void ProcessVMTNode(
		UVMTMaterial* mtl,
		const TArray<FMaterialParameterInfo>& textureParams,
		const TArray<FMaterialParameterInfo>& scalarParams,
		const TArray<FMaterialParameterInfo>& vectorParams,
		const TArray<FMaterialParameterInfo>& staticSwitchParams,
		FStaticParameterSet& staticParamSet,
		const FValveGroupKeyValue& kv
	);

	static FName GetVMTKeyAsParameterName(const FName key);
	static FName GetVMTKeyAsParameterName(const FName key, const FString& postfix);

	static bool GetMaterialParameterByKey(const TArray<FMaterialParameterInfo>& materialParameters, const char* key, FMaterialParameterInfo& out);
	static bool GetMaterialParameterByKey(const TArray<FMaterialParameterInfo>& materialParameters, FName key, FMaterialParameterInfo& out);

	static bool ParseFloatVec3(const FString& value, FVector& out);
	static bool ParseFloatVec3(const FString& value, FLinearColor& out);

	static bool ParseIntVec3(const FString& value, FIntVector& out);
	static bool ParseIntVec3(const FString& value, FVector& out);
	static bool ParseIntVec3(const FString& value, FLinearColor& out);

	static bool ParseTransform(const FString& value, FMatrix& out);
};