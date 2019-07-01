#include "HL2EditorPrivatePCH.h"

#include "MDLFactory.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/FeedbackContext.h"
#include "AssetImportTask.h"
#include "FileHelper.h"
#include "IHL2Runtime.h"
#include "MeshUtils.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "Rendering/SkeletalMeshModel.h"
#include "HL2ModelData.h"
#include "MeshUtilities.h"
#include "MeshDescriptionOperations.h"
#include "MeshUtilitiesCommon.h"
#include "OverlappingCorners.h"
#include "ValveKeyValues.h"

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
		return task->Result;
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
		if (phyHeader->checkSum != header.checksum)
		{
			warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: VVD Header has invalid checksum (expecting %d, got %d)"), header.checksum, vvdHeader.checksum);
			return result;
		}
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
	animPackagePath.Append(TEXT("_a_"));
	ImportSequences(header, result.SkeletalMesh, animPackagePath, aniHeader, result.Animations, warn);

	// TODO: Search for other mdl files that might contain animation data (e.g. alyx.mdl -> alyx_animations.mdl, alyx_gestures.mdl, alyx_postures.mdl)

	for (UAnimSequence* sequence : result.Animations)
	{
		sequence->PostEditChange();
		FAssetRegistryModule::AssetCreated(sequence);
		sequence->MarkPackageDirty();
	}

	return result;
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
			UStaticMesh::RegisterMeshAttributes(meshDescription);
			polyGroupMaterial = meshDescription.PolygonGroupAttributes().GetAttributesRef<FName>(MeshAttribute::PolygonGroup::ImportedMaterialSlotName);
			vertPos = meshDescription.VertexAttributes().GetAttributesRef<FVector>(MeshAttribute::Vertex::Position);
			vertInstNormal = meshDescription.VertexInstanceAttributes().GetAttributesRef<FVector>(MeshAttribute::VertexInstance::Normal);
			vertInstTangent = meshDescription.VertexInstanceAttributes().GetAttributesRef<FVector>(MeshAttribute::VertexInstance::Tangent);
			vertInstUV0 = meshDescription.VertexInstanceAttributes().GetAttributesRef<FVector2D>(MeshAttribute::VertexInstance::TextureCoordinate);
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

								const uint16 idxs[3] =
								{
									indices[strip.indexOffset + i],
									indices[strip.indexOffset + i + 1],
									indices[strip.indexOffset + i + 2]
								};

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
									localMeshData.vertPos[vertID] = vvdVertex.m_vecPosition;

									FVertexInstanceID vertInstID = localMeshData.meshDescription.CreateVertexInstance(vertID);
									localMeshData.vertInstNormal[vertInstID] = vvdVertex.m_vecNormal;
									localMeshData.vertInstTangent[vertInstID] = FVector(vvdTangent.X, vvdTangent.Y, vvdTangent.Z);
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
				FMeshDescriptionOperations::FindOverlappingCorners(overlappingCorners, localMeshDescription, 0.00001f);
				FMeshDescriptionOperations::CreateLightMapUVLayout(localMeshDescription, settings.SrcLightmapIndex, settings.DstLightmapIndex, settings.MinLightmapResolution, ELightmapUVVersion::Latest, overlappingCorners);
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
				staticMesh->SectionInfoMap.Set(lodIndex, sectionIdx, FMeshSectionInfo(sectionIdx));
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
				staticMesh->SectionInfoMap.Set(0, meshSlot, FMeshSectionInfo(meshSlot));
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
	struct LocalMeshData
	{
		TArray<FVector> points;
		TArray<SkeletalMeshImportData::FMeshWedge> wedges;
		TArray<SkeletalMeshImportData::FMeshFace> faces;
		TArray<SkeletalMeshImportData::FVertInfluence> influences;
		TArray<int32> pointToRawMap;
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

	// Read bones
	TArray<const Valve::MDL::mstudiobone_t*> bones;
	header.GetBones(bones);

	// Create skeleton
	USkeleton* skeleton = CreateSkeleton(inSkeletonParent, inSkeletonName, flags);

	// Create skeletal mesh
	USkeletalMesh* skeletalMesh = CreateSkeletalMesh(inParent, inName, flags);
	skeletalMesh->Skeleton = skeleton;

	// Create model data
	UHL2ModelData* modelData = NewObject<UHL2ModelData>(skeletalMesh);
	skeletalMesh->AddAssetUserData(modelData);

	// Load bones into skeleton
	FReferenceSkeleton refSkel(false);
	{
		FReferenceSkeletonModifier refSkelMod(refSkel, skeleton);
		refSkel.Empty(bones.Num());
		for (int i = 0; i < bones.Num(); ++i)
		{
			const Valve::MDL::mstudiobone_t& bone = *bones[i];
			FMeshBoneInfo meshBoneInfo;
			meshBoneInfo.ParentIndex = bone.parent;
			meshBoneInfo.ExportName = bone.GetName();
			meshBoneInfo.Name = FName(*meshBoneInfo.ExportName);

			FTransform bonePose;
			bonePose.SetLocation(FVector(bone.pos[0], bone.pos[1], bone.pos[2]));
			bonePose.SetRotation(FQuat(bone.quat[0], bone.quat[1], bone.quat[2], bone.quat[3]));
			refSkelMod.Add(meshBoneInfo, bonePose);
		}
	}
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
				LocalMeshData& localMeshData = localMeshDatas[lodIndex];
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
								const uint16 idxs[3] =
								{
									indices[strip.indexOffset + i],
									indices[strip.indexOffset + i + 1],
									indices[strip.indexOffset + i + 2]
								};

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

								SkeletalMeshImportData::FMeshFace face;
								face.MeshMaterialIndex = (uint16)sectionIndex;
								face.SmoothingGroups = 0;
								for (int j = 0; j < 3; ++j)
								{
									const Valve::VTX::Vertex_t& vert = *verts[j];
									const Valve::VVD::mstudiovertex_t& origVert = *origVerts[j];

									const int vertIdx = localMeshData.points.Add(origVert.m_vecPosition);
									localMeshData.pointToRawMap.Add(baseIdxs[j]);

									SkeletalMeshImportData::FMeshWedge wedge;
									wedge.iVertex = vertIdx;
									wedge.Color = FColor::White;
									wedge.UVs[0] = origVert.m_vecTexCoord;
									face.iWedge[j] = localMeshData.wedges.Add(wedge);
									face.TangentZ[j] = origVert.m_vecNormal.GetSafeNormal();
									face.TangentY[j] = vvdTangents[baseIdxs[j]].GetSafeNormal();
									face.TangentX[j] = FVector::CrossProduct(face.TangentY[j], face.TangentZ[j]);
									
									const int numBones = FMath::Min((int)origVert.m_BoneWeights.numbones, Valve::VVD::MAX_NUM_BONES_PER_VERT);
									for (int k = 0; k < numBones; ++k)
									{
										const uint8 boneWeightIdx = vert.boneWeightIndex[k];
										const int boneID = origVert.m_BoneWeights.bone[boneWeightIdx];
										// check(vert.boneID[k] == boneID);
										const float weight = origVert.m_BoneWeights.weight[boneWeightIdx];
										SkeletalMeshImportData::FVertInfluence influence;
										influence.BoneIndex = boneID;
										influence.VertIndex = wedge.iVertex;
										influence.Weight = weight;
										localMeshData.influences.Add(influence);
									}
								}
								localMeshData.faces.Add(face);
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
	IMeshUtilities::MeshBuildOptions buildOptions;
	buildOptions.bComputeNormals = false;
	buildOptions.bComputeTangents = false;
	buildOptions.bRemoveDegenerateTriangles = false;
	IMeshUtilities& meshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	for (int lodIndex = 0; lodIndex < localMeshDatas.Num(); ++lodIndex)
	{
		const LocalMeshData& localMeshData = localMeshDatas[lodIndex];

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
			skeletalMesh->SetImportedBounds(FBoxSphereBounds(&localMeshData.points[0], (uint32)localMeshData.points.Num()));
		}
		LODModel.NumTexCoords = 1;

		// Build
		TArray<FText> warningMessages;
		TArray<FName> warningNames;
		if (!meshUtilities.BuildSkeletalMesh(meshModel->LODModels[lodIndex], skeletalMesh->RefSkeleton, localMeshData.influences, localMeshData.wedges, localMeshData.faces, localMeshData.points, localMeshData.pointToRawMap, buildOptions, &warningMessages, &warningNames))
		{
			warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: Skeletal mesh build for lod %d failed"), lodIndex);
			return nullptr;
		}

		// Calculate required bones
		USkeletalMesh::CalculateRequiredBones(LODModel, refSkel, nullptr);
	}
	skeletalMesh->CalculateInvRefMatrices();

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
		// Create phys asset
		UPhysicsAsset* physicsAsset = CreatePhysAsset(inPhysAssetParent, inPhysAssetName, flags);

		// Read solids
		uint8* curPtr = ((uint8*)phyHeader) + phyHeader->size;
		for (int i = 0; i < phyHeader->solidCount; ++i)
		{
			TArray<FPHYSection> sections;
			ReadPHYSolid(curPtr, sections);

			for (int sectionIndex = 0; sectionIndex < sections.Num(); ++sectionIndex)
			{
				const FPHYSection& section = sections[sectionIndex];

				USkeletalBodySetup* bodySetup = NewObject<USkeletalBodySetup>(physicsAsset, FName(*FString::Printf(TEXT("%d:%d"), i, sectionIndex)));
				bodySetup->BoneName = FName(*bones[0]->GetName());
				bodySetup->bGenerateMirroredCollision = true;
				bodySetup->bConsiderForBounds = true;
				FMeshUtils::DecomposeUCXMesh(section.Vertices, section.FaceIndices, bodySetup);
				physicsAsset->SkeletalBodySetups.Add(bodySetup);
			}
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

		// Parse solids
		TArray<UValveValue*> solidValues;
		static const FName fnSolid(TEXT("solid"));
		root->GetItems(fnSolid, solidValues);
		for (const UValveValue* solidValue : solidValues)
		{
			const UValveGroupValue* solidGroupValue = CastChecked<UValveGroupValue>(solidValue);
			int index;
			static const FName fnIndex(TEXT("index"));
			if (!solidGroupValue->GetInt(fnIndex, index))
			{
				warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: PHY solid missing index"));
				continue;
			}
			if (!physicsAsset->SkeletalBodySetups.IsValidIndex(index))
			{
				warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: PHY solid refers to invalid index %i"), index);
				continue;
			}
			FName name;
			static const FName fnName(TEXT("name"));
			if (!solidGroupValue->GetName(fnName, name))
			{
				warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: PHY solid %i missing name"), index);
				continue;
			}
			USkeletalBodySetup* bodySetup = physicsAsset->SkeletalBodySetups[index];
			bodySetup->BoneName = name;
			FName surfaceProp;
			static const FName fnSurfaceProp(TEXT("surfaceprop"));
			if (solidGroupValue->GetName(fnSurfaceProp, surfaceProp))
			{
				bodySetup->PhysMaterial = IHL2Runtime::Get().TryResolveHL2SurfaceProp(surfaceProp);
			}
		}

		// Parse constraints
		TArray<UValveValue*> constraintValues;
		static const FName fnRagdollConstraint(TEXT("ragdollconstraint"));
		root->GetItems(fnRagdollConstraint, constraintValues);
		for (const UValveValue* constraintValue : constraintValues)
		{
			const UValveGroupValue* constraintGroupValue = CastChecked<UValveGroupValue>(constraintValue);

			int parent;
			static const FName fnParent(TEXT("parent"));
			if (!constraintGroupValue->GetInt(fnParent, parent))
			{
				warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: PHY constraint missing parent"));
				continue;
			}

			int child;
			static const FName fnChild(TEXT("child"));
			if (!constraintGroupValue->GetInt(fnChild, child))
			{
				warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: PHY constraint missing child"));
				continue;
			}

			USkeletalBodySetup* parentBodySetup = physicsAsset->SkeletalBodySetups[parent];
			USkeletalBodySetup* childBodySetup = physicsAsset->SkeletalBodySetups[child];

			const FTransform& parentBoneTransform = skeletalMesh->RefSkeleton.GetRefBonePose()[parent];
			const FTransform& childBoneTransform = skeletalMesh->RefSkeleton.GetRefBonePose()[child];

			const FVector loc = FMath::Lerp(parentBoneTransform.GetLocation(), childBoneTransform.GetLocation(), 0.5f);

			UPhysicsConstraintTemplate* constraintTemplate = NewObject<UPhysicsConstraintTemplate>(physicsAsset);
			FConstraintInstance& constraint = constraintTemplate->DefaultInstance;

			constraint.ConstraintBone1 = refSkel.GetBoneName(parent);
			constraint.ConstraintBone2 = refSkel.GetBoneName(child);

			constraint.ConstraintIndex = physicsAsset->ConstraintSetup.Add(constraintTemplate);
		}

		physicsAsset->PreviewSkeletalMesh = skeletalMesh;
		skeletalMesh->PhysicsAsset = physicsAsset;
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
	modelData->PostEditChange();
	modelData->MarkPackageDirty();

	// Done
	return skeletalMesh;
}

