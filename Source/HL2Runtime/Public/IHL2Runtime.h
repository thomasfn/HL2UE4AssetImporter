#pragma once

#include "Modules/ModuleManager.h"
#include "Engine/Texture.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Sound/SoundWave.h"
#include "BaseEntity.h"

#include "SurfaceProp.h"
#include "Materials/MaterialInterface.h"

class HL2RUNTIME_API IHL2Runtime : public IModuleInterface
{
public:

	/**
	* Singleton-like access to this module's interface.  This is just for convenience!
	* Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	*
	* @return Returns singleton instance, loading the module on demand if needed
	*/
	static inline IHL2Runtime& Get()
	{
		return FModuleManager::LoadModuleChecked<IHL2Runtime>("HL2Runtime");
	}

	/**
	* Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	*
	* @return True if the module is loaded and ready to use
	*/
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("HL2Runtime");
	}

	virtual const FString& GetHL2BasePath() const = 0;
	virtual const FString& GetPluginBasePath() const = 0;
	
	virtual const FString& GetHL2TextureBasePath() const = 0;
	virtual const FString& GetHL2MaterialBasePath() const = 0;
	virtual const FString& GetHL2ModelBasePath() const = 0;
	virtual const FString& GetHL2SoundBasePath() const = 0;
	virtual const FString& GetHL2ScriptBasePath() const = 0;
	virtual const FString& GetHL2SurfacePropBasePath() const = 0;
	virtual const FString& GetHL2ShaderBasePath(bool pluginContent = true) const = 0;
	virtual const FString& GetHL2EntityBasePath(bool pluginContent = true) const = 0;

	virtual FSoftObjectPath HL2TexturePathToAssetPath(const FString& hl2TexturePath) const = 0;
	virtual FSoftObjectPath HL2MaterialPathToAssetPath(const FString& hl2MaterialPath) const = 0;
	virtual FSoftObjectPath HL2ModelPathToAssetPath(const FString& hl2ModelPath) const = 0;
	virtual FSoftObjectPath HL2SoundPathToAssetPath(const FString& hl2SoundPath) const = 0;
	virtual FSoftObjectPath HL2ScriptPathToAssetPath(const FString& hl2ScriptPath) const = 0;
	virtual FSoftObjectPath HL2SurfacePropToAssetPath(const FName& surfaceProp) const = 0;
	virtual FSoftObjectPath HL2ShaderPathToAssetPath(const FString& hl2ShaderPath, bool pluginContent = true) const = 0;

	virtual UTexture* TryResolveHL2Texture(const FString& hl2TexturePath) const = 0;
	virtual UMaterialInterface* TryResolveHL2Material(const FString& hl2TexturePath) const = 0;
	virtual UStaticMesh* TryResolveHL2StaticProp(const FString& hl2ModelPath) const = 0;
	virtual USkeletalMesh* TryResolveHL2AnimatedProp(const FString& hl2ModelPath) const = 0;
	virtual USoundWave* TryResolveHL2Sound(const FString& hl2SoundPath) const = 0;
	virtual UObject* TryResolveHL2Script(const FString& hl2ScriptPath) const = 0;
	virtual USurfaceProp* TryResolveHL2SurfaceProp(const FName& surfaceProp) const = 0;
	virtual UMaterial* TryResolveHL2Shader(const FString& hl2ShaderPath, bool searchGameFirst = true) const = 0;

	virtual void FindAllMaterialsThatReferenceTexture(const FString& hl2TexturePath, TArray<UMaterialInterface*>& out) const = 0;
	virtual void FindAllMaterialsThatReferenceTexture(FSoftObjectPath assetPath, TArray<UMaterialInterface*>& out) const = 0;

	virtual void FindEntitiesByTargetName(UWorld* world, const FName targetName, TArray<ABaseEntity*>& outEntities) const = 0;

};