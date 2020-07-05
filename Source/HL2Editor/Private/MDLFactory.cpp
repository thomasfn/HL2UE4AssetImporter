#include "MDLFactory.h"

#include "EditorFramework/AssetImportData.h"
#include "Misc/FeedbackContext.h"
#include "AssetImportTask.h"
#include "Misc/FileHelper.h"
#include "IHL2Runtime.h"
#include "MeshUtils.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "Rendering/SkeletalMeshModel.h"
#include "HL2ModelData.h"
#include "MeshAttributes.h"
#include "StaticMeshAttributes.h"
#include "MeshUtilities.h"
#include "MeshDescriptionOperations.h"
#include "MeshUtilitiesCommon.h"
#include "OverlappingCorners.h"
#include "ValveKeyValues.h"
#include "SkeletonUtils.h"
#include "IMeshBuilderModule.h"
#include "Engine/SkeletalMeshSocket.h"

DEFINE_LOG_CATEGORY(LogMDLFactory);

UMDLFactory::UMDLFactory()
{
	// We only want to create assets by importing files.
	// Set this to true if you want to be able to create new, empty Assets from
	// the editor.
	bCreateNew = false;

	// Our Asset will be imported from a binary file
	bText = false;

	// Allows us to actually use the "Import" button in the Editor for this Asset
	bEditorImport = true;

	// Tells the Editor which file types we can import
	Formats.Add(TEXT("mdl;Valve Model Files"));

	// Tells the Editor which Asset type this UFactory can import
	SupportedClass = UStaticMesh::StaticClass();
}

UMDLFactory::~UMDLFactory()
{
	
}

// Begin UFactory Interface

UObject* UMDLFactory::FactoryCreateFile(
	UClass* inClass,
	UObject* inParent,
	FName inName,
	EObjectFlags flags,
	const FString& filename,
	const TCHAR* parms,
	FFeedbackContext* warn,
	bool& bOutOperationCanceled
)
{
	UAssetImportTask* task = AssetImportTask;
	if (task == nullptr)
	{
		task = NewObject<UAssetImportTask>();
		task->Filename = filename;
	}

	if (ScriptFactoryCreateFile(task))
	{
		return task->Result[0];
	}

	FString fileExt = FPaths::GetExtension(filename);

	// load as binary
	TArray<uint8> data;
	if (!FFileHelper::LoadFileToArray(data, *filename))
	{
		warn->Logf(ELogVerbosity::Error, TEXT("Failed to load file '%s' to array"), *filename);
		return nullptr;
	}

	data.Add(0);
	ParseParms(parms);
	const uint8* buffer = &data[0];

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, inClass, inParent, inName, *fileExt);

	FString path = FPaths::GetPath(filename);
	FImportedMDL result = ImportStudioModel(inClass, inParent, inName, flags, *fileExt, buffer, buffer + data.Num(), path, warn);

	if (!result.StaticMesh && !result.SkeletalMesh)
	{
		warn->Logf(ELogVerbosity::Error, TEXT("Studiomodel import failed"));
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
		return nullptr;
	}

	if (result.StaticMesh != nullptr)
	{
		result.StaticMesh->AssetImportData->Update(CurrentFilename, FileHash.IsValid() ? &FileHash : nullptr);
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, result.StaticMesh);
		result.StaticMesh->PostEditChange();
	}
	if (result.SkeletalMesh != nullptr)
	{
		result.SkeletalMesh->AssetImportData->Update(CurrentFilename, FileHash.IsValid() ? &FileHash : nullptr);
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, result.SkeletalMesh);
		result.SkeletalMesh->PostEditChange();
	}
	if (result.Skeleton != nullptr)
	{
		// result.Skeleton->AssetImportData->Update(CurrentFilename, FileHash.IsValid() ? &FileHash : nullptr);
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, result.Skeleton);
		result.Skeleton->PostEditChange();
	}
	for (UAnimSequence* animSequence : result.Animations)
	{
		animSequence->AssetImportData->Update(CurrentFilename, FileHash.IsValid() ? &FileHash : nullptr);
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, animSequence);
		animSequence->PostEditChange();
	}
	
	return result.StaticMesh != nullptr ? (UObject*)result.StaticMesh : (UObject*)result.SkeletalMesh;
}

/** Returns whether or not the given class is supported by this factory. */
bool UMDLFactory::DoesSupportClass(UClass* Class)
{
	return Class == UStaticMesh::StaticClass() ||
		Class == USkeletalMesh::StaticClass() ||
		Class == USkeleton::StaticClass() ||
		Class == UAnimSequence::StaticClass();
}

/** Returns true if this factory can deal with the file sent in. */
bool UMDLFactory::FactoryCanImport(const FString& Filename)
{
	FString Extension = FPaths::GetExtension(Filename);
	return Extension.Compare("mdl", ESearchCase::IgnoreCase) == 0;
}

// End UFactory Interface

UStaticMesh* UMDLFactory::CreateStaticMesh(UObject* InParent, FName Name, EObjectFlags Flags)
{
	UObject* newObject = CreateOrOverwriteAsset(UStaticMesh::StaticClass(), InParent, Name, Flags);
	UStaticMesh* newStaticMesh = nullptr;
	if (newObject)
	{
		newStaticMesh = CastChecked<UStaticMesh>(newObject);
	}

	return newStaticMesh;
}

USkeletalMesh* UMDLFactory::CreateSkeletalMesh(UObject* InParent, FName Name, EObjectFlags Flags)
{
	UObject* newObject = CreateOrOverwriteAsset(USkeletalMesh::StaticClass(), InParent, Name, Flags);
	USkeletalMesh* newSkeletalMesh = nullptr;
	if (newObject)
	{
		newSkeletalMesh = CastChecked<USkeletalMesh>(newObject);
	}

	return newSkeletalMesh;
}

USkeleton* UMDLFactory::CreateSkeleton(UObject* InParent, FName Name, EObjectFlags Flags)
{
	UObject* newObject = CreateOrOverwriteAsset(USkeleton::StaticClass(), InParent, Name, Flags);
	USkeleton* newSkeleton = nullptr;
	if (newObject)
	{
		newSkeleton = CastChecked<USkeleton>(newObject);
	}

	return newSkeleton;
}

UPhysicsAsset* UMDLFactory::CreatePhysAsset(UObject* InParent, FName Name, EObjectFlags Flags)
{
	UObject* newObject = CreateOrOverwriteAsset(UPhysicsAsset::StaticClass(), InParent, Name, Flags);
	UPhysicsAsset* newPhysAsset = nullptr;
	if (newObject)
	{
		newPhysAsset = CastChecked<UPhysicsAsset>(newObject);
	}

	return newPhysAsset;
}

UAnimSequence* UMDLFactory::CreateAnimSequence(UObject* InParent, FName Name, EObjectFlags Flags)
{
	UObject* newObject = CreateOrOverwriteAsset(UAnimSequence::StaticClass(), InParent, Name, Flags);
	UAnimSequence* newAnimSequence = nullptr;
	if (newObject)
	{
		newAnimSequence = CastChecked<UAnimSequence>(newObject);
	}
	newAnimSequence->CleanAnimSequenceForImport();
	return newAnimSequence;
}

FImportedMDL UMDLFactory::ImportStudioModel(UClass* inClass, UObject* inParent, FName inName, EObjectFlags flags, const TCHAR* type, const uint8*& buffer, const uint8* bufferEnd, const FString& path, FFeedbackContext* warn)
{
	FImportedMDL result;
	const Valve::MDL::studiohdr_t& header = *((Valve::MDL::studiohdr_t*)buffer);
	
	// IDST
	if (header.id != 0x54534449)
	{
		warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: Header has invalid ID (expecting 'IDST')"));
		return result;
	}

	// Version
	if (header.version < 44 || header.version > 48)
	{
		warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: MDL Header has invalid version (expecting 44-48, got %d)"), header.version);
		return result;
	}

	// Load vtx
	FString baseFileName = FPaths::GetBaseFilename(FString(header.name));
	TArray<uint8> vtxData;
	if (!FFileHelper::LoadFileToArray(vtxData, *(path / baseFileName + TEXT(".vtx"))))
	{
		if (!FFileHelper::LoadFileToArray(vtxData, *(path / baseFileName + TEXT(".dx90.vtx"))))
		{
			if (!FFileHelper::LoadFileToArray(vtxData, *(path / baseFileName + TEXT(".dx80.vtx"))))
			{
				warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: Could not find vtx file"));
				return result;
			}
		}
	}
	const Valve::VTX::FileHeader_t& vtxHeader = *((Valve::VTX::FileHeader_t*)&vtxData[0]);
	if (vtxHeader.version != 7)
	{
		warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: VTX Header has invalid version (expecting 7, got %d)"), vtxHeader.version);
		return result;
	}
	if (vtxHeader.checkSum != header.checksum)
	{
		warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: VTX Header has invalid checksum (expecting %d, got %d)"), header.checksum, vtxHeader.checkSum);
		return result;
	}

	// Load vvd
	TArray<uint8> vvdData;
	if (!FFileHelper::LoadFileToArray(vvdData, *(path / baseFileName + TEXT(".vvd"))))
	{
		warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: Could not find vvd file"));
		return result;
	}
	const Valve::VVD::vertexFileHeader_t& vvdHeader = *((Valve::VVD::vertexFileHeader_t*)&vvdData[0]);
	if (vvdHeader.version != 4)
	{
		warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: VVD Header has invalid version (expecting 4, got %d)"), vvdHeader.version);
		return result;
	}
	if (vvdHeader.checksum != header.checksum)
	{
		warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: VVD Header has invalid checksum (expecting %d, got %d)"), header.checksum, vvdHeader.checksum);
		return result;
	}

	// Load phy
	TArray<uint8> phyData;
	Valve::PHY::phyheader_t* phyHeader;
	if (FFileHelper::LoadFileToArray(phyData, *(path / baseFileName + TEXT(".phy"))))
	{
		phyHeader = (Valve::PHY::phyheader_t*)&phyData[0];
		/*if (phyHeader->checkSum != header.checksum)
		{
			warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: PHY Header has invalid checksum (expecting %d, got %d)"), header.checksum, phyHeader->checksum);
			return result;
		}*/
	}
	else
	{
		phyHeader = nullptr;
	}

	// Load ani
	TArray<uint8> aniData;
	Valve::MDL::studiohdr_t* aniHeader;
	if (FFileHelper::LoadFileToArray(aniData, *(path / baseFileName + TEXT(".ani"))))
	{
		aniHeader = (Valve::MDL::studiohdr_t*)&aniData[0];

		// IDAG
		if (aniHeader->id != 0x47414449)
		{
			warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: ANI header has invalid ID (expecting 'IDAG')"));
			return result;
		}

		// Version
		if (aniHeader->version < 44 || aniHeader->version > 48)
		{
			warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: ANI header has invalid version (expecting 44-48, got %d)"), aniHeader->version);
			return result;
		}
	}
	else
	{
		aniHeader = nullptr;
	}

	// Static mesh
	if (header.HasFlag(Valve::MDL::studiohdr_flag::STATIC_PROP))
	{
		result.StaticMesh = ImportStaticMesh(inParent, inName, flags, header, vtxHeader, vvdHeader, phyHeader, warn);
		return result;
	}

	// Skeletal mesh + skeleton
	FString skeletonPackagePath = inParent->GetPathName();
	skeletonPackagePath.Append(TEXT("_skeleton"));
	const FName skeletonPackageName(*(FPaths::GetBaseFilename(skeletonPackagePath)));
	UPackage* skeletonPackage = CreatePackage(nullptr, *skeletonPackagePath);
	FString physAssetPackagePath = inParent->GetPathName();
	physAssetPackagePath.Append(TEXT("_physics"));
	const FName physAssetPackageName(*(FPaths::GetBaseFilename(physAssetPackagePath)));
	UPackage* physAssetPackage = phyHeader != nullptr ? CreatePackage(nullptr, *physAssetPackagePath) : nullptr;
	result.SkeletalMesh = ImportSkeletalMesh(inParent, inName, skeletonPackage, skeletonPackageName, physAssetPackage, physAssetPackageName, flags, header, vtxHeader, vvdHeader, phyHeader, warn);
	if (result.SkeletalMesh == nullptr)
	{
		warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: Failed to import skeletal mesh"));
		skeletonPackage->MarkPendingKill();
		return result;
	}
	result.SkeletalMesh->InvalidateDeriveDataCacheGUID();
	result.SkeletalMesh->PostEditChange();
	FAssetRegistryModule::AssetCreated(result.SkeletalMesh);
	result.SkeletalMesh->MarkPackageDirty();
	result.Skeleton = result.SkeletalMesh->Skeleton;
	result.Skeleton->PostEditChange();
	FAssetRegistryModule::AssetCreated(result.Skeleton);
	result.Skeleton->MarkPackageDirty();
	result.PhysicsAsset = result.SkeletalMesh->PhysicsAsset;
	if (result.PhysicsAsset != nullptr)
	{
		result.PhysicsAsset->PostEditChange();
		FAssetRegistryModule::AssetCreated(result.PhysicsAsset);
		result.PhysicsAsset->MarkPackageDirty();
	}

	// Sequences
	FString animPackagePath = inParent->GetPathName();
	animPackagePath.Append(TEXT("_anims/"));
	ImportSequences(header, result.SkeletalMesh, animPackagePath, aniHeader, result.Animations, warn);

	// Includes
	TArray<const Valve::MDL::mstudiomodelgroup_t*> includes;
	header.GetIncludeModels(includes);
	for (const Valve::MDL::mstudiomodelgroup_t* include : includes)
	{
		ImportInclude(inParent, *include, path, result, warn);
	}

	for (UAnimSequence* sequence : result.Animations)
	{
		sequence->PostEditChange();
		FAssetRegistryModule::AssetCreated(sequence);
		sequence->MarkPackageDirty();
	}

	return result;
}