void UMDLFactory::ImportSequences(const Valve::MDL::studiohdr_t& header, USkeletalMesh* skeletalMesh, const FString& basePath, const Valve::MDL::studiohdr_t* aniHeader, TArray<UAnimSequence*>& outSequences, FFeedbackContext* warn)
{
	TArray<const Valve::MDL::mstudioanimdesc_t*> localAnims;
	header.GetLocalAnims(localAnims);
	TArray<const Valve::MDL::mstudioseqdesc_t*> localSequences;
	header.GetLocalSequences(localSequences);
	TArray<const Valve::MDL::mstudioanimblock_t*> animBlocks;
	header.GetAnimBlocks(animBlocks);

	for (const Valve::MDL::mstudioanimdesc_t* animDesc : localAnims)
	{
		FString name = animDesc->GetName();
		name.RemoveFromStart(TEXT("@"));

		UPackage* package = CreatePackage(nullptr, *(basePath + name));
		UAnimSequence* sequence = CreateAnimSequence(package, FName(*FPaths::GetBaseFilename(basePath + name)), RF_Public | RF_Standalone);
		sequence->SetSkeleton(skeletalMesh->Skeleton);
		sequence->SetRawNumberOfFrame(animDesc->numframes);
		sequence->SequenceLength = animDesc->numframes / (float)animDesc->fps;

		if (!animDesc->HasFlag(Valve::MDL::mstudioanimdesc_flag::ALLZEROS))
		{
			TArray<TArray<const Valve::MDL::mstudioanim_t*>> anims;

			if (animDesc->sectionindex != 0 && animDesc->sectionframes > 0)
			{
				TArray<const Valve::MDL::mstudioanimsections_t*> sections;
				animDesc->GetSections(sections);
				anims.AddDefaulted(sections.Num());
				if (animDesc->animblock == 0)
				{
					for (int sectionIndex = 0; sectionIndex < sections.Num(); ++sectionIndex)
					{
						const Valve::MDL::mstudioanimsections_t* section = sections[sectionIndex];
						TArray<const Valve::MDL::mstudioanim_t*>& sectionAnims = anims[sectionIndex];
						const uint8* basePtr = ((uint8*)animDesc) + section->animindex;
						int sectionFrameCount;
						if (sectionIndex < sections.Num() - 2)
						{
							sectionFrameCount = animDesc->sectionframes;
						}
						else
						{
							sectionFrameCount = animDesc->numframes - ((sections.Num() - 2) * animDesc->sectionframes);
						}
						ReadAnimData(basePtr, animDesc, sectionFrameCount, sectionAnims, aniHeader, skeletalMesh);
					}
				}
				else
				{
					check(aniHeader != nullptr);
					const Valve::MDL::mstudioanimblock_t* block = animBlocks[animDesc->animblock];
					// TODO: Load from ani
					// Can we ever get here?
				}
			}
			else
			{
				const uint8* basePtr;
				if (animDesc->animblock == 0)
				{
					basePtr = ((uint8*)animDesc) + animDesc->animindex;
				}
				else
				{
					check(aniHeader != nullptr);
					const Valve::MDL::mstudioanimblock_t* block = animBlocks[animDesc->animblock];
					basePtr = ((uint8*)aniHeader) + block->datastart;
				}
				anims.AddDefaulted();
				ReadAnimData(basePtr, animDesc, animDesc->numframes, anims[0], aniHeader, skeletalMesh);
			}

			// Load anims into sequence
			int frameIndex = 0;
			TMap<uint8, FRawAnimSequenceTrack> boneTrackMap;
			for (const TArray<const Valve::MDL::mstudioanim_t*>& frameBones : anims)
			{
				for (const Valve::MDL::mstudioanim_t* anim : frameBones)
				{
					FRawAnimSequenceTrack& track = boneTrackMap.FindOrAdd(anim->bone);
					
					// We might not have seen any data for this bone for a while, so play catchup
					const FVector& lastPos = track.PosKeys.Num() > 0 ? track.PosKeys.Last(0) : FVector::ZeroVector;
					const FQuat& lastRot = track.RotKeys.Num() > 0 ? track.RotKeys.Last(0) : FQuat::Identity;
					for (int i = track.PosKeys.Num() + 1; i < frameIndex - 1; ++i)
					{
						track.PosKeys.Add(lastPos);
					}
					for (int i = track.RotKeys.Num() + 1; i < frameIndex - 1; ++i)
					{
						track.RotKeys.Add(lastRot);
					}

					// Rotation
					if (anim->HasFlag(Valve::MDL::mstudioanim_flag::STUDIO_ANIM_ANIMROT))
					{
						const Valve::MDL::mstudioanimvalue_t* rotValue = anim->GetRotValue()->GetAnimValue(0);
						track.RotKeys.Add(lastRot);
					}
					else if (anim->HasFlag(Valve::MDL::mstudioanim_flag::STUDIO_ANIM_RAWROT))
					{
						track.RotKeys.Add(lastRot);
					}
					else if (anim->HasFlag(Valve::MDL::mstudioanim_flag::STUDIO_ANIM_RAWROT2))
					{
						track.RotKeys.Add(lastRot);
					}
					else
					{
						track.RotKeys.Add(lastRot);
					}

					// Position
					if (anim->HasFlag(Valve::MDL::mstudioanim_flag::STUDIO_ANIM_ANIMPOS))
					{
						const Valve::MDL::mstudioanimvalue_t* posValue = anim->GetPosValue()->GetAnimValue(0);
						track.PosKeys.Add(lastPos);

					}
					else if (anim->HasFlag(Valve::MDL::mstudioanim_flag::STUDIO_ANIM_RAWPOS))
					{
						track.PosKeys.Add(lastPos);
					}
					else
					{
						track.PosKeys.Add(lastPos);
					}
				}
				++frameIndex;
			}
			sequence->RemoveAllTracks();
			for (auto& pair : boneTrackMap)
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

void UMDLFactory::ReadAnimData(const uint8* basePtr, const Valve::MDL::mstudioanimdesc_t* animDesc, int frameCount, TArray<const Valve::MDL::mstudioanim_t*>& out, const Valve::MDL::studiohdr_t* aniHeader, USkeletalMesh* skeletalMesh)
{
	const int numBones = skeletalMesh->RefSkeleton.GetRawBoneNum();
	for (int i = 0; i < numBones; ++i)
	{
		const Valve::MDL::mstudioanim_t* anim = (const Valve::MDL::mstudioanim_t*)basePtr;
		if (anim->bone == 255 || anim->bone > numBones || anim->nextoffset == 0) { break; }
		out.Add(anim);
		if (anim->HasFlag(Valve::MDL::mstudioanim_flag::STUDIO_ANIM_ANIMROT))
		{
			const Valve::MDL::mstudioanim_valueptr_t* rotValuePtr = anim->GetRotValue();

		}
		if (anim->HasFlag(Valve::MDL::mstudioanim_flag::STUDIO_ANIM_ANIMPOS))
		{
			const Valve::MDL::mstudioanim_valueptr_t* posValuePtr = anim->GetPosValue();

		}
		basePtr += anim->nextoffset;
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

	TSet<uint16> uniqueVerts;

	// Read all triangles
	while (basePtr < vertPtr)
	{
		// Read header and advance pointer past it
		const Valve::PHY::sectionheader_t& sectionHeader = *((Valve::PHY::sectionheader_t*)basePtr);
		vertPtr = basePtr + sectionHeader.vertexDataOffset;
		basePtr += sizeof(Valve::PHY::sectionheader_t);

		// Read triangles and advance pointer past them
		TArray<Valve::PHY::triangle_t> triangles;
		basePtr += sectionHeader.GetTriangles(triangles);

		// Start a new section
		FPHYSection section;
		section.BoneIndex = sectionHeader.boneIndex;
		section.FaceIndices.Reserve(triangles.Num() * 3);

		// Iterate all triangles, gather new unique verts and face indices
		for (const Valve::PHY::triangle_t& triangle : triangles)
		{
			uniqueVerts.Add(triangle.vertices[0].index);
			uniqueVerts.Add(triangle.vertices[2].index);
			uniqueVerts.Add(triangle.vertices[1].index);
			section.FaceIndices.Add(triangle.vertices[0].index);
			section.FaceIndices.Add(triangle.vertices[2].index);
			section.FaceIndices.Add(triangle.vertices[1].index);
		}

		// Store section
		out.Add(section);
	}

	// Read all vertices
	check(basePtr == vertPtr);
	TArray<FVector> vertices;
	for (int j = 0; j < uniqueVerts.Num(); ++j)
	{
		const FVector4 vertex = *((FVector4*)basePtr); basePtr += sizeof(FVector4);
		FVector tmp;
		tmp.X = 1.0f / 0.0254f * vertex.X;
		tmp.Y = 1.0f / 0.0254f * vertex.Z;
		tmp.Z = 1.0f / 0.0254f * -vertex.Y;
		vertices.Add(tmp);
	}

	// There is sometimes trailing data, skip it
	check(basePtr <= endPtr);
	basePtr = endPtr;

	// Go back through sections and fix them up
	for (FPHYSection& section : out)
	{
		for (int i = 0; i < section.FaceIndices.Num(); ++i)
		{
			// Lookup face index and vertex
			const int idx = section.FaceIndices[i];
			const FVector vert = vertices[idx];

			// Store the vertex uniquely to the section and rewire the face index to point at it
			section.FaceIndices[i] = section.Vertices.AddUnique(vert);
		}
	}
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