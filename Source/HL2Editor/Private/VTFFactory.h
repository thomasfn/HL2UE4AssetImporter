#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "Factories/Factory.h"
#include "Engine/Texture.h"
#include "UnrealEd/Classes/Factories/TextureFactory.h"

#include "VTFFactory.generated.h"

UCLASS()
class UVTFFactory : public UTextureFactory
{
	GENERATED_BODY()

public:
	UVTFFactory();
	virtual ~UVTFFactory();

	// Begin UFactory Interface

	virtual UObject* FactoryCreateBinary(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		UObject* Context,
		const TCHAR* Type,
		const uint8*& Buffer,
		const uint8* BufferEnd,
		FFeedbackContext* Warn
	) override;

	virtual bool DoesSupportClass(UClass* Class) override;

	virtual bool FactoryCanImport(const FString& Filename) override;

	// End UFactory Interface

private:

	UTexture* ImportTexture(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn);

	/** Texture settings from the automated importer that should be applied to the new texture */
	TSharedPtr<class FJsonObject> AutomatedImportSettings;

	void ApplyAutoImportSettings(UTexture* Texture);
};