void UMDLFactory::ImportInclude(UObject* inParent, const Valve::MDL::mstudiomodelgroup_t& include, const FString& basePath, FImportedMDL& result, FFeedbackContext* warn)
{
	const FString includeName = include.GetName();
	const FString fileExt = FPaths::GetExtension(includeName);
	if (fileExt != TEXT("mdl"))
	{
		warn->Logf(ELogVerbosity::Error, TEXT("ImportInclude: Failed to load file '%s' (unknown extension)"), *includeName);
		return;
	}
	if (includeName.Contains(TEXT("gestures")) || includeName.Contains(TEXT("postures")))
	{
		warn->Logf(ELogVerbosity::Warning, TEXT("ImportInclude: Refusing to load '%s' (not yet properly supported)"), *includeName);
		return;
	}
	// includeName will be something like "models/blah/blah_animations.mdl"
	// so we need to follow basePath up until we reach models
	FString fileName = basePath;
	FPaths::NormalizeDirectoryName(fileName);
	do {
		FString left, right;
		fileName.Split(TEXT("/"), &left, &right, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		right.TrimStartAndEndInline();
		if (right.Equals(TEXT("models"), ESearchCase::IgnoreCase))
		{
			break;
		}
		fileName = left;
	} while (fileName.Contains(TEXT("/")));
	fileName = fileName / TEXT("..") / includeName;

	// load as binary
	TArray<uint8> data;
	if (!FFileHelper::LoadFileToArray(data, *fileName))
	{
		warn->Logf(ELogVerbosity::Error, TEXT("ImportInclude: Failed to load file '%s'"), *fileName);
		return;
	}

	const Valve::MDL::studiohdr_t& header = *((Valve::MDL::studiohdr_t*)data.GetData());

	// IDST
	if (header.id != 0x54534449)
	{
		warn->Logf(ELogVerbosity::Error, TEXT("ImportInclude: Header has invalid ID (expecting 'IDST')"));
		return;
	}

	// Version
	if (header.version < 44 || header.version > 48)
	{
		warn->Logf(ELogVerbosity::Error, TEXT("ImportInclude: MDL Header has invalid version (expecting 44-48, got %d)"), header.version);
		return;
	}

	// Load ani
	TArray<uint8> aniData;
	Valve::MDL::studiohdr_t* aniHeader;
	if (FFileHelper::LoadFileToArray(aniData, *(FPaths::GetPath(fileName) / FPaths::GetBaseFilename(fileName) + TEXT(".ani"))))
	{
		aniHeader = (Valve::MDL::studiohdr_t*) & aniData[0];

		// IDAG
		if (aniHeader->id != 0x47414449)
		{
			warn->Logf(ELogVerbosity::Error, TEXT("ImportInclude: ANI header has invalid ID (expecting 'IDAG')"));
			return;
		}

		// Version
		if (aniHeader->version < 44 || aniHeader->version > 48)
		{
			warn->Logf(ELogVerbosity::Error, TEXT("ImportInclude: ANI header has invalid version (expecting 44-48, got %d)"), aniHeader->version);
			return;
		}
	}
	else
	{
		aniHeader = nullptr;
	}

	// Sequences
	FString animPackagePath = inParent->GetPathName();
	animPackagePath.Append(TEXT("_anims/"));
	ImportSequences(header, result.SkeletalMesh, animPackagePath, aniHeader, result.Animations, warn);
}

UStaticMesh* UMDLFactory::ImportStaticMesh
(
	UObject* inParent, FName inName, EObjectFlags flags,
	const Valve::MDL::studiohdr_t& header, const Valve::VTX::FileHeader_t& vtxHeader, const Valve::VVD::vertexFileHeader_t& vvdHeader, const Valve::PHY::phyheader_t* phyHeader,
	FFeedbackContext* warn
)
{
	// Read and validate body parts
	TArray<const Valve::MDL::mstudiobodyparts_t*> bodyParts;
	header.GetBodyParts(bodyParts);
	TArray<const Valve::VTX::BodyPartHeader_t*> vtxBodyParts;
	vtxHeader.GetBodyParts(vtxBodyParts);
	if (bodyParts.Num() != vtxBodyParts.Num())
	{
		warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: Body parts in vtx didn't match body parts in mdl"));
		return nullptr;
	}

	// Create all mesh descriptions
	struct LocalMeshData
	{
		FMeshDescription meshDescription;
		TMap<FVector, FVertexID> vertexMap;
		TMap<uint16, FVertexInstanceID> vertexInstanceMap;

		TPolygonGroupAttributesRef<FName> polyGroupMaterial;
		TVertexAttributesRef<FVector> vertPos;
		TVertexInstanceAttributesRef<FVector> vertInstNormal;
		TVertexInstanceAttributesRef<FVector> vertInstTangent;
		TVertexInstanceAttributesRef<FVector2D> vertInstUV0;

		LocalMeshData()
		{
			FStaticMeshAttributes staticMeshAttr(meshDescription);
			staticMeshAttr.Register();
			polyGroupMaterial = staticMeshAttr.GetPolygonGroupMaterialSlotNames();
			vertPos = staticMeshAttr.GetVertexPositions();
			vertInstNormal = staticMeshAttr.GetVertexInstanceNormals();
			vertInstTangent = staticMeshAttr.GetVertexInstanceTangents();
			vertInstUV0 = staticMeshAttr.GetVertexInstanceUVs();
		}
	};
	TArray<LocalMeshData> localMeshDatas;
	localMeshDatas.AddDefaulted(vtxHeader.numLODs);

	// Read materials
	TArray<FName> materials;
	ResolveMaterials(header, materials);
	struct LocalSectionData
	{
		int materialIndex;
		FName sectionName;
	};
	TArray<LocalSectionData> localSectionDatas;
	TMap<FName, FModelBodygroup> bodygroups;

	// Iterate all body parts
	uint16 accumIndex = 0;
	for (int bodyPartIndex = 0; bodyPartIndex < bodyParts.Num(); ++bodyPartIndex)
	{
		const Valve::MDL::mstudiobodyparts_t& bodyPart = *bodyParts[bodyPartIndex];
		const Valve::VTX::BodyPartHeader_t& vtxBodyPart = *vtxBodyParts[bodyPartIndex];

		FModelBodygroup bodygroup;

		// Read and validate models
		TArray<const Valve::MDL::mstudiomodel_t*> models;
		bodyPart.GetModels(models);
		TArray<const Valve::VTX::ModelHeader_t*> vtxModels;
		vtxBodyPart.GetModels(vtxModels);
		if (models.Num() != vtxModels.Num())
		{
			warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: Models in vtx didn't match models in mdl for body part %d"), bodyPartIndex);
			return nullptr;
		}

		// Iterate all models
		for (int modelIndex = 0; modelIndex < models.Num(); ++modelIndex)
		{
			const Valve::MDL::mstudiomodel_t& model = *models[modelIndex];
			const Valve::VTX::ModelHeader_t& vtxModel = *vtxModels[modelIndex];
			TMap<int, int> materialToSectionMap;

			FModelBodygroupMapping bodygroupMapping;

			// Read and validate meshes & lods
			TArray<const Valve::MDL::mstudiomesh_t*> meshes;
			model.GetMeshes(meshes);
			TArray<const Valve::VTX::ModelLODHeader_t*> lods;
			vtxModel.GetLODs(lods);
			if (lods.Num() != vtxHeader.numLODs)
			{
				warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: Model %d in vtx didn't have correct lod count for body part %d"), modelIndex, bodyPartIndex);
				return nullptr;
			}

			// Iterate all lods
			for (int lodIndex = 0; lodIndex < lods.Num(); ++lodIndex)
			{
				const Valve::VTX::ModelLODHeader_t& lod = *lods[lodIndex];

				// Fetch the local mesh data for this lod
				LocalMeshData& localMeshData = localMeshDatas[lodIndex];

				// Iterate all vertices in the vvd for this lod and insert them
				TArray<Valve::VVD::mstudiovertex_t> vvdVertices;
				TArray<FVector4> vvdTangents;
				{
					const uint16 vertCount = (uint16)vvdHeader.numLODVertexes[0];
					vvdVertices.Reserve(vertCount);
					vvdTangents.Reserve(vertCount);

					if (vvdHeader.numFixups > 0)
					{
						TArray<Valve::VVD::mstudiovertex_t> fixupVertices;
						TArray<FVector4> fixupTangents;
						TArray<Valve::VVD::vertexFileFixup_t> fixups;
						vvdHeader.GetFixups(fixups);
						for (const Valve::VVD::vertexFileFixup_t& fixup : fixups)
						{
							//if (fixup.lod >= lodIndex)
							{
								for (int i = 0; i < fixup.numVertexes; ++i)
								{
									Valve::VVD::mstudiovertex_t vertex;
									FVector4 tangent;
									vvdHeader.GetVertex(fixup.sourceVertexID + i, vertex, tangent);
									vvdVertices.Add(vertex);
									vvdTangents.Add(tangent);
								}
							}
						}
					}
					else
					{
						for (uint16 i = 0; i < vertCount; ++i)
						{
							Valve::VVD::mstudiovertex_t vertex;
							FVector4 tangent;
							vvdHeader.GetVertex(i, vertex, tangent);
							vvdVertices.Add(vertex);
							vvdTangents.Add(tangent);
						}
					}

					// Write to mesh and store in map
					/*for (uint16 i = 0; i < vertCount; ++i)
					{
						const Valve::VVD::mstudiovertex_t& vertex = vertices[i];
						const FVector4& tangent = tangents[i];
						FVertexID vertID;
						if (localMeshData.vertexMap.Contains(vertex.m_vecPosition))
						{
							vertID = localMeshData.vertexMap[vertex.m_vecPosition];
						}
						else
						{
							vertID = localMeshData.meshDescription.CreateVertex();
							localMeshData.vertPos[vertID] = vertex.m_vecPosition;
							localMeshData.vertexMap.Add(vertex.m_vecPosition, vertID);
						}
						FVertexInstanceID vertInstID = localMeshData.meshDescription.CreateVertexInstance(vertID);
						localMeshData.vertInstNormal[vertInstID] = vertex.m_vecNormal;
						localMeshData.vertInstTangent[vertInstID] = FVector(tangent.X, tangent.Y, tangent.Z);
						localMeshData.vertInstUV0[vertInstID] = vertex.m_vecTexCoord;
						localMeshData.vertexInstanceMap.Add(i, vertInstID);
					}*/
				}

				// Fetch and iterate all meshes
				TArray<const Valve::VTX::MeshHeader_t*> vtxMeshes;
				lod.GetMeshes(vtxMeshes);
				for (int meshIndex = 0; meshIndex < meshes.Num(); ++meshIndex)
				{
					const Valve::MDL::mstudiomesh_t& mesh = *meshes[meshIndex];
					const Valve::VTX::MeshHeader_t& vtxMesh = *vtxMeshes[meshIndex];

					// Fetch and validate material
					if (!materials.IsValidIndex(mesh.material))
					{
						warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: Mesh %d in model %d had invalid material index %d (%d materials available)"), meshIndex, modelIndex, mesh.material, materials.Num());
						return nullptr;
					}

					// Allocate a section index for us
					int sectionIndex;
					{
						const int* sectionIndexPtr = materialToSectionMap.Find(mesh.material);
						if (sectionIndexPtr == nullptr)
						{
							LocalSectionData localSectionData;
							localSectionData.materialIndex = mesh.material;
							localSectionData.sectionName = FName(*FString::Printf(TEXT("%s:%d:%s"), *bodyPart.GetName(), modelIndex, *materials[mesh.material].ToString()));
							sectionIndex = localSectionDatas.Add(localSectionData);
							materialToSectionMap.Add(mesh.material, sectionIndex);
						}
						else
						{
							sectionIndex = *sectionIndexPtr;
						}
					}

					// Add to the current bodygroup mapping
					bodygroup.AllSections.Add(sectionIndex);
					bodygroupMapping.Sections.Add(sectionIndex);
					
					// Create poly group
					const FPolygonGroupID polyGroupID = localMeshData.meshDescription.CreatePolygonGroup();
					localMeshData.polyGroupMaterial[polyGroupID] = localSectionDatas[sectionIndex].sectionName;

					// Fetch and iterate all strip groups
					TArray<const Valve::VTX::StripGroupHeader_t*> stripGroups;
					vtxMesh.GetStripGroups(stripGroups);
					for (int stripGroupIndex = 0; stripGroupIndex < stripGroups.Num(); ++stripGroupIndex)
					{
						const Valve::VTX::StripGroupHeader_t& stripGroup = *stripGroups[stripGroupIndex];

						TArray<uint16> indices;
						stripGroup.GetIndices(indices);

						TArray<Valve::VTX::Vertex_t> vertices;
						stripGroup.GetVertices(vertices);

						TArray<const Valve::VTX::StripHeader_t*> strips;
						stripGroup.GetStrips(strips);

						TArray<FVertexInstanceID> tmpVertInstIDs;
						for (int stripIndex = 0; stripIndex < strips.Num(); ++stripIndex)
						{
							const Valve::VTX::StripHeader_t& strip = *strips[stripIndex];

							for (int i = 0; i < strip.numIndices; i += 3)
							{
								tmpVertInstIDs.Empty(3);

								uint16 idxs[3];
								if (StudioMdlToUnreal.ShouldReverseWinding())
								{
									idxs[0] = indices[strip.indexOffset + i + 2];
									idxs[1] = indices[strip.indexOffset + i + 1];
									idxs[2] = indices[strip.indexOffset + i];
								}
								else
								{
									idxs[0] = indices[strip.indexOffset + i];
									idxs[1] = indices[strip.indexOffset + i + 1];
									idxs[2] = indices[strip.indexOffset + i + 2];
								}

								const Valve::VTX::Vertex_t* verts[3] =
								{
									&vertices[idxs[0]],
									&vertices[idxs[1]],
									&vertices[idxs[2]]
								};

								const uint16 baseIdxs[3] =
								{
									accumIndex + mesh.vertexoffset + verts[0]->origMeshVertID,
									accumIndex + mesh.vertexoffset + verts[1]->origMeshVertID,
									accumIndex + mesh.vertexoffset + verts[2]->origMeshVertID
								};

								for (int j = 0; j < 3; ++j)
								{
									const Valve::VVD::mstudiovertex_t& vvdVertex = vvdVertices[baseIdxs[j]];
									const FVector4& vvdTangent = vvdTangents[baseIdxs[j]];

									FVertexID vertID = localMeshData.meshDescription.CreateVertex();
									localMeshData.vertPos[vertID] = StudioMdlToUnreal.Position(vvdVertex.m_vecPosition);

									FVertexInstanceID vertInstID = localMeshData.meshDescription.CreateVertexInstance(vertID);
									localMeshData.vertInstNormal[vertInstID] = StudioMdlToUnreal.Direction(vvdVertex.m_vecNormal);
									localMeshData.vertInstTangent[vertInstID] = StudioMdlToUnreal.Direction(FVector(vvdTangent.X, vvdTangent.Y, vvdTangent.Z));
									localMeshData.vertInstUV0[vertInstID] = vvdVertex.m_vecTexCoord;

									tmpVertInstIDs.Add(vertInstID);
								}

								//tmpVertInstIDs.Add(localMeshData.vertexInstanceMap[baseIdx0]);
								//tmpVertInstIDs.Add(localMeshData.vertexInstanceMap[baseIdx1]);
								//tmpVertInstIDs.Add(localMeshData.vertexInstanceMap[baseIdx2]);

								// Sanity check: if one or more vertex instances share the same base vertex, this poly is degenerate so reject it
								//const FVertexID baseVert0ID = localMeshData.meshDescription.GetVertexInstanceVertex(tmpVertInstIDs[0]);
								//const FVertexID baseVert1ID = localMeshData.meshDescription.GetVertexInstanceVertex(tmpVertInstIDs[1]);
								//const FVertexID baseVert2ID = localMeshData.meshDescription.GetVertexInstanceVertex(tmpVertInstIDs[2]);
								//if (baseVert0ID == baseVert1ID || baseVert0ID == baseVert2ID || baseVert1ID == baseVert2ID) { continue; }

								FPolygonID polyID = localMeshData.meshDescription.CreatePolygon(polyGroupID, tmpVertInstIDs);
							}
						}
					}
				}
			}

			bodygroup.Mappings.Add(bodygroupMapping);

			accumIndex += model.numvertices;
		}

		bodygroups.Add(FName(*bodyPart.GetName()), bodygroup);
	}

	// Construct static mesh
	UStaticMesh* staticMesh = CreateStaticMesh(inParent, inName, flags);
	if (staticMesh == nullptr) { return nullptr; }
	staticMesh->LightMapResolution = 64;

	// Create model data
	UHL2ModelData* modelData = NewObject<UHL2ModelData>(staticMesh);
	modelData->Bodygroups = bodygroups;
	staticMesh->AddAssetUserData(modelData);

	constexpr bool debugPhysics = false; // if true, the physics mesh is rendered instead

	// Write all lods to the static mesh
	if (!debugPhysics || phyHeader == nullptr)
	{
		for (int sectionIdx = 0; sectionIdx < localSectionDatas.Num(); ++sectionIdx)
		{
			const LocalSectionData& localSectionData = localSectionDatas[sectionIdx];
			const FName materialName = materials[localSectionData.materialIndex];
			FStaticMaterial material;
			material.ImportedMaterialSlotName = localSectionData.sectionName;
			material.MaterialSlotName = localSectionData.sectionName;
			material.MaterialInterface = IHL2Runtime::Get().TryResolveHL2Material(materialName.ToString());
			staticMesh->StaticMaterials.Add(material);
		}
		for (int lodIndex = 0; lodIndex < vtxHeader.numLODs; ++lodIndex)
		{
			// Setup source model
			FStaticMeshSourceModel& sourceModel = staticMesh->AddSourceModel();
			FMeshBuildSettings& settings = sourceModel.BuildSettings;
			settings.bRecomputeNormals = false;
			settings.bRecomputeTangents = false;
			settings.bGenerateLightmapUVs = false;
			settings.SrcLightmapIndex = 0;
			settings.DstLightmapIndex = 1;
			settings.bRemoveDegenerates = false;
			settings.bUseFullPrecisionUVs = true;
			settings.MinLightmapResolution = 64;
			sourceModel.ScreenSize.Default = FMath::Pow(0.5f, lodIndex + 1.0f);

			// Fetch and prepare our mesh description
			FMeshDescription& localMeshDescription = localMeshDatas[lodIndex].meshDescription;
			FMeshUtils::Clean(localMeshDescription);

			// Generate lightmap coords
			{
				localMeshDescription.VertexInstanceAttributes().SetAttributeIndexCount<FVector2D>(MeshAttribute::VertexInstance::TextureCoordinate, 2);
				FOverlappingCorners overlappingCorners;
				FStaticMeshOperations::FindOverlappingCorners(overlappingCorners, localMeshDescription, 0.00001f);
				FStaticMeshOperations::CreateLightMapUVLayout(localMeshDescription, settings.SrcLightmapIndex, settings.DstLightmapIndex, settings.MinLightmapResolution, ELightmapUVVersion::Latest, overlappingCorners);
			}

			// Clean again but this time weld - we do weld after lightmap uv layout because welded vertices sometimes break that algorithm for whatever reason
			{
				//FMeshCleanSettings cleanSettings = FMeshCleanSettings::Default;
				//cleanSettings.WeldVertices = true;
				//FMeshUtils::Clean(localMeshDescription, cleanSettings);
			}

			// Copy to static mesh
			FMeshDescription* meshDescription = staticMesh->CreateMeshDescription(lodIndex);
			*meshDescription = localMeshDescription;
			staticMesh->CommitMeshDescription(lodIndex);

			// Assign materials
			for (int sectionIdx = 0; sectionIdx < localSectionDatas.Num(); ++sectionIdx)
			{
				const LocalSectionData& localSectionData = localSectionDatas[sectionIdx];
				staticMesh->GetSectionInfoMap().Set(lodIndex, sectionIdx, FMeshSectionInfo(sectionIdx));
			}
		}
	}

	// Resolve physics
	if (phyHeader != nullptr)
	{
		if (!debugPhysics)
		{
			staticMesh->bCustomizedCollision = true;
			staticMesh->CreateBodySetup();
		}

		LocalMeshData debugPhysMeshData;
		
		uint8* curPtr = ((uint8*)phyHeader) + phyHeader->size;
		for (int i = 0; i < phyHeader->solidCount; ++i)
		{
			TArray<FPHYSection> sections;
			ReadPHYSolid(curPtr, sections);

			for (int sectionIndex = 0; sectionIndex < sections.Num(); ++sectionIndex)
			{
				const FPHYSection& section = sections[sectionIndex];
				// Filter out any section attached to a bone, as we're a static mesh
				if (section.BoneIndex == 0)
				{
					if (debugPhysics)
					{
						// Create poly group
						const FPolygonGroupID polyGroupID = debugPhysMeshData.meshDescription.CreatePolygonGroup();
						debugPhysMeshData.polyGroupMaterial[polyGroupID] = FName(*FString::Printf(TEXT("Solid%dSection%d"), i, sectionIndex));

						// Write to debug mesh
						TArray<FVertexID> vertIDs;
						vertIDs.Reserve(section.Vertices.Num());
						for (int j = 0; j < section.Vertices.Num(); ++j)
						{
							const FVertexID vertID = debugPhysMeshData.meshDescription.CreateVertex();
							debugPhysMeshData.vertPos[vertID] = section.Vertices[j];
							vertIDs.Add(vertID);
						}
						TArray<FVertexInstanceID> verts;
						for (int j = 0; j < section.FaceIndices.Num(); j += 3)
						{
							const int i0 = section.FaceIndices[j];
							const int i1 = section.FaceIndices[j + 1];
							const int i2 = section.FaceIndices[j + 2];
							const FVertexID v0 = vertIDs[i0];
							const FVertexID v1 = vertIDs[i1];
							const FVertexID v2 = vertIDs[i2];
							const FVertexInstanceID vi0 = debugPhysMeshData.meshDescription.CreateVertexInstance(v0);
							debugPhysMeshData.vertInstUV0[vi0] = FVector2D();
							const FVertexInstanceID vi1 = debugPhysMeshData.meshDescription.CreateVertexInstance(v1);
							debugPhysMeshData.vertInstUV0[vi1] = FVector2D();
							const FVertexInstanceID vi2 = debugPhysMeshData.meshDescription.CreateVertexInstance(v2);
							debugPhysMeshData.vertInstUV0[vi2] = FVector2D();

							verts.Empty(3);
							verts.Add(vi0);
							verts.Add(vi1);
							verts.Add(vi2);

							debugPhysMeshData.meshDescription.CreatePolygon(polyGroupID, verts);
						}
					}
					else
					{
						// Create primitive
						FMeshUtils::DecomposeUCXMesh(section.Vertices, section.FaceIndices, staticMesh->BodySetup);
					}
				}
			}
		}

		if (debugPhysics)
		{
			FStaticMeshSourceModel& sourceModel = staticMesh->AddSourceModel();
			FMeshBuildSettings& settings = sourceModel.BuildSettings;
			settings.bRecomputeNormals = false;
			settings.bRecomputeTangents = false;
			settings.bGenerateLightmapUVs = false;
			settings.bRemoveDegenerates = false;
			FMeshDescription* meshDescription = staticMesh->CreateMeshDescription(0);
			FMeshUtils::Clean(debugPhysMeshData.meshDescription);
			*meshDescription = debugPhysMeshData.meshDescription;
			staticMesh->CommitMeshDescription(0);
			for (int i = 0; i < phyHeader->solidCount; ++i)
			{
				FName material(*FString::Printf(TEXT("Solid%d"), i));
				const int32 meshSlot = staticMesh->StaticMaterials.Emplace(nullptr, material, material);
				staticMesh->GetSectionInfoMap().Set(0, meshSlot, FMeshSectionInfo(meshSlot));
			}
		}
	}

	staticMesh->LightMapCoordinateIndex = 1;
	staticMesh->Build();

	// Resolve skins
	{
		TArray<TArray<int>> rawSkinFamilies;
		ReadSkins(header, rawSkinFamilies);
		for (const TArray<int>& rawSkinFamily : rawSkinFamilies)
		{
			FModelSkinMapping skinMapping;
			// Key is src material index and value is dst material index
			// We've mapped src material index onto arbitrary sections
			// So, we need to go back over our section map, identify all sections pointing to src material index, and map them to dst material
			for (int srcMatIdx = 0; srcMatIdx < rawSkinFamily.Num(); ++srcMatIdx)
			{
				const int dstMatIdx = rawSkinFamily[srcMatIdx];
				if (srcMatIdx != dstMatIdx)
				{
					for (int sectionIdx = 0; sectionIdx < localSectionDatas.Num(); ++sectionIdx)
					{
						const LocalSectionData& localSectionData = localSectionDatas[sectionIdx];
						if (localSectionData.materialIndex == srcMatIdx)
						{
							const FName materialName = materials[dstMatIdx];
							skinMapping.MaterialOverrides.Add(sectionIdx, IHL2Runtime::Get().TryResolveHL2Material(materialName.ToString()));
						}
					}
				}
			}
			modelData->Skins.Add(skinMapping);
		}
	}
	modelData->PostEditChange();
	modelData->MarkPackageDirty();

	return staticMesh;
}

USkeletalMesh* UMDLFactory::ImportSkeletalMesh
(
	UObject* inParent, FName inName, UObject* inSkeletonParent, FName inSkeletonName, UObject* inPhysAssetParent, FName inPhysAssetName, EObjectFlags flags,
	const Valve::MDL::studiohdr_t& header, const Valve::VTX::FileHeader_t& vtxHeader, const Valve::VVD::vertexFileHeader_t& vvdHeader, const Valve::PHY::phyheader_t* phyHeader,
	FFeedbackContext* warn
)
{
	// Read and validate body parts
	TArray<const Valve::MDL::mstudiobodyparts_t*> bodyParts;
	header.GetBodyParts(bodyParts);
	TArray<const Valve::VTX::BodyPartHeader_t*> vtxBodyParts;
	vtxHeader.GetBodyParts(vtxBodyParts);
	if (bodyParts.Num() != vtxBodyParts.Num())
	{
		warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: Body parts in vtx didn't match body parts in mdl"));
		return nullptr;
	}

	// Create all data arrays
	TArray<FSkeletalMeshImportData> importDatas;
	importDatas.AddDefaulted(vtxHeader.numLODs);

	// Read materials
	TArray<FName> materials;
	ResolveMaterials(header, materials);
	struct LocalSectionData
	{
		int materialIndex;
		FName sectionName;
	};
	TArray<LocalSectionData> localSectionDatas;

	// Read bones
	TArray<const Valve::MDL::mstudiobone_t*> bones;
	header.GetBones(bones);
	const bool multiRoot = SkeletonHasMultipleRoots(bones);
	const int boneIndexOffset = multiRoot ? 1 : 0;

	// Create skeleton
	USkeleton* skeleton = CreateSkeleton(inSkeletonParent, inSkeletonName, flags);

	// Create skeletal mesh
	USkeletalMesh* skeletalMesh = CreateSkeletalMesh(inParent, inName, flags);
	skeletalMesh->Skeleton = skeleton;

	// Create model data
	UHL2ModelData* modelData = NewObject<UHL2ModelData>(skeletalMesh);
	skeletalMesh->AddAssetUserData(modelData);

	// Load bones into skeleton
	FReferenceSkeleton refSkelSource;
	{
		FReferenceSkeletonModifier refSkelSourceMod(refSkelSource, skeleton);
		refSkelSource.Empty(bones.Num() + boneIndexOffset);
		if (multiRoot)
		{
			FMeshBoneInfo meshBoneInfo;
			meshBoneInfo.ParentIndex = -1;
			meshBoneInfo.ExportName = TEXT("GENERATED_ROOT");
			meshBoneInfo.Name = FName(TEXT("root"));
			refSkelSourceMod.Add(meshBoneInfo, FTransform::Identity);
		}
		for (int i = 0; i < bones.Num(); ++i)
		{
			const Valve::MDL::mstudiobone_t& bone = *bones[i];
			FMeshBoneInfo meshBoneInfo;
			if (bone.parent < 0)
			{
				meshBoneInfo.ParentIndex = boneIndexOffset - 1;
			}
			else
			{
				meshBoneInfo.ParentIndex = bone.parent + boneIndexOffset;
			}
			meshBoneInfo.ExportName = bone.GetName();
			meshBoneInfo.Name = FName(*meshBoneInfo.ExportName);

			FTransform bonePose;
			bonePose.SetLocation(FVector(bone.pos[0], bone.pos[1], bone.pos[2]));
			bonePose.SetRotation(FQuat(bone.quat[0], bone.quat[1], bone.quat[2], bone.quat[3]).GetNormalized());
			refSkelSourceMod.Add(meshBoneInfo, bonePose);
		}
	}
	const FReferenceSkeleton refSkel = FSkeletonUtils::TransformSkeleton(refSkelSource, StudioMdlToUnreal);
	skeletalMesh->RefSkeleton = refSkel;
	skeleton->RecreateBoneTree(skeletalMesh);

	// Prepare mesh geometry
	FSkeletalMeshModel* meshModel = skeletalMesh->GetImportedModel();
	meshModel->LODModels.Empty();
	skeletalMesh->ResetLODInfo();

	// Iterate all body parts
	uint16 accumIndex = 0;
	for (int bodyPartIndex = 0; bodyPartIndex < bodyParts.Num(); ++bodyPartIndex)
	{
		const Valve::MDL::mstudiobodyparts_t& bodyPart = *bodyParts[bodyPartIndex];
		const Valve::VTX::BodyPartHeader_t& vtxBodyPart = *vtxBodyParts[bodyPartIndex];
		FModelBodygroup bodygroup;

		// Read and validate models
		TArray<const Valve::MDL::mstudiomodel_t*> models;
		bodyPart.GetModels(models);
		TArray<const Valve::VTX::ModelHeader_t*> vtxModels;
		vtxBodyPart.GetModels(vtxModels);
		if (models.Num() != vtxModels.Num())
		{
			warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: Models in vtx didn't match models in mdl for body part %d"), bodyPartIndex);
			return nullptr;
		}

		// Iterate all models
		for (int modelIndex = 0; modelIndex < models.Num(); ++modelIndex)
		{
			const Valve::MDL::mstudiomodel_t& model = *models[modelIndex];
			const Valve::VTX::ModelHeader_t& vtxModel = *vtxModels[modelIndex];
			TMap<int, int> materialToSectionMap;

			FModelBodygroupMapping bodygroupMapping;

			// Read and validate meshes & lods
			TArray<const Valve::MDL::mstudiomesh_t*> meshes;
			model.GetMeshes(meshes);
			TArray<const Valve::VTX::ModelLODHeader_t*> lods;
			vtxModel.GetLODs(lods);
			if (lods.Num() != vtxHeader.numLODs)
			{
				warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: Model %d in vtx didn't have correct lod count for body part %d"), modelIndex, bodyPartIndex);
				return nullptr;
			}

			// Iterate all lods
			for (int lodIndex = 0; lodIndex < lods.Num(); ++lodIndex)
			{
				FSkeletalMeshImportData& importData = importDatas[lodIndex];
				importData.bHasNormals = true;
				importData.bHasTangents = true;
				importData.bHasVertexColors = false;
				const Valve::VTX::ModelLODHeader_t& lod = *lods[lodIndex];

				// Iterate all vertices in the vvd for this lod and insert them
				TArray<Valve::VVD::mstudiovertex_t> vvdVertices;
				TArray<FVector4> vvdTangents;
				{
					const uint16 vertCount = (uint16)vvdHeader.numLODVertexes[0];
					vvdVertices.Reserve(vertCount);
					vvdTangents.Reserve(vertCount);

					if (vvdHeader.numFixups > 0)
					{
						TArray<Valve::VVD::mstudiovertex_t> fixupVertices;
						TArray<FVector4> fixupTangents;
						TArray<Valve::VVD::vertexFileFixup_t> fixups;
						vvdHeader.GetFixups(fixups);
						for (const Valve::VVD::vertexFileFixup_t& fixup : fixups)
						{
							//if (fixup.lod >= lodIndex)
							{
								for (int i = 0; i < fixup.numVertexes; ++i)
								{
									Valve::VVD::mstudiovertex_t vertex;
									FVector4 tangent;
									vvdHeader.GetVertex(fixup.sourceVertexID + i, vertex, tangent);
									vvdVertices.Add(vertex);
									vvdTangents.Add(tangent);
								}
							}
						}
					}
					else
					{
						for (uint16 i = 0; i < vertCount; ++i)
						{
							Valve::VVD::mstudiovertex_t vertex;
							FVector4 tangent;
							vvdHeader.GetVertex(i, vertex, tangent);
							vvdVertices.Add(vertex);
							vvdTangents.Add(tangent);
						}
					}
				}

				// Fetch and iterate all meshes
				TArray<const Valve::VTX::MeshHeader_t*> vtxMeshes;
				lod.GetMeshes(vtxMeshes);
				for (int meshIndex = 0; meshIndex < meshes.Num(); ++meshIndex)
				{
					const Valve::MDL::mstudiomesh_t& mesh = *meshes[meshIndex];
					const Valve::VTX::MeshHeader_t& vtxMesh = *vtxMeshes[meshIndex];

					// Fetch and validate material
					if (!materials.IsValidIndex(mesh.material))
					{
						warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: Mesh %d in model %d had invalid material index %d (%d materials available)"), meshIndex, modelIndex, mesh.material, materials.Num());
						return nullptr;
					}

					// Allocate a section index for us
					int sectionIndex;
					{
						const int* sectionIndexPtr = materialToSectionMap.Find(mesh.material);
						if (sectionIndexPtr == nullptr)
						{
							LocalSectionData localSectionData;
							localSectionData.materialIndex = mesh.material;
							localSectionData.sectionName = FName(*FString::Printf(TEXT("%s:%d:%s"), *bodyPart.GetName(), modelIndex, *materials[mesh.material].ToString()));
							sectionIndex = localSectionDatas.Add(localSectionData);
							materialToSectionMap.Add(mesh.material, sectionIndex);
						}
						else
						{
							sectionIndex = *sectionIndexPtr;
						}
					}

					// Add to the current bodygroup mapping
					bodygroup.AllSections.Add(sectionIndex);
					bodygroupMapping.Sections.Add(sectionIndex);

					// Fetch and iterate all strip groups
					TArray<const Valve::VTX::StripGroupHeader_t*> stripGroups;
					vtxMesh.GetStripGroups(stripGroups);
					for (int stripGroupIndex = 0; stripGroupIndex < stripGroups.Num(); ++stripGroupIndex)
					{
						const Valve::VTX::StripGroupHeader_t& stripGroup = *stripGroups[stripGroupIndex];

						TArray<uint16> indices;
						stripGroup.GetIndices(indices);

						TArray<Valve::VTX::Vertex_t> vertices;
						stripGroup.GetVertices(vertices);

						TArray<const Valve::VTX::StripHeader_t*> strips;
						stripGroup.GetStrips(strips);

						for (int stripIndex = 0; stripIndex < strips.Num(); ++stripIndex)
						{
							const Valve::VTX::StripHeader_t& strip = *strips[stripIndex];

							for (int i = 0; i < strip.numIndices; i += 3)
							{
								uint16 idxs[3];
								if (StudioMdlToUnreal.ShouldReverseWinding())
								{
									idxs[0] = indices[strip.indexOffset + i + 2];
									idxs[1] = indices[strip.indexOffset + i + 1];
									idxs[2] = indices[strip.indexOffset + i];
								}
								else
								{
									idxs[0] = indices[strip.indexOffset + i];
									idxs[1] = indices[strip.indexOffset + i + 1];
									idxs[2] = indices[strip.indexOffset + i + 2];
								}

								const Valve::VTX::Vertex_t* verts[3] =
								{
									 &vertices[idxs[0]],
									 &vertices[idxs[1]],
									 &vertices[idxs[2]]
								};

								const uint16 baseIdxs[3] =
								{
									accumIndex + mesh.vertexoffset + verts[0]->origMeshVertID,
									accumIndex + mesh.vertexoffset + verts[1]->origMeshVertID,
									accumIndex + mesh.vertexoffset + verts[2]->origMeshVertID
								};

								const Valve::VVD::mstudiovertex_t* origVerts[3] =
								{
									&vvdVertices[baseIdxs[0]],
									&vvdVertices[baseIdxs[1]],
									&vvdVertices[baseIdxs[2]]
								};

								SkeletalMeshImportData::FTriangle face;
								face.MatIndex = (uint16)sectionIndex;
								face.SmoothingGroups = 0;
								for (int j = 0; j < 3; ++j)
								{
									const Valve::VTX::Vertex_t& vert = *verts[j];
									const Valve::VVD::mstudiovertex_t& origVert = *origVerts[j];

									const int vertIdx = importData.Points.Add(StudioMdlToUnreal.Position(origVert.m_vecPosition));
									importData.PointToRawMap.Add(baseIdxs[j]);

									SkeletalMeshImportData::FVertex wedge;
									wedge.VertexIndex = vertIdx;
									wedge.Color = FColor::White;
									wedge.UVs[0] = origVert.m_vecTexCoord;
									face.WedgeIndex[j] = importData.Wedges.Add(wedge);
									face.TangentZ[j] = StudioMdlToUnreal.Direction(origVert.m_vecNormal.GetSafeNormal());
									face.TangentY[j] = StudioMdlToUnreal.Direction(vvdTangents[baseIdxs[j]].GetSafeNormal());
									face.TangentX[j] = FVector::CrossProduct(face.TangentY[j], face.TangentZ[j]);
									
									const int numBones = FMath::Min((int)origVert.m_BoneWeights.numbones, Valve::VVD::MAX_NUM_BONES_PER_VERT);
									for (int k = 0; k < numBones; ++k)
									{
										const uint8 boneWeightIdx = vert.boneWeightIndex[k];
										const int boneID = origVert.m_BoneWeights.bone[boneWeightIdx] + boneIndexOffset;
										// check(vert.boneID[k] == boneID);
										const float weight = origVert.m_BoneWeights.weight[boneWeightIdx];
										SkeletalMeshImportData::FRawBoneInfluence influence;
										influence.BoneIndex = boneID;
										influence.VertexIndex = wedge.VertexIndex;
										influence.Weight = weight;
										importData.Influences.Add(influence);
									}
								}
								importData.Faces.Add(face);
							}
						}
					}
				}
			}

			bodygroup.Mappings.Add(bodygroupMapping);

			accumIndex += model.numvertices;
		}

		modelData->Bodygroups.Add(FName(*bodyPart.GetName()), bodygroup);
	}

	// Build geometry
	skeletalMesh->bHasVertexColors = false;
	IMeshBuilderModule& meshBuilderModule = IMeshBuilderModule::GetForRunningPlatform();
	for (int lodIndex = 0; lodIndex < importDatas.Num(); ++lodIndex)
	{
		FSkeletalMeshImportData& importData = importDatas[lodIndex];

		// Prepare lod on skeletal mesh
		meshModel->LODModels.Add(new FSkeletalMeshLODModel());
		FSkeletalMeshLODModel& LODModel = meshModel->LODModels[lodIndex];
		FSkeletalMeshLODInfo& newLODInfo = skeletalMesh->AddLODInfo();
		newLODInfo.ReductionSettings.NumOfTrianglesPercentage = 1.0f;
		newLODInfo.ReductionSettings.NumOfVertPercentage = 1.0f;
		newLODInfo.ReductionSettings.MaxDeviationPercentage = 0.0f;
		newLODInfo.LODHysteresis = 0.02f;
		if (lodIndex == 0)
		{
			skeletalMesh->SetImportedBounds(FBoxSphereBounds(&importData.Points[0], (uint32)importData.Points.Num()));
		}
		LODModel.NumTexCoords = 1;
		newLODInfo.BuildSettings.bRecomputeNormals = false;
		newLODInfo.BuildSettings.bRecomputeTangents = false;
		newLODInfo.BuildSettings.bRemoveDegenerates = false;

		// Build
		skeletalMesh->SaveLODImportedData(lodIndex, importData);
		const bool buildSuccess = meshBuilderModule.BuildSkeletalMesh(skeletalMesh, lodIndex, false);
		if (!buildSuccess)
		{
			warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: Skeletal mesh build for lod %d failed"), lodIndex);
			return nullptr;
		}

		// Calculate required bones
		USkeletalMesh::CalculateRequiredBones(LODModel, refSkel, nullptr);
	}
	skeletalMesh->CalculateInvRefMatrices();
	skeletalMesh->Build();

	// Assign materials
	for (int sectionIdx = 0; sectionIdx < localSectionDatas.Num(); ++sectionIdx)
	{
		const LocalSectionData& localSectionData = localSectionDatas[sectionIdx];
		const FName materialName = materials[localSectionData.materialIndex];
		FSkeletalMaterial material;
		material.ImportedMaterialSlotName = localSectionData.sectionName;
		material.MaterialSlotName = localSectionData.sectionName;
		material.MaterialInterface = IHL2Runtime::Get().TryResolveHL2Material(materialName.ToString());
		skeletalMesh->Materials.Add(material);
	}

	// Resolve physics
	if (phyHeader != nullptr && inPhysAssetParent != nullptr)
	{
		constexpr bool DEBUG_PHYSICS_MESH = false;
		FMeshDescription debugMeshDesc;
		if (DEBUG_PHYSICS_MESH)
		{
			FStaticMeshAttributes attr(debugMeshDesc);
			attr.Register();
			attr.RegisterPolygonNormalAndTangentAttributes();
		}
		UPhysicsAsset* physicsAsset = ImportPhysicsAsset(inPhysAssetParent, inPhysAssetName, flags, header, phyHeader, refSkel, DEBUG_PHYSICS_MESH ? &debugMeshDesc : nullptr, warn);
		physicsAsset->PreviewSkeletalMesh = skeletalMesh;
		skeletalMesh->PhysicsAsset = physicsAsset;
		if (DEBUG_PHYSICS_MESH)
		{
			UStaticMesh* staticMesh = CreateStaticMesh(inParent, FName(inName.ToString() + TEXT("_DebugPhys")), flags);
			FStaticMeshSourceModel& sourceModel = staticMesh->AddSourceModel();
			FMeshBuildSettings& settings = sourceModel.BuildSettings;
			settings.bRecomputeNormals = false;
			settings.bRecomputeTangents = false;
			settings.bGenerateLightmapUVs = false;
			settings.bRemoveDegenerates = false;
			FMeshDescription* meshDescription = staticMesh->CreateMeshDescription(0);
			FStaticMeshOperations::ComputePolygonTangentsAndNormals(debugMeshDesc);
			FStaticMeshOperations::ComputeTangentsAndNormals(debugMeshDesc, EComputeNTBsFlags::Normals | EComputeNTBsFlags::Tangents);
			FMeshUtils::Clean(debugMeshDesc);
			*meshDescription = debugMeshDesc;
			staticMesh->CommitMeshDescription(0);
			/*for (int i = 0; i < phyHeader->solidCount; ++i)
			{
				FName material(*FString::Printf(TEXT("Solid%d"), i));
				const int32 meshSlot = staticMesh->StaticMaterials.Emplace(nullptr, material, material);
				staticMesh->GetSectionInfoMap().Set(0, meshSlot, FMeshSectionInfo(meshSlot));
			}*/
			staticMesh->Build();
			staticMesh->PostEditChange();
			staticMesh->MarkPackageDirty();
			FAssetRegistryModule::AssetCreated(staticMesh);
			GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, staticMesh);
		}
	}

	// Resolve skins
	{
		TArray<TArray<int>> rawSkinFamilies;
		ReadSkins(header, rawSkinFamilies);
		for (const TArray<int>& rawSkinFamily : rawSkinFamilies)
		{
			FModelSkinMapping skinMapping;
			// Key is src material index and value is dst material index
			// We've mapped src material index onto arbitrary sections
			// So, we need to go back over our section map, identify all sections pointing to src material index, and map them to dst material
			for (int srcMatIdx = 0; srcMatIdx < rawSkinFamily.Num(); ++srcMatIdx)
			{
				const int dstMatIdx = rawSkinFamily[srcMatIdx];
				if (srcMatIdx != dstMatIdx)
				{
					for (int sectionIdx = 0; sectionIdx < localSectionDatas.Num(); ++sectionIdx)
					{
						const LocalSectionData& localSectionData = localSectionDatas[sectionIdx];
						if (localSectionData.materialIndex == srcMatIdx)
						{
							const FName materialName = materials[dstMatIdx];
							skinMapping.MaterialOverrides.Add(sectionIdx, IHL2Runtime::Get().TryResolveHL2Material(materialName.ToString()));
						}
					}
				}
			}
			modelData->Skins.Add(skinMapping);
		}
	}

	// Resolve attachments
	{
		TArray<USkeletalMeshSocket*>& sockets = skeletalMesh->GetMeshOnlySocketList();
		TArray<const Valve::MDL::mstudioattachment_t*> attachments;
		header.GetAttachments(attachments);
		for (const Valve::MDL::mstudioattachment_t* attachment : attachments)
		{
			const int boneIndex = attachment->localbone + boneIndexOffset;
			USkeletalMeshSocket* socket = NewObject<USkeletalMeshSocket>(skeletalMesh);
			check(refSkel.IsValidIndex(boneIndex));
			const FName boneName = refSkel.GetBoneName(boneIndex);
			socket->BoneName = boneName;
			socket->SocketName = FName(attachment->GetName());
			// ATTACHMENT_FLAG_WORLD_ALIGN 
			if ((attachment->flags & 0x10000) != 0)
			{
				// TODO: Figure out how to make a world-aligned socket (does unreal even support this?)
			}
			const FTransform attachmentTransformSource(
				FVector(attachment->local[0], attachment->local[4], attachment->local[8]),
				FVector(attachment->local[1], attachment->local[5], attachment->local[9]),
				FVector(attachment->local[2], attachment->local[6], attachment->local[10]),
				FVector(attachment->local[3], attachment->local[7], attachment->local[11])
			);
			const FTransform boneComponentTransformSource = FSkeletonUtils::GetBoneComponentTransform(refSkelSource, boneIndex);
			const FTransform attachmentComponentTransform = StudioMdlToUnreal.Transform(attachmentTransformSource * boneComponentTransformSource);
			const FTransform boneComponentTransform = FSkeletonUtils::GetBoneComponentTransform(refSkel, boneIndex);
			const FTransform attachmentTransform = attachmentComponentTransform.GetRelativeTransform(boneComponentTransform);
			socket->RelativeLocation = attachmentTransform.GetLocation();
			socket->RelativeRotation = attachmentTransform.GetRotation().Rotator();
			socket->RelativeScale = attachmentTransform.GetScale3D();
			sockets.Add(socket);
		}
	}
	modelData->PostEditChange();
	modelData->MarkPackageDirty();

	// Done
	return skeletalMesh;
}

