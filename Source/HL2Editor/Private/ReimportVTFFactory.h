#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EditorReimportHandler.h"
#include "VTFFactory.h"

#include "ReimportVTFFactory.generated.h"

class UTexture2D;
class UTextureCube;

UCLASS(hidecategories = (LightMap, DitherMipMaps, LODGroup), collapsecategories)
class UReimportVTFFactory : public UVTFFactory, public FReimportHandler
{
	GENERATED_BODY()

public:

	UReimportVTFFactory();
	virtual ~UReimportVTFFactory();

private:

	UPROPERTY()
	class UTexture* pOriginalTex;

public:

	//~ Begin FReimportHandler Interface
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	virtual int32 GetPriority() const override;
	//~ End FReimportHandler Interface

private:
	//~ Begin UVTFFactory Interface
	virtual UTexture2D* CreateTexture2D(UObject* InParent, FName Name, EObjectFlags Flags) override;
	virtual UTextureCube* CreateTextureCube(UObject* InParent, FName Name, EObjectFlags Flags) override;
	//~ End UVTFFactory Interface
};