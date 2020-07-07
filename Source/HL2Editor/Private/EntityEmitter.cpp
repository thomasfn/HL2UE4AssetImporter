#include "EntityEmitter.h"

#include "EditorActorFolders.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/ScopedSlowTask.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "IHL2Editor.h"
#include "IHL2Runtime.h"
#include "SourceCoord.h"
#include "AssetRegistryModule.h"
#include "Internationalization/Regex.h"
#include "HL2EntityDataUtils.h"
#include "Animation/SkeletalMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"

const FName fnLightEnv(TEXT("light_environment"));
const FName fnLight(TEXT("light"));
const FName fnLightSpot(TEXT("light_spot"));
const FName fnPropPhysics(TEXT("prop_physics"));
const FName fnPropStatic(TEXT("prop_static"));
const FName fnPropDynamic(TEXT("prop_dynamic"));
const FName fnFuncBrush(TEXT("func_brush"));
const FName fnModel(TEXT("model"));
const FName fnAngles(TEXT("angles"));
const FName fnPitch(TEXT("pitch"));
const FName fnLightColor(TEXT("_light"));
const FName fnAmbientLightColor(TEXT("_ambient"));
const FName fnAssetRegistry(TEXT("AssetRegistry"));
const FName fnEntities(TEXT("Entities"));
const FName fnHL2Entities(TEXT("HL2Entities"));

FEntityEmitter::FEntityEmitter(UWorld* world, const Valve::BSPFile& bspFile, const TArray<UStaticMesh*>& bspModels, AVBSPInfo* vbspInfo)
	: world(world), bspFile(bspFile), bspModels(bspModels), vbspInfo(vbspInfo)
{
	// TODO: Figure out how to wrap methods into TFunction without using lambdas
	portableEntityImporters.Add(fnPropPhysics, [&](const FHL2EntityData& entityData) { return ImportPortableProp(entityData); });
	portableEntityImporters.Add(fnPropStatic, [&](const FHL2EntityData& entityData) { return ImportPortableProp(entityData); });
	portableEntityImporters.Add(fnPropDynamic, [&](const FHL2EntityData& entityData) { return ImportPortableProp(entityData); });
	portableEntityImporters.Add(fnFuncBrush, [&](const FHL2EntityData& entityData) { return ImportPortableBrush(entityData); });
	portableEntityImporters.Add(fnLightEnv, [&](const FHL2EntityData& entityData) { return ImportPortableLight(entityData); });
	portableEntityImporters.Add(fnLight, [&](const FHL2EntityData& entityData) { return ImportPortableLight(entityData); });
	portableEntityImporters.Add(fnLightSpot, [&](const FHL2EntityData& entityData) { return ImportPortableLight(entityData); });
}

void FEntityEmitter::GenerateActors(const TArrayView<FHL2EntityData>& entityDatas, FScopedSlowTask* progress)
{
	const FHL2EditorBSPConfig& bspConfig = IHL2Editor::Get().GetConfig().BSP;

	const FName entitiesFolder = bspConfig.Portable ? fnEntities : fnHL2Entities;
	FActorFolders& folders = FActorFolders::Get();
	folders.CreateFolder(*world, entitiesFolder);

	GEditor->SelectNone(false, true, false);

	bool importedLightEnv = false;
	importCountMap.Empty();
	for (const FHL2EntityData& entityData : entityDatas)
	{
		progress->EnterProgressFrame();

		// Skip duplicate light_environment
		if (entityData.Classname == fnLightEnv && importedLightEnv) { continue; }

		if (bspConfig.Portable)
		{
			AActor* actor = ImportPortableEntityToWorld(entityData);
			if (actor != nullptr)
			{
				if (entityData.Classname == fnLightEnv) { importedLightEnv = true; }
				GEditor->SelectActor(actor, true, false, true, false);
			}
		}
		else
		{
			ABaseEntity* entity = ImportEntityToWorld(entityData);
			if (entity != nullptr)
			{
				GEditor->SelectActor(entity, true, false, true, false);
				if (entityData.Classname == fnLightEnv)
				{
					importedLightEnv = true;
				}
			}
		}
	}

	folders.SetSelectedFolderPath(entitiesFolder);
	GEditor->SelectNone(false, true, false);
}

ABaseEntity* FEntityEmitter::ImportEntityToWorld(const FHL2EntityData& entityData)
{
	// Resolve blueprint
	const FString assetPath = IHL2Runtime::Get().GetHL2EntityBasePath() + entityData.Classname.ToString() + TEXT(".") + entityData.Classname.ToString();
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	FAssetData assetData = assetRegistry.GetAssetByObjectPath(FName(*assetPath));
	if (!assetData.IsValid()) { return false; }
	UBlueprint* blueprint = CastChecked<UBlueprint>(assetData.GetAsset());

	// Setup transform
	FTransform transform = FTransform::Identity;
	transform.SetLocation(SourceToUnreal.Position(entityData.Origin));

	// Spawn the entity
	ABaseEntity* entity = world->SpawnActor<ABaseEntity>(blueprint->GeneratedClass, transform);
	if (entity == nullptr) { return nullptr; }

	// Set brush model on it
	FString model;
	if (entityData.TryGetString(fnModel, model))
	{
		static const FRegexPattern patternWorldModel(TEXT("^\\*([0-9]+)$"));
		FRegexMatcher matchWorldModel(patternWorldModel, model);
		if (matchWorldModel.FindNext())
		{
			int modelIndex = FCString::Atoi(*matchWorldModel.GetCaptureGroup(1));
			check(bspModels.IsValidIndex(modelIndex));
			entity->WorldModel = bspModels[modelIndex];
		}
	}

	// Run ctor on the entity
	entity->EntityData = entityData;
	entity->VBSPInfo = vbspInfo;
	if (!entityData.Targetname.IsEmpty())
	{
		entity->SetActorLabel(entityData.Targetname);
		entity->TargetName = FName(*entityData.Targetname);
	}
	entity->RerunConstructionScripts();
	entity->ResetLogicOutputs();
	entity->PostEditChange();
	entity->MarkPackageDirty();

	return entity;
}