UPhysicsAsset* UMDLFactory::ImportPhysicsAsset
(
	UObject* inParent, FName inName, EObjectFlags flags,
	const Valve::MDL::studiohdr_t& header, const Valve::PHY::phyheader_t* phyHeader, const FReferenceSkeleton& referenceSkeleton, FMeshDescription* outDebugMesh,
	FFeedbackContext* warn
)
{
	// Create phys asset
	UPhysicsAsset* physicsAsset = CreatePhysAsset(inParent, inName, flags);

	// Read solids
	uint8* curPtr = ((uint8*)phyHeader) + phyHeader->size;
	TArray<TArray<FPHYSection>> solids;
	for (int i = 0; i < phyHeader->solidCount; ++i)
	{
		TArray<FPHYSection>& sections = solids[solids.AddDefaulted()];
		ReadPHYSolid(curPtr, sections);
	}

	// Read text section
	UValveDocument* doc;
	{
		const auto rawStr = StringCast<TCHAR, ANSICHAR>((const char*)curPtr);
		const FString textSection(rawStr.Length(), rawStr.Get());
		doc = UValveDocument::Parse(textSection);
		if (doc == nullptr)
		{
			warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: Failed to parse PHY text section"));
			return nullptr;
		}
	}
	const UValveGroupValue* root = CastChecked<UValveGroupValue>(doc->Root);

	// Cache bone data
	TArray<FTransform> boneComponentTransforms = FSkeletonUtils::GetSkeletonComponentTransforms(referenceSkeleton);
	const FReferenceSkeleton referenceSkeletonSource = FSkeletonUtils::TransformSkeleton(referenceSkeleton, UnrealToStudioMdl);
	TArray<FTransform> boneComponentSourceTransforms = FSkeletonUtils::GetSkeletonComponentTransforms(referenceSkeletonSource);
	const bool multiRoot = SkeletonHasMultipleRoots(header);
	const int boneIndexOffset = multiRoot ? 1 : 0;

	// Parse solids
	TArray<UValveValue*> solidValues;
	static const FName fnSolid(TEXT("solid"));
	root->GetItems(fnSolid, solidValues);
	TArray<TArray<int>> indexToBone;
	for (const UValveValue* solidValue : solidValues)
	{
		const UValveGroupValue* solidGroupValue = CastChecked<UValveGroupValue>(solidValue);

		// Parse index
		int index;
		static const FName fnIndex(TEXT("index"));
		if (!solidGroupValue->GetInt(fnIndex, index))
		{
			warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: PHY solid missing index"));
			continue;
		}
		if (!solids.IsValidIndex(index))
		{
			warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: PHY solid refers to invalid index %i"), index);
			continue;
		}
		if (index <= indexToBone.Num())
		{
			indexToBone.AddDefaulted((index - indexToBone.Num()) + 1);
		}
		TArray<int>& solidBones = indexToBone[index];

		// Parse name
		FName name;
		static const FName fnName(TEXT("name"));
		if (!solidGroupValue->GetName(fnName, name))
		{
			warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: PHY solid %i missing name"), index);
			continue;
		}

		// Parse surface prop
		FName surfacePropName;
		static const FName fnSurfaceProp(TEXT("surfaceprop"));
		USurfaceProp* surfaceProp = nullptr;
		if (solidGroupValue->GetName(fnSurfaceProp, surfacePropName))
		{
			surfaceProp = IHL2Runtime::Get().TryResolveHL2SurfaceProp(surfacePropName);
		}

		// Iterate sections
		TArray<FPHYSection>& sections = solids[index];
		for (int sectionIndex = 0; sectionIndex < sections.Num(); ++sectionIndex)
		{
			FPHYSection& section = sections[sectionIndex];
			const int boneIndex = section.BoneIndex + boneIndexOffset;
			if (referenceSkeleton.IsValidIndex(boneIndex))
			{
				solidBones.AddUnique(boneIndex);

				// Retrieve body setup
				const FName bodyName = referenceSkeleton.GetBoneName(boneIndex);
				const int bodyIndex = physicsAsset->FindBodyIndex(bodyName);
				USkeletalBodySetup* bodySetup = bodyIndex == INDEX_NONE ? nullptr : physicsAsset->SkeletalBodySetups[bodyIndex];
				if (bodySetup == nullptr)
				{
					bodySetup = NewObject<USkeletalBodySetup>(physicsAsset, name);
					bodySetup->CollisionTraceFlag = CTF_UseSimpleAsComplex;
					bodySetup->PhysicsType = PhysType_Default;
					bodySetup->BoneName = referenceSkeleton.GetBoneName(boneIndex);
					bodySetup->bGenerateMirroredCollision = false;
					bodySetup->bConsiderForBounds = true;
					bodySetup->PhysMaterial = surfaceProp;
					physicsAsset->SkeletalBodySetups.Add(bodySetup);
					physicsAsset->UpdateBodySetupIndexMap();
					physicsAsset->UpdateBoundsBodiesArray();
				}

				// Correct transform of verts (which are in bone-local space)
				for (FVector& position : section.Vertices)
				{
					const FVector componentPosSource = boneComponentSourceTransforms[boneIndex].TransformPosition(position);
					const FVector componentPos = StudioMdlToUnreal.Position(componentPosSource);
					position = boneComponentTransforms[boneIndex].InverseTransformPosition(componentPos);
				}
				if (StudioMdlToUnreal.ShouldReverseWinding())
				{
					for (int i = 0; i < section.FaceIndices.Num(); i += 3)
					{
						Swap(section.FaceIndices[i], section.FaceIndices[i + 2]);
					}
				}

				// Build mesh
				FMeshUtils::DecomposeUCXMesh(section.Vertices, section.FaceIndices, bodySetup);
				bodySetup->CalculateMass();

				// Write debug
				if (outDebugMesh != nullptr)
				{
					FStaticMeshAttributes attr(*outDebugMesh);
					FPolygonGroupID polyGroupID = outDebugMesh->CreatePolygonGroup();
					attr.GetPolygonGroupMaterialSlotNames()[polyGroupID] = referenceSkeleton.GetBoneName(boneIndex);
					TArray<FVertexID> vertexIDs;
					vertexIDs.Reserve(section.Vertices.Num());
					for (const FVector& position : section.Vertices)
					{
						const FVertexID vertexID = outDebugMesh->CreateVertex();
						attr.GetVertexPositions()[vertexID] = boneComponentTransforms[boneIndex].TransformPosition(position);
						vertexIDs.Add(vertexID);
					}
					for (int i = 0; i < section.FaceIndices.Num(); i += 3)
					{
						const int i0 = section.FaceIndices[i + 0];
						const int i1 = section.FaceIndices[i + 1];
						const int i2 = section.FaceIndices[i + 2];

						const FVertexInstanceID vertexInstID0 = outDebugMesh->CreateVertexInstance(vertexIDs[i0]);
						const FVertexInstanceID vertexInstID1 = outDebugMesh->CreateVertexInstance(vertexIDs[i1]);
						const FVertexInstanceID vertexInstID2 = outDebugMesh->CreateVertexInstance(vertexIDs[i2]);

						outDebugMesh->CreatePolygon(polyGroupID, TArray<FVertexInstanceID> { vertexInstID0, vertexInstID1, vertexInstID2 });
					}
				}
			}
		}
	}

	// Parse constraints
	TArray<UValveValue*> constraintValues;
	static const FName fnRagdollConstraint(TEXT("ragdollconstraint"));
	root->GetItems(fnRagdollConstraint, constraintValues);
	for (const UValveValue* constraintValue : constraintValues)
	{
		const UValveGroupValue* constraintGroupValue = CastChecked<UValveGroupValue>(constraintValue);

		// Resolve parent
		int parent;
		static const FName fnParent(TEXT("parent"));
		if (!constraintGroupValue->GetInt(fnParent, parent))
		{
			warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: PHY constraint missing parent"));
			continue;
		}
		const TArray<int>& parentBones = indexToBone[parent];
		check(parentBones.Num() > 0);

		// Resolve child
		int child;
		static const FName fnChild(TEXT("child"));
		if (!constraintGroupValue->GetInt(fnChild, child))
		{
			warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: PHY constraint missing child"));
			continue;
		}
		const TArray<int>& childBones = indexToBone[child];
		check(childBones.Num() > 0);

		// Parse rotational constraint
		float xMin, xMax, xFric, yMin, yMax, yFric, zMin, zMax, zFric;

		static const FName fnXMin(TEXT("xmin"));
		if (!constraintGroupValue->GetFloat(fnXMin, xMin)) { xMin = 0.0f; }
		static const FName fnXMax(TEXT("xmax"));
		if (!constraintGroupValue->GetFloat(fnXMax, xMax)) { xMax = 0.0f; }
		static const FName fnXFric(TEXT("xfric"));
		if (!constraintGroupValue->GetFloat(fnXFric, xFric)) { xFric = 0.0f; }

		static const FName fnYMin(TEXT("ymin"));
		if (!constraintGroupValue->GetFloat(fnYMin, yMin)) { yMin = 0.0f; }
		static const FName fnYMax(TEXT("ymax"));
		if (!constraintGroupValue->GetFloat(fnYMax, yMax)) { yMax = 0.0f; }
		static const FName fnYFric(TEXT("yfric"));
		if (!constraintGroupValue->GetFloat(fnYFric, yFric)) { yFric = 0.0f; }

		static const FName fnZMin(TEXT("zmin"));
		if (!constraintGroupValue->GetFloat(fnZMin, zMin)) { zMin = 0.0f; }
		static const FName fnZMax(TEXT("zmax"));
		if (!constraintGroupValue->GetFloat(fnZMax, zMax)) { zMax = 0.0f; }
		static const FName fnZFric(TEXT("zfric"));
		if (!constraintGroupValue->GetFloat(fnZFric, zFric)) { zFric = 0.0f; }

		UPhysicsConstraintTemplate* constraintTemplate = NewObject<UPhysicsConstraintTemplate>(physicsAsset);
		FConstraintInstance& constraint = constraintTemplate->DefaultInstance;

		const FTransform& parentComponentTransform = boneComponentTransforms[parentBones[0]];
		const FTransform& childComponentTransform = boneComponentTransforms[childBones[0]];

		constraint.ConstraintBone1 = referenceSkeleton.GetBoneName(parentBones[0]);
		constraint.SetRefFrame(EConstraintFrame::Frame1, childComponentTransform.GetRelativeTransform(parentComponentTransform));

		constraint.ConstraintBone2 = referenceSkeleton.GetBoneName(childBones[0]);
		//constraint.SetRefFrame(EConstraintFrame::Frame2, FTransform(FQuat(FRotator((xMin + xMax) * 0.5f, (yMin + yMax) * 0.5f, (zMin + zMax) * 0.5f))));

		constraint.SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, xMax - xMin);
		constraint.SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Limited, zMax - zMin);
		constraint.SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Limited, yMax - yMin);
		

		constraint.ConstraintIndex = physicsAsset->ConstraintSetup.Add(constraintTemplate);
	}

	// Parse collision rules
	static const FName fnCollisionRules(TEXT("collisionrules"));
	UValveValue* collisionRules = root->GetItem(fnCollisionRules);
	if (collisionRules != nullptr)
	{
		const UValveGroupValue* collisionRulesGroup = CastChecked<UValveGroupValue>(collisionRules);
		TArray<UValveValue*> collisionPairs;
		static const FName fnCollisionPair(TEXT("collisionpair"));
		collisionRulesGroup->GetItems(fnCollisionPair, collisionPairs);
		if (collisionPairs.Num() > 0)
		{
			for (int i = 0; i < physicsAsset->SkeletalBodySetups.Num(); ++i)
			{
				for (int j = i + 1; j < physicsAsset->SkeletalBodySetups.Num(); ++j)
				{
					physicsAsset->DisableCollision(i, j);
				}
			}
		}
		for (const UValveValue* collisionPairValue : collisionPairs)
		{
			const UValvePrimitiveValue* collisionPair = CastChecked<UValvePrimitiveValue>(collisionPairValue);
			const FString collisionPairStr = collisionPair->AsString();
			FString left, right;
			collisionPairStr.Split(TEXT(","), &left, &right);
			const int leftIndex = FCString::Atoi(*left);
			const int rightIndex = FCString::Atoi(*right);
			physicsAsset->EnableCollision(leftIndex, rightIndex);
		}
	}

	return physicsAsset;
}

