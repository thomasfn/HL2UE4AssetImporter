#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"

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

};