AActor* FEntityEmitter::ImportPortableEntityToWorld(const FHL2EntityData& entityData)
{
	PortableEntityImporterFunc* importerFunc = portableEntityImporters.Find(entityData.Classname);
	if (importerFunc == nullptr) { return nullptr; }
	AActor* actor = (*importerFunc)(entityData);
	if (actor == nullptr) { return nullptr; }
	int& importCount = importCountMap.FindOrAdd(entityData.Classname, 0);
	if (!entityData.Targetname.IsEmpty())
	{
		actor->SetActorLabel(entityData.Targetname);
	}
	else
	{
		actor->SetActorLabel(FString::Printf(TEXT("%s%i"), *entityData.Classname.ToString(), importCount));
	}
	++importCount;
	actor->PostEditChange();
	actor->MarkPackageDirty();
	return actor;
}

#pragma region Portable Entity Importers

AActor* FEntityEmitter::ImportPortableProp(const FHL2EntityData& entityData)
{
	const FVector pos = SourceToUnreal.Position(entityData.Origin);
	const FRotator rot = UHL2EntityDataUtils::GetRotator(entityData, fnAngles);
	const FString model = entityData.GetString(fnModel);
	if (entityData.Classname == fnPropPhysics || entityData.Classname == fnPropDynamic)
	{
		USkeletalMesh* skeletalMesh = IHL2Runtime::Get().TryResolveHL2AnimatedProp(model);
		if (skeletalMesh != nullptr)
		{
			ASkeletalMeshActor* actor = world->SpawnActor<ASkeletalMeshActor>(pos, rot);
			if (actor == nullptr) { return nullptr; }
			USkeletalMeshComponent* skeletalMeshComponent = CastChecked<USkeletalMeshComponent>(actor->GetRootComponent());
			skeletalMeshComponent->SkeletalMesh = skeletalMesh;
			skeletalMeshComponent->PostEditChange();
			return Cast<AActor>(actor);
		}
	}
	AStaticMeshActor* actor = world->SpawnActor<AStaticMeshActor>(pos, rot);
	if (actor == nullptr) { return nullptr; }
	UStaticMesh* staticMesh = IHL2Runtime::Get().TryResolveHL2StaticProp(model);
	UStaticMeshComponent* staticMeshComponent = CastChecked<UStaticMeshComponent>(actor->GetRootComponent());
	if (staticMesh != nullptr)
	{
		staticMeshComponent->SetStaticMesh(staticMesh);
		staticMeshComponent->PostEditChange();
	}
	return Cast<AActor>(actor);
}

AActor* FEntityEmitter::ImportPortableBrush(const FHL2EntityData& entityData)
{
	const FVector pos = SourceToUnreal.Position(entityData.Origin);
	const FString model = entityData.GetString(fnModel);
	AStaticMeshActor* actor = world->SpawnActor<AStaticMeshActor>(pos, FRotator::ZeroRotator);
	if (actor == nullptr) { return nullptr; }
	UStaticMeshComponent* staticMeshComponent = CastChecked<UStaticMeshComponent>(actor->GetRootComponent());
	static const FRegexPattern patternWorldModel(TEXT("^\\*([0-9]+)$"));
	FRegexMatcher matchWorldModel(patternWorldModel, model);
	if (matchWorldModel.FindNext())
	{
		int modelIndex = FCString::Atoi(*matchWorldModel.GetCaptureGroup(1));
		check(bspModels.IsValidIndex(modelIndex));
		staticMeshComponent->SetStaticMesh(bspModels[modelIndex]);
		staticMeshComponent->PostEditChange();
	}
	return Cast<AActor>(actor);
}

AActor* FEntityEmitter::ImportPortableLight(const FHL2EntityData& entityData)
{
	const FVector pos = SourceToUnreal.Position(entityData.Origin);
	const FRotator rot = UHL2EntityDataUtils::GetRotator(entityData, fnAngles);

	if (entityData.Classname == fnLightEnv)
	{
		const FRotator sunRot(entityData.GetFloat(fnPitch), rot.Yaw, rot.Roll);
		ADirectionalLight* actor = world->SpawnActor<ADirectionalLight>(pos, rot);
		if (actor == nullptr) { return nullptr; }
		UDirectionalLightComponent* directionalLightComponent = CastChecked<UDirectionalLightComponent>(actor->GetRootComponent());
		FLinearColor color;
		float alpha;
		if (UHL2EntityDataUtils::TryGetColorAndAlpha(entityData, fnLightColor, color, alpha))
		{
			// TODO: Figure out how to convert a source light value (e.g. 200 200 200 500) to unreal units
			directionalLightComponent->SetLightColor(color);
			directionalLightComponent->SetLightBrightness(alpha);
			directionalLightComponent->PostEditChange();
		}
		return Cast<AActor>(actor);
	}

	// TODO: light and light_spot
	
	return nullptr;
}


#pragma endregion