void UMDLFactory::ImportSequences(const Valve::MDL::studiohdr_t& header, USkeletalMesh* skeletalMesh, const FString& basePath, const Valve::MDL::studiohdr_t* aniHeader, TArray<UAnimSequence*>& outSequences, FFeedbackContext* warn)
{
	TArray<const Valve::MDL::mstudioanimdesc_t*> localAnims;
	header.GetLocalAnims(localAnims);
	TArray<const Valve::MDL::mstudioseqdesc_t*> localSequences;
	header.GetLocalSequences(localSequences);
	TArray<const Valve::MDL::mstudioanimblock_t*> animBlocks;
	header.GetAnimBlocks(animBlocks);
	TArray<const Valve::MDL::mstudiobone_t*> bones;
	header.GetBones(bones);
	const bool multiRoot = SkeletonHasMultipleRoots(bones);
	const int boneIndexOffset = multiRoot ? 1 : 0;

	const FReferenceSkeleton& refSkel = skeletalMesh->RefSkeleton;
	const FReferenceSkeleton refSkelSource = FSkeletonUtils::TransformSkeleton(refSkel, UnrealToStudioMdl);

	for (const Valve::MDL::mstudioanimdesc_t* animDesc : localAnims)
	{
		FString name = animDesc->GetName();
		name.RemoveFromStart(TEXT("@"));

		UPackage* package = CreatePackage(nullptr, *(basePath + name));
		UAnimSequence* sequence = CreateAnimSequence(package, FName(*FPaths::GetBaseFilename(basePath + name)), RF_Public | RF_Standalone);
		sequence->SetSkeleton(skeletalMesh->Skeleton);
		sequence->SetRawNumberOfFrame(animDesc->numframes);
		sequence->SequenceLength = animDesc->numframes / (float)animDesc->fps;
		sequence->AdditiveAnimType = EAdditiveAnimationType::AAT_None;

		if (!animDesc->HasFlag(Valve::MDL::mstudioanimdesc_flag::ALLZEROS))
		{
			TArray<TArray<const Valve::MDL::mstudioanim_t*>> allSectionAnims;

			// Read anim data
			if (animDesc->sectionindex != 0 && animDesc->sectionframes > 0)
			{
				TArray<const Valve::MDL::mstudioanimsections_t*> sections;
				animDesc->GetSections(sections);
				allSectionAnims.AddDefaulted(sections.Num());
				if (animDesc->animblock == 0)
				{
					for (int sectionIndex = 0; sectionIndex < sections.Num(); ++sectionIndex)
					{
						const Valve::MDL::mstudioanimsections_t* section = sections[sectionIndex];
						TArray<const Valve::MDL::mstudioanim_t*>& sectionAnims = allSectionAnims[sectionIndex];
						uint8* basePtr = ((uint8*)animDesc) + section->animindex;
						ReadAnimData(basePtr, skeletalMesh, sectionAnims);
					}
				}
				else
				{
					check(aniHeader != nullptr);
					for (int sectionIndex = 0; sectionIndex < sections.Num(); ++sectionIndex)
					{
						const Valve::MDL::mstudioanimsections_t* section = sections[sectionIndex];
						const Valve::MDL::mstudioanimblock_t* block = animBlocks[section->animblock];
						TArray<const Valve::MDL::mstudioanim_t*>& sectionAnims = allSectionAnims[sectionIndex];
						uint8* basePtr = ((uint8*)aniHeader) + block->datastart + section->animindex;
						ReadAnimData(basePtr, skeletalMesh, sectionAnims);
					}
				}
			}
			else
			{
				uint8* basePtr;
				if (animDesc->animblock == 0)
				{
					basePtr = ((uint8*)animDesc) + animDesc->animindex;
				}
				else
				{
					check(aniHeader != nullptr);
					const Valve::MDL::mstudioanimblock_t* block = animBlocks[animDesc->animblock];
					basePtr = ((uint8*)aniHeader) + block->datastart + animDesc->animindex;
				}
				ReadAnimData(basePtr, skeletalMesh, allSectionAnims[allSectionAnims.AddDefaulted()]);
			}

			// Load anims into sequence
			TMap<uint8, FRawAnimSequenceTrack> boneTrackMap;
			int framesLeft = animDesc->numframes;
			for (int sectionIndex = 0; sectionIndex < allSectionAnims.Num(); ++sectionIndex)
			{
				const TArray<const Valve::MDL::mstudioanim_t*>& sectionAnims = allSectionAnims[sectionIndex];
				const int sectionFrameCount = allSectionAnims.Num() > 1 ? FMath::Min(framesLeft, animDesc->sectionframes) : animDesc->numframes;
				framesLeft -= sectionFrameCount;
				const int sectionFrameIndex = sectionIndex * animDesc->sectionframes;
				for (const Valve::MDL::mstudioanim_t* anim : sectionAnims)
				{
					const int boneIndex = anim->bone + boneIndexOffset;
					check(bones.IsValidIndex(anim->bone));
					const Valve::MDL::mstudiobone_t* bone = bones[anim->bone];
					const FTransform& boneRefPose = refSkelSource.GetRefBonePose()[boneIndex];
					FRawAnimSequenceTrack& track = boneTrackMap.FindOrAdd(boneIndex);
					if (track.RotKeys.Num() == 0) { track.RotKeys.Init(boneRefPose.GetRotation(), animDesc->numframes); }
					if (track.PosKeys.Num() == 0) { track.PosKeys.Init(boneRefPose.GetLocation(), animDesc->numframes); }
					const bool isRootBone = bone->parent < 0;

					// Rotation
					if (anim->HasFlag(Valve::MDL::mstudioanim_flag::ANIMROT))
					{
						// Compressed rotations for each frame in the section
						const Valve::MDL::mstudioanim_valueptr_t* rotValuePtr = anim->GetRotValue();
						TArray<short> xValues, yValues, zValues;
						if (rotValuePtr->offset[0] > 0)
						{
							TArray<const Valve::MDL::mstudioanimvalue_t*> animValues;
							ReadAnimValues(((uint8*)rotValuePtr) + rotValuePtr->offset[0], animDesc->numframes, animValues);
							DecompressAnimValues(animValues, xValues);
						}
						if (rotValuePtr->offset[1] > 0)
						{
							TArray<const Valve::MDL::mstudioanimvalue_t*> animValues;
							ReadAnimValues(((uint8*)rotValuePtr) + rotValuePtr->offset[1], animDesc->numframes, animValues);
							DecompressAnimValues(animValues, yValues);
						}
						if (rotValuePtr->offset[2] > 0)
						{
							TArray<const Valve::MDL::mstudioanimvalue_t*> animValues;
							ReadAnimValues(((uint8*)rotValuePtr) + rotValuePtr->offset[2], animDesc->numframes, animValues);
							DecompressAnimValues(animValues, zValues);
						}
						for (int i = 0; i < sectionFrameCount; ++i)
						{
							FQuat& rot = track.RotKeys[sectionFrameIndex + i];
							FVector angles = FVector::ZeroVector;
							if (rotValuePtr->offset[0] > 0) { angles.X = (xValues.IsValidIndex(i) ? xValues[i] : xValues.Last()) * bone->rotscale[0]; }
							if (rotValuePtr->offset[1] > 0) { angles.Y = (yValues.IsValidIndex(i) ? yValues[i] : yValues.Last()) * bone->rotscale[1]; }
							if (rotValuePtr->offset[2] > 0) { angles.Z = (zValues.IsValidIndex(i) ? zValues[i] : zValues.Last()) * bone->rotscale[2]; }
							check(!angles.ContainsNaN());
							if (!anim->HasFlag(Valve::MDL::mstudioanim_flag::ANIMDELTA))
							{
								angles += FVector(bone->rot[0], bone->rot[1], bone->rot[2]);
							}
							if (isRootBone)
							{
								angles.Z += FMath::DegreesToRadians(-90.0f);
							}
							rot = SourceAnglesToQuat(angles);
						}
					}
					else if (anim->HasFlag(Valve::MDL::mstudioanim_flag::RAWROT))
					{
						// Single rotation covering every frame in the section
						FQuat rot = *anim->GetQuat48();
						if (isRootBone)
						{
							FVector angles = QuatToSourceAngles(rot);
							angles.Z += FMath::DegreesToRadians(-90.0f);
							rot = SourceAnglesToQuat(angles);
						}
						for (int i = 0; i < sectionFrameCount; ++i)
						{
							track.RotKeys[sectionFrameIndex + i] = rot;
						}
					}
					else if (anim->HasFlag(Valve::MDL::mstudioanim_flag::RAWROT2))
					{
						// Single rotation covering every frame in the section
						FQuat rot = *anim->GetQuat64();
						if (isRootBone)
						{
							FVector angles = QuatToSourceAngles(rot);
							angles.Z += FMath::DegreesToRadians(-90.0f);
							rot = SourceAnglesToQuat(angles);
						}
						for (int i = 0; i < sectionFrameCount; ++i)
						{
							track.RotKeys[sectionFrameIndex + i] = rot;
						}
					}
					else
					{
						// Match reference pose for every frame in the section
						FQuat rot;
						if (anim->HasFlag(Valve::MDL::mstudioanim_flag::ANIMDELTA))
						{
							rot = FQuat::Identity;
						}
						else
						{
							rot = FQuat(bone->quat[0], bone->quat[1], bone->quat[2], bone->quat[3]).GetNormalized();
						}
						for (int i = 0; i < sectionFrameCount; ++i)
						{
							track.RotKeys[sectionFrameIndex + i] = rot;
						}
					}

					// Position
					if (anim->HasFlag(Valve::MDL::mstudioanim_flag::ANIMPOS))
					{
						// Compressed positions for each frame in the section
						const Valve::MDL::mstudioanim_valueptr_t* posValuePtr = anim->GetPosValue();
						TArray<short> xValues, yValues, zValues;
						if (posValuePtr->offset[0] > 0)
						{
							TArray<const Valve::MDL::mstudioanimvalue_t*> animValues;
							ReadAnimValues(((uint8*)posValuePtr) + posValuePtr->offset[0], animDesc->numframes, animValues);
							DecompressAnimValues(animValues, xValues);
						}
						if (posValuePtr->offset[1] > 0)
						{
							TArray<const Valve::MDL::mstudioanimvalue_t*> animValues;
							ReadAnimValues(((uint8*)posValuePtr) + posValuePtr->offset[1], animDesc->numframes, animValues);
							DecompressAnimValues(animValues, yValues);
						}
						if (posValuePtr->offset[2] > 0)
						{
							TArray<const Valve::MDL::mstudioanimvalue_t*> animValues;
							ReadAnimValues(((uint8*)posValuePtr) + posValuePtr->offset[2], animDesc->numframes, animValues);
							DecompressAnimValues(animValues, zValues);
						}
						for (int i = 0; i < sectionFrameCount; ++i)
						{
							FVector& pos = track.PosKeys[sectionFrameIndex + i];
							float x = 0.0f, y = 0.0f, z = 0.0f;
							if (posValuePtr->offset[0] > 0) { x = (xValues.IsValidIndex(i) ? xValues[i] : xValues.Last()) * bone->posscale[0]; }
							if (posValuePtr->offset[1] > 0) { y = (yValues.IsValidIndex(i) ? yValues[i] : yValues.Last()) * bone->posscale[1]; }
							if (posValuePtr->offset[2] > 0) { z = (zValues.IsValidIndex(i) ? zValues[i] : zValues.Last()) * bone->posscale[2]; }
							check(!FMath::IsNaN(x));
							check(!FMath::IsNaN(y));
							check(!FMath::IsNaN(z));
							if (!anim->HasFlag(Valve::MDL::mstudioanim_flag::ANIMDELTA))
							{
								x += bone->pos[0];
								y += bone->pos[1];
								z += bone->pos[2];
							}
							if (isRootBone)
							{
								pos = FVector(y, -x, z);
							}
							else
							{
								pos = FVector(x, y, z);
							}
						}
					}
					else if (anim->HasFlag(Valve::MDL::mstudioanim_flag::RAWPOS))
					{
						// Single position covering every frame in the section
						FVector pos = *anim->GetPos();
						if (isRootBone)
						{
							pos = FVector(pos.Y, -pos.X, pos.Z);
						}
						for (int i = 0; i < sectionFrameCount; ++i)
						{
							track.PosKeys[sectionFrameIndex + i] = pos;
						}
					}
					else
					{
						// Match reference pose for every frame in the section
						FVector pos;
						if (anim->HasFlag(Valve::MDL::mstudioanim_flag::ANIMDELTA))
						{
							pos = FVector::ZeroVector;
						}
						else
						{
							pos = FVector(bone->pos[0], bone->pos[1], bone->pos[2]);
						}
						for (int i = 0; i < sectionFrameCount; ++i)
						{
							track.PosKeys[sectionFrameIndex + i] = pos;
						}
					}
				}
			}
			check(framesLeft == 0);

			// Convert sequence tracks into unreal coordinate space
			TMap<uint8, FRawAnimSequenceTrack> transformedBoneTrackMap;
			for (const TPair<uint8, FRawAnimSequenceTrack>& pair : boneTrackMap)
			{
				transformedBoneTrackMap.Add(pair.Key, FRawAnimSequenceTrack());
			}
			for (int frameIndex = 0; frameIndex < animDesc->numframes; ++frameIndex)
			{
				// Merge the anim tracks into the skeleton for this frame
				FReferenceSkeleton frameSkelSource = refSkelSource;
				FSkeletonUtils::MergeAnimTracks(frameSkelSource, boneTrackMap, frameIndex);

				// Transform the skeleton to unreal space
				const FReferenceSkeleton frameSkel = FSkeletonUtils::TransformSkeleton(frameSkelSource, StudioMdlToUnreal);

				// Extract the keyframes out of the transformed skeleton
				FSkeletonUtils::ExtractAnimTracks(frameSkel, transformedBoneTrackMap, frameIndex);
			}

			// Ensure all tracks have the correct number of keys
			for (TPair<uint8, FRawAnimSequenceTrack>& pair : transformedBoneTrackMap)
			{
				const FVector lastPos = pair.Value.PosKeys.Num() > 0 ? pair.Value.PosKeys.Last() : FVector::ZeroVector;
				while (pair.Value.PosKeys.Num() > 1 && pair.Value.PosKeys.Num() < animDesc->numframes)
				{
					pair.Value.PosKeys.Add(lastPos);
				}
				const FQuat lastRot = pair.Value.RotKeys.Num() > 0 ? pair.Value.RotKeys.Last() : FQuat::Identity;
				while (pair.Value.RotKeys.Num() > 1 && pair.Value.RotKeys.Num() < animDesc->numframes)
				{
					pair.Value.RotKeys.Add(lastRot);
				}
				const FVector lastScale = pair.Value.ScaleKeys.Num() > 0 ? pair.Value.ScaleKeys.Last() : FVector::OneVector;
				while (pair.Value.ScaleKeys.Num() > 1 && pair.Value.ScaleKeys.Num() < animDesc->numframes)
				{
					pair.Value.ScaleKeys.Add(lastScale);
				}
			}

			// Add the tracks to the sequence
			sequence->RemoveAllTracks();
			for (auto& pair : transformedBoneTrackMap)
			{
				sequence->AddNewRawTrack(skeletalMesh->RefSkeleton.GetBoneName(pair.Key), &pair.Value);
			}
		}

		outSequences.Add(sequence);
	}

	/*for (const Valve::MDL::mstudioseqdesc_t* localSeq : localSequences)
	{
		FString label = localSeq->GetLabel();
		FString activityName = localSeq->GetActivityName();

		TArray<TArray<int>> animIndices;
		localSeq->GetAnimIndices(animIndices);

		for (int x = 0; x < animIndices.Num(); ++x)
		{
			const TArray<int>& indices = animIndices[x];
			for (int y = 0; y < animIndices.Num(); ++y)
			{
				const int idx = indices[y];
				const Valve::MDL::mstudioanimdesc_t* animDesc = localAnims[idx];


			}
		}


	}*/
}

