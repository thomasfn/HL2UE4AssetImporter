#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EditorReimportHandler.h"
#include "MDLFactory.h"

#include "ReimportMDLFactory.generated.h"

UCLASS(hidecategories = (LightMap, DitherMipMaps, LODGroup), collapsecategories)
class UReimportMDLFactory : public UMDLFactory, public FReimportHandler
{
	GENERATED_BODY()

public:

	UReimportMDLFactory();
	virtual ~UReimportMDLFactory();

private:

	UPROPERTY()
	class UStaticMesh* pOriginalStaticMesh;

	UPROPERTY()
	class USkeletalMesh* pOriginalSkeletalMesh;

	UPROPERTY()
	class USkeleton* pOriginalSkeleton;

	UPROPERTY()
	class UPhysicsAsset* pOriginalPhysicsAsset;

	UPROPERTY()
	class UAnimSequence* pOriginalAnimSequence;

public:

	//~ Begin FReimportHandler Interface
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	virtual int32 GetPriority() const override;
	//~ End FReimportHandler Interface

private:
	//~ Begin UMDLFactory Interface
	virtual UStaticMesh* CreateStaticMesh(UObject* InParent, FName Name, EObjectFlags Flags);
	virtual USkeletalMesh* CreateSkeletalMesh(UObject* InParent, FName Name, EObjectFlags Flags);
	virtual USkeleton* CreateSkeleton(UObject* InParent, FName Name, EObjectFlags Flags);
	virtual UPhysicsAsset* CreatePhysAsset(UObject* InParent, FName Name, EObjectFlags Flags);
	virtual UAnimSequence* CreateAnimSequence(UObject* InParent, FName Name, EObjectFlags Flags);
	//~ End UMDLFactory Interface
};