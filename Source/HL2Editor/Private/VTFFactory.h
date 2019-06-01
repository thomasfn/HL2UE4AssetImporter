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

	/**
	*	Tests if the given height and width specify a supported texture resolution to import; Can optionally check if the height/width are powers of two
	*
	*	@param	Width					The width of an imported texture whose validity should be checked
	*	@param	Height					The height of an imported texture whose validity should be checked
	*	@param	bAllowNonPowerOfTwo		Whether or not non-power-of-two textures are allowed
	*	@param	Warn					Where to send warnings/errors
	*
	*	@return	bool					true if the given height/width represent a supported texture resolution, false if not
	*/
	static bool IsImportResolutionValid(int32 Width, int32 Height, bool bAllowNonPowerOfTwo, FFeedbackContext* Warn);

	UTexture* ImportTexture(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn);

	/** Texture settings from the automated importer that should be applied to the new texture */
	TSharedPtr<class FJsonObject> AutomatedImportSettings;

	void ApplyAutoImportSettings(UTexture* Texture);
};