bool UMDLFactory::SkeletonHasMultipleRoots(const Valve::MDL::studiohdr_t& header)
{
	TArray<const Valve::MDL::mstudiobone_t*> bones;
	header.GetBones(bones);
	return SkeletonHasMultipleRoots(bones);
}

bool UMDLFactory::SkeletonHasMultipleRoots(const TArray<const Valve::MDL::mstudiobone_t*>& bones)
{
	int rootCount = 0;
	for (const Valve::MDL::mstudiobone_t* bone : bones)
	{
		if (bone->parent < 0)
		{
			++rootCount;
		}
	}
	return rootCount > 1;
}

void UMDLFactory::ReadAnimData(const uint8* basePtr, USkeletalMesh* skeletalMesh, TArray<const Valve::MDL::mstudioanim_t*>& out)
{
	const int numBones = skeletalMesh->RefSkeleton.GetRawBoneNum();
	for (int i = 0; i < numBones; ++i)
	{
		const Valve::MDL::mstudioanim_t* anim = (const Valve::MDL::mstudioanim_t*)basePtr;
		if (anim->bone == 255 || anim->bone > numBones) { break; }
		out.Add(anim);
		if (anim->nextoffset == 0) { break; }
		basePtr += anim->nextoffset;
	}
}

void UMDLFactory::ReadAnimValues(uint8* basePtr, int frameCount, TArray<const Valve::MDL::mstudioanimvalue_t*>& animValues)
{
	int frameCountRemainingToBeChecked = frameCount;
	while (frameCountRemainingToBeChecked > 0)
	{
		const Valve::MDL::mstudioanimvalue_t* animValue = (Valve::MDL::mstudioanimvalue_t*)basePtr;
		basePtr += sizeof(Valve::MDL::mstudioanimvalue_t);
		if (animValue->num.total == 0)
		{
			// TODO: Should we ever be getting here?
			break;
		}
		frameCountRemainingToBeChecked -= animValue->num.total;
		animValues.Add(animValue);
		for (int i = 0; i < animValue->num.valid; ++i)
		{
			animValues.Add((Valve::MDL::mstudioanimvalue_t*)basePtr);
			basePtr += sizeof(Valve::MDL::mstudioanimvalue_t);
		}
	}
}

