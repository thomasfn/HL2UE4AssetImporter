#include "HL2EditorPrivatePCH.h"

#include "MDLFactory.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/FeedbackContext.h"
#include "AssetImportTask.h"
#include "FileHelper.h"
#include "IHL2Runtime.h"
#include "MeshUtils.h"
#include "PhysicsEngine/BodySetup.h"

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
	if (header.version != 44)
	{
		warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: MDL Header has invalid version (expecting 44, got %d)"), header.version);
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
	

	// Static mesh
	if (header.HasFlag(Valve::MDL::studiohdr_flag::STATIC_PROP))
	{
		result.StaticMesh = ImportStaticMesh(inParent, inName, flags, header, vtxHeader, vvdHeader, phyHeader, warn);
		return result;
	}

	warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: Animated models not yet supported"));

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

	// Read textures
	TArray<FString> textureDirs;
	header.GetTextureDirs(textureDirs);
	TArray<const Valve::MDL::mstudiotexture_t*> textures;
	header.GetTextures(textures);
	if (textureDirs.Num() < 1)
	{
		warn->Logf(ELogVerbosity::Error, TEXT("ImportStudioModel: No texture dirs present (%d)"), textures.Num(), textureDirs.Num());
		return nullptr;
	}
	TArray<FName> materials;
	materials.Reserve(textures.Num());
	for (int textureIndex = 0; textureIndex < textures.Num(); ++textureIndex)
	{
		FString materialPath = (textureIndex >= textureDirs.Num() ? textureDirs.Top() : textureDirs[textureIndex]) / textures[textureIndex]->GetName();
		materialPath.ReplaceCharInline('\\', '/');
		materials.Add(FName(*materialPath));
	}

	// Iterate all body parts
	uint16 accumIndex = 0;
	for (int bodyPartIndex = 0; bodyPartIndex < bodyParts.Num(); ++bodyPartIndex)
	{
		const Valve::MDL::mstudiobodyparts_t& bodyPart = *bodyParts[bodyPartIndex];
		const Valve::VTX::BodyPartHeader_t& vtxBodyPart = *vtxBodyParts[bodyPartIndex];

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
				{
					const uint16 vertCount = (uint16)vvdHeader.numLODVertexes[0];
					TArray<Valve::VVD::mstudiovertex_t> vertices;
					vertices.Reserve(vertCount);
					TArray<FVector4> tangents;
					tangents.Reserve(vertCount);

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
									vertices.Add(vertex);
									tangents.Add(tangent);
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
							vertices.Add(vertex);
							tangents.Add(tangent);
						}
					}

					// Write to mesh and store in map
					for (uint16 i = 0; i < vertCount; ++i)
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
					
					// Create poly group
					const FPolygonGroupID polyGroupID = localMeshData.meshDescription.CreatePolygonGroup();
					localMeshData.polyGroupMaterial[polyGroupID] = materials[mesh.material];

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

								const uint16 idx0 = indices[strip.indexOffset + i];
								const uint16 idx1 = indices[strip.indexOffset + i + 1];
								const uint16 idx2 = indices[strip.indexOffset + i + 2];

								const Valve::VTX::Vertex_t& vert0 = vertices[idx0];
								const Valve::VTX::Vertex_t& vert1 = vertices[idx1];
								const Valve::VTX::Vertex_t& vert2 = vertices[idx2];

								const uint16 baseIdx0 = accumIndex + mesh.vertexoffset + vert0.origMeshVertID;
								const uint16 baseIdx1 = accumIndex + mesh.vertexoffset + vert1.origMeshVertID;
								const uint16 baseIdx2 = accumIndex + mesh.vertexoffset + vert2.origMeshVertID;

								tmpVertInstIDs.Add(localMeshData.vertexInstanceMap[baseIdx0]);
								tmpVertInstIDs.Add(localMeshData.vertexInstanceMap[baseIdx1]);
								tmpVertInstIDs.Add(localMeshData.vertexInstanceMap[baseIdx2]);

								// Sanity check: if one or more vertex instances share the same base vertex, this poly is degenerate so reject it
								const FVertexID baseVert0ID = localMeshData.meshDescription.GetVertexInstanceVertex(tmpVertInstIDs[0]);
								const FVertexID baseVert1ID = localMeshData.meshDescription.GetVertexInstanceVertex(tmpVertInstIDs[1]);
								const FVertexID baseVert2ID = localMeshData.meshDescription.GetVertexInstanceVertex(tmpVertInstIDs[2]);
								if (baseVert0ID == baseVert1ID || baseVert0ID == baseVert2ID || baseVert1ID == baseVert2ID) { continue; }

								FPolygonID polyID = localMeshData.meshDescription.CreatePolygon(polyGroupID, tmpVertInstIDs);
							}
						}
					}
				}
			}

			accumIndex += model.numvertices;
		}
	}

	// Construct static mesh
	UStaticMesh* staticMesh = CreateStaticMesh(inParent, inName, flags);
	if (staticMesh == nullptr) { return nullptr; }
	staticMesh->LightMapResolution = 64;

	constexpr bool debugPhysics = false; // if true, the physics mesh is rendered instead

	// Write all lods to the static mesh
	if (!debugPhysics || phyHeader == nullptr)
	{
		TMap<FName, int> materialSlots;
		for (const FName material : materials)
		{
			materialSlots.Add(material, staticMesh->StaticMaterials.Emplace(nullptr, material, material));
		}
		for (int lodIndex = 0; lodIndex < vtxHeader.numLODs; ++lodIndex)
		{
			// Setup source model
			FStaticMeshSourceModel& sourceModel = staticMesh->AddSourceModel();
			FMeshBuildSettings& settings = sourceModel.BuildSettings;
			settings.bRecomputeNormals = false;
			settings.bRecomputeTangents = false;
			settings.bGenerateLightmapUVs = true;
			settings.SrcLightmapIndex = 0;
			settings.DstLightmapIndex = 1;
			settings.bRemoveDegenerates = false;
			settings.bUseFullPrecisionUVs = true;
			settings.MinLightmapResolution = 64;
			sourceModel.ScreenSize.Default = FMath::Pow(0.5f, lodIndex + 1.0f);

			// Fetch and prepare our mesh description
			FMeshDescription& localMeshDescription = localMeshDatas[lodIndex].meshDescription;
			FMeshUtils::Clean(localMeshDescription);

			// Copy to static mesh
			FMeshDescription* meshDescription = staticMesh->CreateMeshDescription(lodIndex);
			*meshDescription = localMeshDescription;
			staticMesh->CommitMeshDescription(lodIndex);

			// Assign materials
			for (const FName material : materials)
			{
				const int32 meshSlot = materialSlots[material];
				staticMesh->SectionInfoMap.Set(lodIndex, meshSlot, FMeshSectionInfo(meshSlot));
				staticMesh->SetMaterial(meshSlot, Cast<UMaterialInterface>(IHL2Runtime::Get().TryResolveHL2Material(material.ToString())));
			}
		}
	}

	// Identify physics
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

	return staticMesh;
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