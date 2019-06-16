#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"

#include "studiomdl/studiomdl.h"
#include "studiomdl/valvemeshstrip.h"
#include "studiomdl/valvevertexdata.h"
#include "studiomdl/physdata.h"

#include "MDLFactory.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMDLFactory, Log, All);

USTRUCT()
struct FImportedMDL
{
	GENERATED_BODY()

public:

	UPROPERTY()
	UStaticMesh* StaticMesh = nullptr;

	UPROPERTY()
	USkeletalMesh* SkeletalMesh = nullptr;

	UPROPERTY()
	USkeleton* Skeleton = nullptr;

	UPROPERTY()
	TArray<UAnimSequence*> Animations;

};

USTRUCT()
struct FPHYSection
{
	GENERATED_BODY()

public:

	UPROPERTY()
	int BoneIndex = -1;

	UPROPERTY()
	TArray<int32> FaceIndices;

	UPROPERTY()
	TArray<FVector> Vertices;
};

UCLASS()
class UMDLFactory : public UFactory
{
	GENERATED_BODY()

public:
	UMDLFactory();
	virtual ~UMDLFactory();

	// Begin UFactory Interface

	virtual UObject* FactoryCreateFile
	(
		UClass* inClass,
		UObject* inParent,
		FName inName,
		EObjectFlags flags,
		const FString& filename,
		const TCHAR* parms,
		FFeedbackContext* warn,
		bool& bOutOperationCanceled
	) override;

	virtual bool DoesSupportClass(UClass* Class) override;

	virtual bool FactoryCanImport(const FString& Filename) override;

	// End UFactory Interface

	virtual UStaticMesh* CreateStaticMesh(UObject* InParent, FName Name, EObjectFlags Flags);

	virtual USkeletalMesh* CreateSkeletalMesh(UObject* InParent, FName Name, EObjectFlags Flags);

	virtual USkeleton* CreateSkeleton(UObject* InParent, FName Name, EObjectFlags Flags);

	virtual UAnimSequence* CreateAnimSequence(UObject* InParent, FName Name, EObjectFlags Flags);

private:

	FImportedMDL ImportStudioModel(UClass* inClass, UObject* inParent, FName inName, EObjectFlags flags, const TCHAR* type, const uint8*& buffer, const uint8* bufferEnd, const FString& path, FFeedbackContext* warn);

	UStaticMesh* ImportStaticMesh
	(
		UObject* inParent, FName inName, EObjectFlags flags,
		const Valve::MDL::studiohdr_t& header, const Valve::VTX::FileHeader_t& vtxHeader, const Valve::VVD::vertexFileHeader_t& vvdHeader, const Valve::PHY::phyheader_t* phyHeader,
		FFeedbackContext* warn
	);

	USkeletalMesh* ImportSkeletalMesh
	(
		UObject* inParent, FName inName, UObject* inSkeletonParent, FName inSkeletonName, EObjectFlags flags,
		const Valve::MDL::studiohdr_t& header, const Valve::VTX::FileHeader_t& vtxHeader, const Valve::VVD::vertexFileHeader_t& vvdHeader, const Valve::PHY::phyheader_t* phyHeader,
		FFeedbackContext* warn
	);

	void ReadPHYSolid(uint8*& basePtr, TArray<FPHYSection>& out);
};