void UMDLFactory::DecompressAnimValues(const TArray<const Valve::MDL::mstudioanimvalue_t*> animValues, TArray<short>& outValues)
{
	outValues.Empty();
	int i = 0;
	while (animValues.IsValidIndex(i))
	{
		const Valve::MDL::mstudioanimvalue_t& rootAnimValue = *animValues[i++];
		outValues.Reserve(outValues.Num() + rootAnimValue.num.total);
		for (int j = 0; j < rootAnimValue.num.valid; ++j)
		{
			const Valve::MDL::mstudioanimvalue_t& childAnimValue = *animValues[i++];
			outValues.Add(childAnimValue.value);
		}
		if (rootAnimValue.num.total > rootAnimValue.num.valid)
		{
			const Valve::MDL::mstudioanimvalue_t& lastAnimValue = *animValues[i - 1];
			for (int j = 0; j < (rootAnimValue.num.total - rootAnimValue.num.valid); ++j)
			{
				outValues.Add(lastAnimValue.value);
			}
		}
	}
}

void UMDLFactory::ReadPHYSolid(uint8*& basePtr, TArray<FPHYSection>& out)
{
	const Valve::PHY::compactsurfaceheader_t& newSolid = *((Valve::PHY::compactsurfaceheader_t*)basePtr);
	uint8* endPtr;
	if (newSolid.vphysicsID != *(int*)"VPHY")
	{
		const Valve::PHY::legacysurfaceheader_t& oldSolid = *((Valve::PHY::legacysurfaceheader_t*)basePtr);
		endPtr = basePtr + oldSolid.size + sizeof(int);
		basePtr += sizeof(Valve::PHY::legacysurfaceheader_t);
	}
	else
	{
		endPtr = basePtr + newSolid.size + sizeof(int);
		basePtr += sizeof(Valve::PHY::compactsurfaceheader_t);
	}
	uint8* vertPtr = endPtr;

	// Read all triangles
	out.Empty();
	TArray<TSet<uint16>> uniqueVertsPerSection;
	int numVerts = -1;
	while (basePtr < vertPtr)
	{
		TSet<uint16>& uniqueVerts = uniqueVertsPerSection.AddDefaulted_GetRef();

		// Read header and advance pointer past it
		const Valve::PHY::sectionheader_t& sectionHeader = *((Valve::PHY::sectionheader_t*)basePtr);
		vertPtr = basePtr + sectionHeader.vertexDataOffset;
		basePtr += sizeof(Valve::PHY::sectionheader_t);

		// Read triangles and advance pointer past them
		TArray<Valve::PHY::triangle_t> triangles;
		basePtr += sectionHeader.GetTriangles(triangles);

		// Start a new section
		FPHYSection section;
		if (sectionHeader.boneIndex <= 0)
		{
			section.BoneIndex = 0;
			section.IsCollisionModel = true;
		}
		else
		{
			section.BoneIndex = sectionHeader.boneIndex - 1;
			section.IsCollisionModel = false;
		}
		section.FaceIndices.Reserve(triangles.Num() * 3);

		// Iterate all triangles, gather new unique verts and face indices
		for (const Valve::PHY::triangle_t& triangle : triangles)
		{
			uniqueVerts.Add(triangle.vertices[0].index);
			uniqueVerts.Add(triangle.vertices[1].index);
			uniqueVerts.Add(triangle.vertices[2].index);
			numVerts = FMath::Max(numVerts, (int)FMath::Max3(triangle.vertices[0].index, triangle.vertices[1].index, triangle.vertices[2].index));
			section.FaceIndices.Add(triangle.vertices[2].index);
			section.FaceIndices.Add(triangle.vertices[1].index);
			section.FaceIndices.Add(triangle.vertices[0].index);
		}

		// Store section
		out.Add(section);
	}
	++numVerts;

	// Read all vertices
	check(basePtr == vertPtr);
	TArray<FVector4> rawVerts;
	rawVerts.Reserve(numVerts);
	for (int i = 0; i < numVerts; ++i)
	{
		rawVerts.Add(((FVector4*)basePtr)[i]);
	}
	basePtr += sizeof(FVector4) * numVerts;

	// Fixup sections
	for (FPHYSection& section : out)
	{
		// Fix vertices
		for (int i = 0; i < section.FaceIndices.Num(); ++i)
		{
			// Lookup face index and vertex
			const int idx = section.FaceIndices[i];
			const FVector4 rawVert = rawVerts[idx];

			// Convert to UE4 vertex
			FVector fixedVert;
			if (section.IsCollisionModel)
			{
				fixedVert.X = (100.0f / 2.54f) * rawVert.Z;
				fixedVert.Y = (100.0f / 2.54f) * -rawVert.X;
				fixedVert.Z = (100.0f / 2.54f) * -rawVert.Y;
			}
			else
			{
				fixedVert.X = (100.0f / 2.54f) * rawVert.X;
				fixedVert.Y = (100.0f / 2.54f) * rawVert.Z;
				fixedVert.Z = (100.0f / 2.54f) * -rawVert.Y;
			}

			// Store the vertex uniquely to the section and rewire the face index to point at it
			section.FaceIndices[i] = section.Vertices.AddUnique(fixedVert);
		}
	}

	// There is sometimes trailing data, skip it
	check(basePtr <= endPtr);
	basePtr = endPtr;
}

