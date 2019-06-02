#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EditorReimportHandler.h"
#include "VMTFactory.h"
#include "VMTMaterial.h"

#include "ReimportVMTFactory.generated.h"

UCLASS(collapsecategories)
class UReimportVMTFactory : public UVMTFactory, public FReimportHandler
{
	GENERATED_BODY()

public:

	UReimportVMTFactory();
	virtual ~UReimportVMTFactory();

private:

	UPROPERTY()
	class UVMTMaterial* pOriginalMat;

public:

	//~ Begin FReimportHandler Interface
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	virtual int32 GetPriority() const override;
	//~ End FReimportHandler Interface

private:
	//~ Begin UVMTFactory Interface
	virtual UVMTMaterial* CreateMaterial(UObject* InParent, FName Name, EObjectFlags Flags) override;
	//~ End UVMTFactory Interface
};