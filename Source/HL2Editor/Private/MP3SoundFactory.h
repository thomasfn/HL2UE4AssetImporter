#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "EditorReimportHandler.h"

#include "MP3SoundFactory.generated.h"

// Based on (but somewhat modified) from https://github.com/kat0r/UE4_MP3Importer

UCLASS()
class UMP3SoundFactory : public UFactory, public FReimportHandler
{
	GENERATED_BODY()

public:

	UMP3SoundFactory();
	virtual ~UMP3SoundFactory();

	// UFactory Interface
	virtual UObject* FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn) override;

	// FReimportHandler interface
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	virtual int32 GetPriority() const override;


	UNREALED_API static void SuppressImportOverwriteDialog();

private:
	/** If true, the overwrite dialog should not be shown while importing */
	static bool bSuppressImportOverwriteDialog;
};