void UMDLFactory::ResolveMaterials(const Valve::MDL::studiohdr_t& header, TArray<FName>& out)
{
	// Read texture dirs
	TArray<FString> textureDirs;
	header.GetTextureDirs(textureDirs);

	// Read textures
	TArray<const Valve::MDL::mstudiotexture_t*> textures;
	header.GetTextures(textures);

	// Resolve materials
	TArray<FName> materials;
	materials.Reserve(textures.Num());
	IHL2Runtime& hl2Runtime = IHL2Runtime::Get();
	for (int textureIndex = 0; textureIndex < textures.Num(); ++textureIndex)
	{
		FString materialName = textures[textureIndex]->GetName();

		// Search each textureDir for it
		bool found = false;
		for (int i = 0; i < textureDirs.Num(); ++i)
		{
			FString materialPath = textureDirs[i] / materialName;
			materialPath.ReplaceCharInline('\\', '/');
			if (hl2Runtime.TryResolveHL2Material(materialPath) != nullptr)
			{
				out.Add(FName(*materialPath));
				found = true;
				break;
			}
		}
		if (!found)
		{
			// Not found but we can't skip entries as the order of the array matters, so insert it with no path
			out.Add(FName(*materialName));
		}
	}
}

void UMDLFactory::ReadSkins(const Valve::MDL::studiohdr_t& header, TArray<TArray<int>>& out)
{
	const uint16* basePtr = (uint16*)(((uint8*)& header) + header.skinreference_index);
	for (int familyIndex = 0; familyIndex < header.skinrfamily_count; ++familyIndex)
	{
		const uint16* familyPtr = basePtr + familyIndex * header.skinreference_count;
		TArray<int> mapping;
		for (int skinRefIndex = 0; skinRefIndex < FMath::Min(header.skinreference_count, header.texture_count); ++skinRefIndex)
		{
			mapping.Add((int)familyPtr[skinRefIndex]);
		}
		out.Add(mapping);
	}
}

