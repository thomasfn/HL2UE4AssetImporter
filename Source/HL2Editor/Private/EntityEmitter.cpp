#include "EntityEmitter.h"

#include "EditorActorFolders.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/ScopedSlowTask.h"
#include "Engine/World.h"
#include "IHL2Editor.h"

FEntityEmitter::FEntityEmitter(UWorld* world, const Valve::BSPFile& bspFile, const TArray<UStaticMesh*>& bspModels, AVBSPInfo* vbspInfo)
	: world(world), bspFile(bspFile), bspModels(bspModels), vbspInfo(vbspInfo)
{ }

void FEntityEmitter::GenerateActors(const TArrayView<FHL2EntityData>& entityDatas, FScopedSlowTask* progress)
{
	const FHL2EditorBSPConfig& bspConfig = IHL2Editor::Get().GetConfig().BSP;

	const FName entitiesFolder = bspConfig.Portable ? TEXT("Entities") : TEXT("HL2Entities");
	FActorFolders& folders = FActorFolders::Get();
	folders.CreateFolder(*world, entitiesFolder);

	GEditor->SelectNone(false, true, false);

	bool importedLightEnv = false;
	const static FName fnLightEnv(TEXT("light_environment"));
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
	static const FName kModel(TEXT("model"));
	FString model;
	if (entityData.TryGetString(kModel, model))
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
	return (*importerFunc)(entityData);
}