FQuat UMDLFactory::SourceAnglesToQuat(const FVector& angles)
{
	// https://marc-b-reynolds.github.io/math/2017/04/18/TaitEuler.html
	const double hy = 0.5 * angles.Z;
	const double hp = 0.5 * angles.Y;
	const double hr = 0.5 * angles.X;
	const double ys = sin(hy), yc = cos(hy);
	const double ps = sin(hp), pc = cos(hp);
	const double rs = sin(hr), rc = cos(hr);

	FQuat quat;
	quat.W = (float)(rc * pc * yc + rs * ps * ys);
	quat.X = (float)(rs * pc * yc - rc * ps * ys);
	quat.Y = (float)(rc * ps * yc + rs * pc * ys);
	quat.Z = (float)(rc * pc * ys - rs * ps * yc);
	return quat.GetNormalized();
}

FVector UMDLFactory::QuatToSourceAngles(const FQuat& quat)
{
	// https://marc-b-reynolds.github.io/math/2017/04/18/TaitEuler.html
	const double x = quat.X, y = quat.Y, z = quat.Z, w = quat.W;

	const double t0 = x * x - z * z;
	const double t1 = w * w - y * y;
	const double xx = 0.5 * (t0 + t1);        // 1/2 x of x'
	const double xy = x * y + w * z;            // 1/2 y of x'
	const double xz = w * y - x * z;            // 1/2 z of x'
	const double t = xx * xx + xy * xy;        // cos(theta)^2
	const double yz = 2.0 * (y * z + w * x);      // z of y'

	FVector v;
	v.Z = (float)atan2(xy, xx);    // yaw   (psi)
	v.Y = (float)atan(xz / sqrt(t)); // pitch (theta)
	if (t != 0)
	{
		v.X = (float)atan2(yz, t1 - t0);
	}
	else
	{
		v.X = (float)(2.0 * atan2(x, w) - FMath::Sign(xz) * v.Z);
	}
	return v;
}