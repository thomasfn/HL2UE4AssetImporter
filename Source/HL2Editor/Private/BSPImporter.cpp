#pragma once

#include "BSPImporter.h"
#include "IHL2Runtime.h"
#include "Paths.h"
#include "EditorActorFolders.h"
#include "Engine/World.h"
#include "Engine/Brush.h"
#include "Builders/CubeBuilder.h"
#include "Engine/Polys.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Selection.h"
#include "Lightmass/LightmassImportanceVolume.h"
#include "Builders/EditorBrushBuilder.h"
#include "Components/BrushComponent.h"
#include "VBSPBrushBuilder.h"
#include "MeshAttributes.h"
#include "Internationalization/Regex.h"
#include "MeshAttributes.h"
#include "MeshUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "AssetRegistryModule.h"
#include "Internationalization/Regex.h"
#include "Misc/ScopedSlowTask.h"

DEFINE_LOG_CATEGORY(LogHL2BSPImporter);

#define LOCTEXT_NAMESPACE ""

FBSPImporter::FBSPImporter()
{

}

bool FBSPImporter::ImportToCurrentLevel(const FString& fileName)
{
	FString bspDir = FPaths::GetPath(fileName) + "/";
	FString bspFileName = FPaths::GetCleanFilename(fileName);
	UE_LOG(LogHL2BSPImporter, Log, TEXT("Importing map '%s' to current level"), *bspFileName);
	const auto bspDirConvert = StringCast<ANSICHAR, TCHAR>(*bspDir);
	const auto bspFileNameConvert = StringCast<ANSICHAR, TCHAR>(*bspFileName);
	Valve::BSPFile bspFile;
	if (!bspFile.parse(std::string(bspDirConvert.Get()), std::string(bspFileNameConvert.Get())))
	{
		UE_LOG(LogHL2BSPImporter, Error, TEXT("Failed to parse BSP"));
		return false;
	}
	return ImportToWorld(bspFile, GEditor->GetEditorWorldContext().World());
}

bool FBSPImporter::ImportToWorld(const Valve::BSPFile& bspFile, UWorld* world)
{
	FScopedSlowTask loopProgress(2, LOCTEXT("MapImporting", "Importing map..."));
	loopProgress.MakeDialog();

	loopProgress.EnterProgressFrame(1.0f);
	if (!ImportGeometryToWorld(bspFile, world)) { return false; }

	loopProgress.EnterProgressFrame(1.0f);
	if (!ImportEntitiesToWorld(bspFile, world)) { return false; }

	//loopProgress.EnterProgressFrame(1.0f);
	//if (!ImportBrushesToWorld(bspFile, world)) { return false; }

	return true;
}

bool FBSPImporter::ImportBrushesToWorld(const Valve::BSPFile& bspFile, UWorld* world)
{
	const FName brushesFolder = TEXT("HL2Brushes");
	UE_LOG(LogHL2BSPImporter, Log, TEXT("Importing brushes..."));
	FActorFolders& folders = FActorFolders::Get();
	folders.CreateFolder(*world, brushesFolder);
	for (uint32 i = 0, l = bspFile.m_Models.size(); i < l; ++i)
	{
		const Valve::BSP::dmodel_t& model = bspFile.m_Models[i];
		TArray<uint16> brushIndices;
		GatherBrushes(bspFile, model.m_Headnode, brushIndices);
		brushIndices.Sort();

		int num = 0;
		for (uint16 brushIndex : brushIndices)
		{
			if (i == 0)
			{
				AActor* brushActor = ImportBrush(world, bspFile, brushIndex);
				GEditor->SelectActor(brushActor, true, false, true, false);
				folders.SetSelectedFolderPath(brushesFolder);
				++num;
				//if (num >= 100) break;
			}
			else
			{
				// TODO: Deal with brush entities
			}
		}
	}

	//GEditor->csgRebuild(world);
	GEditor->RebuildAlteredBSP();

	return true;
}

bool FBSPImporter::ImportGeometryToWorld(const Valve::BSPFile& bspFile, UWorld* world)
{
	const FName geometryFolder = TEXT("HL2Geometry");
	UE_LOG(LogHL2BSPImporter, Log, TEXT("Importing geometry..."));
	FActorFolders& folders = FActorFolders::Get();
	folders.CreateFolder(*world, geometryFolder);

	const Valve::BSP::dmodel_t& bspWorldModel = bspFile.m_Models[0];

	TArray<AStaticMeshActor*> staticMeshes;
	RenderTreeToActors(bspFile, world, staticMeshes, bspWorldModel.m_Headnode);
	GEditor->SelectNone(false, true, false);
	for (AStaticMeshActor* actor : staticMeshes)
	{
		GEditor->SelectActor(actor, true, false, true, false);
	}
	folders.SetSelectedFolderPath(geometryFolder);
	GEditor->SelectNone(false, true, false);

	FVector mins(bspWorldModel.m_Mins(0, 0), bspWorldModel.m_Mins(0, 1), bspWorldModel.m_Mins(0, 2));
	FVector maxs(bspWorldModel.m_Maxs(0, 0), bspWorldModel.m_Maxs(0, 1), bspWorldModel.m_Maxs(0, 2));
	ALightmassImportanceVolume* lightmassImportanceVolume = world->SpawnActor<ALightmassImportanceVolume>();
	lightmassImportanceVolume->Brush = NewObject<UModel>(lightmassImportanceVolume, NAME_None, RF_Transactional);
	lightmassImportanceVolume->Brush->Initialize(nullptr, true);
	lightmassImportanceVolume->Brush->Polys = NewObject<UPolys>(lightmassImportanceVolume->Brush, NAME_None, RF_Transactional);
	lightmassImportanceVolume->GetBrushComponent()->Brush = lightmassImportanceVolume->Brush;
	lightmassImportanceVolume->SetActorLocation(FMath::Lerp(mins, maxs, 0.5f) * FVector(1.0f, -1.0f, 1.0f));
	UCubeBuilder* brushBuilder = NewObject<UCubeBuilder>(lightmassImportanceVolume);
	brushBuilder->X = maxs.X - mins.X;
	brushBuilder->Y = maxs.Y - mins.Y;
	brushBuilder->Z = maxs.Z - mins.Z;
	brushBuilder->Build(world, lightmassImportanceVolume);

	return true;
}

bool FBSPImporter::ImportEntitiesToWorld(const Valve::BSPFile& bspFile, UWorld* world)
{
	const FName entitiesFolder = TEXT("HL2Entities");
	UE_LOG(LogHL2BSPImporter, Log, TEXT("Importing entities..."));
	FActorFolders& folders = FActorFolders::Get();
	folders.CreateFolder(*world, entitiesFolder);

	// Read entities lump
	const auto entityStrRaw = StringCast<TCHAR, ANSICHAR>(&bspFile.m_Entities[0], bspFile.m_Entities.size());
	FString entityStr(entityStrRaw.Get());

	// Parse into entity data
	TArray<FHL2EntityData> entityDatas;
	if (!FEntityParser::ParseEntities(entityStr, entityDatas)) { return false; }

	// Parse static props
	const static FName fnStaticProp(TEXT("prop_static"));
	const static FName fnSolidity(TEXT("solidity"));
	const static FName fnModel(TEXT("model"));
	const static FName fnSkin(TEXT("skin"));
	const static FName fnAngles(TEXT("angles"));
	for (const Valve::BSP::StaticProp_v4_t staticProp : bspFile.m_Staticprops_v4)
	{
		FHL2EntityData entityData;
		entityData.Classname = fnStaticProp;
		entityData.Origin = FVector(staticProp.m_Origin(0, 0), -staticProp.m_Origin(0, 1), staticProp.m_Origin(0, 2));
		entityData.KeyValues[fnSolidity] = FString::FromInt((int)staticProp.m_Solid);
		const char* modelStr = bspFile.m_StaticpropStringTable[staticProp.m_PropType].m_Str;
		const auto& modelRaw = StringCast<TCHAR, ANSICHAR>(modelStr, 128);
		entityData.KeyValues[fnModel] = FString(modelRaw.Length(), modelRaw.Get());
		entityData.KeyValues[fnAngles] = FString::Printf(TEXT("%f %f %f"), staticProp.m_Angles(0, 0), staticProp.m_Angles(0, 1), staticProp.m_Angles(0, 2));
		entityData.KeyValues[fnSkin] = FString::FromInt(staticProp.m_Skin);
		entityDatas.Add(entityData);
	}
	for (const Valve::BSP::StaticProp_v5_t staticProp : bspFile.m_Staticprops_v5)
	{
		FHL2EntityData entityData;
		entityData.Classname = fnStaticProp;
		entityData.Origin = FVector(staticProp.m_Origin(0, 0), -staticProp.m_Origin(0, 1), staticProp.m_Origin(0, 2));
		entityData.KeyValues[fnSolidity] = FString::FromInt((int)staticProp.m_Solid);
		const char* modelStr = bspFile.m_StaticpropStringTable[staticProp.m_PropType].m_Str;
		const auto& modelRaw = StringCast<TCHAR, ANSICHAR>(modelStr, 128);
		entityData.KeyValues[fnModel] = FString(modelRaw.Length(), modelRaw.Get());
		entityData.KeyValues[fnAngles] = FString::Printf(TEXT("%f %f %f"), staticProp.m_Angles(0, 0), staticProp.m_Angles(0, 1), staticProp.m_Angles(0, 2));
		entityData.KeyValues[fnSkin] = FString::FromInt(staticProp.m_Skin);
		entityDatas.Add(entityData);
	}
	for (const Valve::BSP::StaticProp_v6_t staticProp : bspFile.m_Staticprops_v6)
	{
		FHL2EntityData entityData;
		entityData.Classname = fnStaticProp;
		entityData.Origin = FVector(staticProp.m_Origin(0, 0), -staticProp.m_Origin(0, 1), staticProp.m_Origin(0, 2));
		entityData.KeyValues.Add(fnSolidity, FString::FromInt((int)staticProp.m_Solid));
		const char* modelStr = bspFile.m_StaticpropStringTable[staticProp.m_PropType].m_Str;
		const auto& modelRaw = StringCast<TCHAR, ANSICHAR>(modelStr, 128);
		entityData.KeyValues.Add(fnModel, FString(modelRaw.Length(), modelRaw.Get()));
		entityData.KeyValues.Add(fnAngles, FString::Printf(TEXT("%f %f %f"), staticProp.m_Angles(0, 0), staticProp.m_Angles(0, 1), staticProp.m_Angles(0, 2)));
		entityData.KeyValues.Add(fnSkin, FString::FromInt(staticProp.m_Skin));
		entityDatas.Add(entityData);
	}

	// Parse cubemaps
	const static FName fnCubemap(TEXT("env_cubemap"));
	const static FName fnSize(TEXT("size"));
	for (const Valve::BSP::dcubemapsample_t cubemap : bspFile.m_Cubemaps)
	{
		FHL2EntityData entityData;
		entityData.Classname = fnCubemap;
		entityData.Origin = FVector(cubemap.m_Origin[0], -cubemap.m_Origin[1], cubemap.m_Origin[2]);
		entityData.KeyValues.Add(fnSize, FString::FromInt(cubemap.m_Size));
		entityDatas.Add(entityData);
	}

	// Convert into actors
	FScopedSlowTask progress(entityDatas.Num(), LOCTEXT("MapEntitiesImporting", "Importing map entities..."));
	GEditor->SelectNone(false, true, false);
	for (const FHL2EntityData& entityData : entityDatas)
	{
		progress.EnterProgressFrame();
		AActor* actor = ImportEntityToWorld(bspFile, world, entityData);
		if (actor != nullptr)
		{
			GEditor->SelectActor(actor, true, false, true, false);
		}
	}
	folders.SetSelectedFolderPath(entitiesFolder);
	GEditor->SelectNone(false, true, false);

	return true;
}

void FBSPImporter::RenderTreeToActors(const Valve::BSPFile& bspFile, UWorld* world, TArray<AStaticMeshActor*>& out, uint32 nodeIndex)
{
	const static float cellSize = 1024.0f;

	// Determine cell mins and maxs
	const Valve::BSP::snode_t& bspNode = bspFile.m_Nodes[nodeIndex];
	const int cellMinX = FMath::FloorToInt(bspNode.m_Mins[0] / cellSize);
	const int cellMaxX = FMath::CeilToInt(bspNode.m_Maxs[0] / cellSize);
	const int cellMinY = FMath::FloorToInt(bspNode.m_Mins[1] / cellSize);
	const int cellMaxY = FMath::CeilToInt(bspNode.m_Maxs[1] / cellSize);

	FScopedSlowTask progress((cellMaxX - cellMinX + 1) * (cellMaxY - cellMinY + 1) + 12, LOCTEXT("MapGeometryImporting", "Importing map geometry..."));
	progress.MakeDialog();

	// Gather all faces and displacements from tree
	progress.EnterProgressFrame(1.0f, LOCTEXT("MapGeometryImporting_GATHER", "Gathering faces and displacements..."));
	TArray<uint16> faces;
	GatherFaces(bspFile, nodeIndex, faces);
	TArray<uint16> displacements;
	GatherDisplacements(bspFile, faces, displacements);

	{
		// Render whole tree to a single mesh
		progress.EnterProgressFrame(10.0f, LOCTEXT("MapGeometryImporting_GENERATE", "Generating map geometry..."));
		FMeshDescription meshDesc;
		UStaticMesh::RegisterMeshAttributes(meshDesc);
		RenderFacesToMesh(bspFile, faces, meshDesc, false);
		RenderDisplacementsToMesh(bspFile, displacements, meshDesc);
		meshDesc.ComputeTangentsAndNormals(EComputeNTBsOptions::Normals & EComputeNTBsOptions::Tangents);

		// Iterate each cell
		for (int cellX = cellMinX; cellX <= cellMaxX; ++cellX)
		{
			for (int cellY = cellMinY; cellY <= cellMaxY; ++cellY)
			{
				progress.EnterProgressFrame(1.0f, LOCTEXT("MapGeometryImporting_CELL", "Splitting cells..."));

				// Establish bounding planes for cell
				TArray<FPlane> boundingPlanes;
				boundingPlanes.Add(FPlane(FVector(cellX * cellSize), FVector::ForwardVector));
				boundingPlanes.Add(FPlane(FVector((cellX + 1) * cellSize), FVector::BackwardVector));
				boundingPlanes.Add(FPlane(FVector(cellY * cellSize), FVector::RightVector));
				boundingPlanes.Add(FPlane(FVector((cellY + 1) * cellSize), FVector::LeftVector));

				// Clip the mesh by the planes into a new one
				FMeshDescription cellMeshDesc = meshDesc;
				FMeshUtils::Clip(cellMeshDesc, boundingPlanes);

				// Check if it has anything
				if (cellMeshDesc.Polygons().Num() > 0)
				{
					// Create a static mesh for it
					AStaticMeshActor* staticMeshActor = RenderMeshToActor(world, cellMeshDesc);
					staticMeshActor->SetActorLabel(FString::Printf(TEXT("Cell_%d_%d"), cellX, cellY));
					out.Add(staticMeshActor);
				}
			}
		}
	}

	{
		progress.EnterProgressFrame(1.0f, LOCTEXT("MapGeometryImporting_SKYBOX", "Generating skybox geometry..."));

		// Render skybox to a single mesh
		FMeshDescription meshDesc;
		UStaticMesh::RegisterMeshAttributes(meshDesc);
		RenderFacesToMesh(bspFile, faces, meshDesc, true);
		meshDesc.ComputeTangentsAndNormals(EComputeNTBsOptions::Normals & EComputeNTBsOptions::Tangents);
		meshDesc.TriangulateMesh();

		// Create actor for it
		AStaticMeshActor* staticMeshActor = RenderMeshToActor(world, meshDesc);
		staticMeshActor->SetActorLabel(TEXT("Skybox"));
		staticMeshActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
		staticMeshActor->GetStaticMeshComponent()->CastShadow = false;
		out.Add(staticMeshActor);
	}
}

UStaticMesh* FBSPImporter::RenderMeshToStaticMesh(UWorld* world, const FMeshDescription& meshDesc)
{
	UStaticMesh* staticMesh = NewObject<UStaticMesh>(world);
	FStaticMeshSourceModel& staticMeshSourceModel = staticMesh->AddSourceModel();
	FMeshBuildSettings& settings = staticMeshSourceModel.BuildSettings;
	settings.bRecomputeNormals = false;
	settings.bRecomputeTangents = false;
	settings.bGenerateLightmapUVs = true;
	settings.SrcLightmapIndex = 0;
	settings.DstLightmapIndex = 1;
	settings.bRemoveDegenerates = false;
	settings.bUseFullPrecisionUVs = true;
	settings.MinLightmapResolution = 128;
	staticMesh->LightMapResolution = 128;
	FMeshDescription* worldModelMesh = staticMesh->CreateMeshDescription(0);
	*worldModelMesh = meshDesc;
	const auto& importedMaterialSlotNameAttr = worldModelMesh->PolygonGroupAttributes().GetAttributesRef<FName>(MeshAttribute::PolygonGroup::ImportedMaterialSlotName);
	for (const FPolygonGroupID& polyGroupID : worldModelMesh->PolygonGroups().GetElementIDs())
	{
		FName material = importedMaterialSlotNameAttr[polyGroupID];
		const int32 meshSlot = staticMesh->StaticMaterials.Emplace(nullptr, material, material);
		staticMesh->SectionInfoMap.Set(0, meshSlot, FMeshSectionInfo(meshSlot));
		staticMesh->SetMaterial(meshSlot, Cast<UMaterialInterface>(IHL2Runtime::Get().TryResolveHL2Material(material.ToString())));
	}
	staticMesh->CommitMeshDescription(0);
	staticMesh->LightMapCoordinateIndex = 1;
	staticMesh->Build();
	staticMesh->CreateBodySetup();
	staticMesh->BodySetup->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseComplexAsSimple;
	return staticMesh;
}

AStaticMeshActor* FBSPImporter::RenderMeshToActor(UWorld* world, const FMeshDescription& meshDesc)
{
	UStaticMesh* staticMesh = RenderMeshToStaticMesh(world, meshDesc);

	FTransform transform = FTransform::Identity;
	transform.SetScale3D(FVector(1.0f, -1.0f, 1.0f));

	AStaticMeshActor* staticMeshActor = world->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), transform);
	UStaticMeshComponent* staticMeshComponent = staticMeshActor->GetStaticMeshComponent();
	staticMeshComponent->SetStaticMesh(staticMesh);
	FLightmassPrimitiveSettings& lightmassSettings = staticMeshComponent->LightmassSettings;
	lightmassSettings.bUseEmissiveForStaticLighting = true;
	staticMeshComponent->bCastShadowAsTwoSided = true;

	staticMeshActor->PostEditChange();
	return staticMeshActor;
}

AActor* FBSPImporter::ImportBrush(UWorld* world, const Valve::BSPFile& bspFile, uint16 brushIndex)
{
	const Valve::BSP::dbrush_t& brush = bspFile.m_Brushes[brushIndex];

	// Create and initialise brush actor
	ABrush* brushActor = world->SpawnActor<ABrush>(FVector::ZeroVector, FRotator::ZeroRotator);
	brushActor->SetActorLabel(FString::Printf(TEXT("Brush_%d"), brushIndex));
	brushActor->Brush = NewObject<UModel>(brushActor, NAME_None, RF_Transactional);
	brushActor->Brush->Initialize(nullptr, true);
	brushActor->Brush->Polys = NewObject<UPolys>(brushActor->Brush, NAME_None, RF_Transactional);
	brushActor->GetBrushComponent()->Brush = brushActor->Brush;

	// Create and initialise brush builder
	UVBSPBrushBuilder* brushBuilder = NewObject<UVBSPBrushBuilder>(brushActor);
	brushActor->BrushBuilder = brushBuilder;
	
	// Add all planes to the brush builder
	for (uint32 i = 0, l = brush.m_Numsides; i < l; ++i)
	{
		const Valve::BSP::dbrushside_t& brushSide = bspFile.m_Brushsides[brush.m_Firstside + i];
		const Valve::BSP::cplane_t& plane = bspFile.m_Planes[brushSide.m_Planenum];

		brushBuilder->Planes.Add(ValveToUnrealPlane(plane));
	}

	// Evaluate the geometric center of the brush and transform all planes to it
	FVector origin = brushBuilder->EvaluateGeometricCenter();
	FTransform transform = FTransform::Identity;
	transform.SetLocation(-origin);
	FMatrix transformMtx = transform.ToMatrixNoScale();
	for (uint32 i = 0, l = brush.m_Numsides; i < l; ++i)
	{
		brushBuilder->Planes[i] = brushBuilder->Planes[i].TransformBy(transformMtx);
	}

	// Relocate the brush
	brushActor->SetActorLocation(origin);

	// Build brush geometry
	brushBuilder->Build(world, brushActor);

	return brushActor;
}

void FBSPImporter::GatherBrushes(const Valve::BSPFile& bspFile, uint32 nodeIndex, TArray<uint16>& out)
{
	const Valve::BSP::snode_t& node = bspFile.m_Nodes[nodeIndex];
	if (node.m_Children[0] < 0)
	{
		const Valve::BSP::dleaf_t& leaf = bspFile.m_Leaves[-1-node.m_Children[0]];
		for (uint32 i = 0; i < leaf.m_Numleafbrushes; ++i)
		{
			out.AddUnique(bspFile.m_Leafbrushes[leaf.m_Firstleafbrush + i]);
		}
	}
	else
	{
		GatherBrushes(bspFile, (uint32)node.m_Children[0], out);
	}
	if (node.m_Children[1] < 0)
	{
		const Valve::BSP::dleaf_t& leaf = bspFile.m_Leaves[-1 - node.m_Children[1]];
		for (uint32 i = 0; i < leaf.m_Numleafbrushes; ++i)
		{
			out.AddUnique(bspFile.m_Leafbrushes[leaf.m_Firstleafbrush + i]);
		}
	}
	else
	{
		GatherBrushes(bspFile, (uint32)node.m_Children[1], out);
	}
}

void FBSPImporter::GatherFaces(const Valve::BSPFile& bspFile, uint32 nodeIndex, TArray<uint16>& out, TSet<int16>* clusterFilter)
{
	const Valve::BSP::snode_t& node = bspFile.m_Nodes[nodeIndex];
	if (node.m_Children[0] < 0)
	{
		const Valve::BSP::dleaf_t& leaf = bspFile.m_Leaves[-1 - node.m_Children[0]];
		if (clusterFilter == nullptr || clusterFilter->Contains(leaf.m_Cluster))
		{
			for (uint32 i = 0; i < leaf.m_Numleaffaces; ++i)
			{
				out.AddUnique(bspFile.m_Leaffaces[leaf.m_Firstleafface + i]);
			}
		}
	}
	else
	{
		GatherFaces(bspFile, (uint32)node.m_Children[0], out, clusterFilter);
	}
	if (node.m_Children[1] < 0)
	{
		const Valve::BSP::dleaf_t& leaf = bspFile.m_Leaves[-1 - node.m_Children[1]];
		if (clusterFilter == nullptr || clusterFilter->Contains(leaf.m_Cluster))
		{
			for (uint32 i = 0; i < leaf.m_Numleaffaces; ++i)
			{
				out.AddUnique(bspFile.m_Leaffaces[leaf.m_Firstleafface + i]);
			}
		}
	}
	else
	{
		GatherFaces(bspFile, (uint32)node.m_Children[1], out, clusterFilter);
	}
}

void FBSPImporter::GatherDisplacements(const Valve::BSPFile& bspFile, const TArray<uint16>& faceIndices, TArray<uint16>& out)
{
	for (uint16 dispIndex = 0; dispIndex < (uint16)bspFile.m_Dispinfos.size(); ++dispIndex)
	{
		//const Valve::BSP::ddispinfo_t& bspDispInfo = bspFile.m_Dispinfos[dispIndex];
		//if (faceIndices.Contains(bspDispInfo.m_MapFace))
		{
			out.Add(dispIndex);
		}
	}
}

void FBSPImporter::GatherClusters(const Valve::BSPFile& bspFile, uint32 nodeIndex, TArray<int16>& out)
{
	const Valve::BSP::snode_t& node = bspFile.m_Nodes[nodeIndex];
	if (node.m_Children[0] < 0)
	{
		const Valve::BSP::dleaf_t& leaf = bspFile.m_Leaves[-1 - node.m_Children[0]];
		if (leaf.m_Numleaffaces > 0)
		{
			out.AddUnique(leaf.m_Cluster);
		}
	}
	else
	{
		GatherClusters(bspFile, (uint32)node.m_Children[0], out);
	}
	if (node.m_Children[1] < 0)
	{
		const Valve::BSP::dleaf_t& leaf = bspFile.m_Leaves[-1 - node.m_Children[1]];
		if (leaf.m_Numleaffaces > 0)
		{
			out.AddUnique(leaf.m_Cluster);
		}
	}
	else
	{
		GatherClusters(bspFile, (uint32)node.m_Children[1], out);
	}
}

FPlane FBSPImporter::ValveToUnrealPlane(const Valve::BSP::cplane_t& plane)
{
	return FPlane(plane.m_Normal(0, 0), plane.m_Normal(0, 1), plane.m_Normal(0, 2), plane.m_Distance);
}

void FBSPImporter::RenderFacesToMesh(const Valve::BSPFile& bspFile, const TArray<uint16>& faceIndices, FMeshDescription& meshDesc, bool skyboxFilter)
{
	TMap<uint32, FVertexID> valveToUnrealVertexMap;
	TMap<FName, FPolygonGroupID> materialToPolyGroupMap;
	TMap<FPolygonID, uint16> polyToValveFaceMap;

	TAttributesSet<FVertexID>& vertexAttr = meshDesc.VertexAttributes();
	TMeshAttributesRef<FVertexID, FVector> vertexAttrPosition = vertexAttr.GetAttributesRef<FVector>(MeshAttribute::Vertex::Position);

	TAttributesSet<FVertexInstanceID>& vertexInstanceAttr = meshDesc.VertexInstanceAttributes();
	TMeshAttributesRef<FVertexInstanceID, FVector2D> vertexInstanceAttrUV = vertexInstanceAttr.GetAttributesRef<FVector2D>(MeshAttribute::VertexInstance::TextureCoordinate);
	TMeshAttributesRef<FVertexInstanceID, FVector4> vertexInstanceAttrCol = vertexInstanceAttr.GetAttributesRef<FVector4>(MeshAttribute::VertexInstance::Color);
	// vertexInstanceAttrUV.SetNumIndices(2);

	TAttributesSet<FEdgeID>& edgeAttr = meshDesc.EdgeAttributes();
	TMeshAttributesRef<FEdgeID, bool> edgeAttrIsHard = edgeAttr.GetAttributesRef<bool>(MeshAttribute::Edge::IsHard);
	TMeshAttributesRef<FEdgeID, float> edgeCreaseSharpness = edgeAttr.GetAttributesRef<float>(MeshAttribute::Edge::CreaseSharpness);

	TAttributesSet<FPolygonGroupID>& polyGroupAttr = meshDesc.PolygonGroupAttributes();
	TMeshAttributesRef<FPolygonGroupID, FName> polyGroupMaterial = polyGroupAttr.GetAttributesRef<FName>(MeshAttribute::PolygonGroup::ImportedMaterialSlotName);

	int cnt = 0;

	// Pass 1: Create geometry
	for (const uint16 faceIndex : faceIndices)
	{
		const Valve::BSP::dface_t& bspFace = bspFile.m_Surfaces[faceIndex];
		const Valve::BSP::texinfo_t& bspTexInfo = bspFile.m_Texinfos[bspFace.m_Texinfo];
		const uint16 texDataIndex = (uint16)bspTexInfo.m_Texdata;
		const Valve::BSP::texdata_t& bspTexData = bspFile.m_Texdatas[bspTexInfo.m_Texdata];
		const char* bspMaterialName = &bspFile.m_TexdataStringData[0] + bspFile.m_TexdataStringTable[bspTexData.m_NameStringTableID];
		FString parsedMaterialName = ParseMaterialName(bspMaterialName);

		const bool isSkybox = parsedMaterialName.Equals(TEXT("tools/toolsskybox"), ESearchCase::IgnoreCase) || parsedMaterialName.Equals(TEXT("tools/toolsskybox2d"), ESearchCase::IgnoreCase);
		if (isSkybox != skyboxFilter) { continue; }
		FName material(*parsedMaterialName);

		// Create polygroup if needed (we make one per material/texdata)
		FPolygonGroupID polyGroup;
		if (!materialToPolyGroupMap.Contains(material))
		{
			polyGroup = meshDesc.CreatePolygonGroup();
			materialToPolyGroupMap.Add(material, polyGroup);
			polyGroupMaterial[polyGroup] = material;
		}
		else
		{
			polyGroup = materialToPolyGroupMap[material];
		}

		TArray<FVertexInstanceID> polyVerts;
		TSet<uint16> bspVerts; // track all vbsp verts we've visited as it likes to revisit them sometimes and cause degenerate polys

		// Iterate all edges
		for (uint16 i = 0; i < bspFace.m_Numedges; ++i)
		{
			const int32 surfEdge = bspFile.m_Surfedges[bspFace.m_Firstedge + i];
			const uint32 edgeIndex = (uint32)(surfEdge < 0 ? -surfEdge : surfEdge);

			const Valve::BSP::dedge_t& bspEdge = bspFile.m_Edges[edgeIndex];
			const uint16 vertIndex = surfEdge < 0 ? bspEdge.m_V[1] : bspEdge.m_V[0];

			if (bspVerts.Contains(vertIndex)) { break; }
			bspVerts.Add(vertIndex);

			// Create vertices if needed
			FVertexID vertID;
			if (!valveToUnrealVertexMap.Contains(vertIndex))
			{
				vertID = meshDesc.CreateVertex();
				valveToUnrealVertexMap.Add(vertIndex, vertID);
				const Valve::BSP::mvertex_t& bspVertex = bspFile.m_Vertexes[vertIndex];
				vertexAttrPosition[vertID] = FVector(bspVertex.m_Position(0, 0), bspVertex.m_Position(0, 1), bspVertex.m_Position(0, 2));
			}
			else
			{
				vertID = valveToUnrealVertexMap[vertIndex];
			}

			// Create vertex instance
			FVertexInstanceID vertInstID = meshDesc.CreateVertexInstance(vertID);

			// Calculate texture coords
			FVector pos = vertexAttrPosition[vertID];
			{
				const FVector texU_XYZ = FVector(bspTexInfo.m_TextureVecs[0][0], bspTexInfo.m_TextureVecs[0][1], bspTexInfo.m_TextureVecs[0][2]);
				const float texU_W = bspTexInfo.m_TextureVecs[0][3];
				const FVector texV_XYZ = FVector(bspTexInfo.m_TextureVecs[1][0], bspTexInfo.m_TextureVecs[1][1], bspTexInfo.m_TextureVecs[1][2]);
				const float texV_W = bspTexInfo.m_TextureVecs[1][3];
				vertexInstanceAttrUV.Set(vertInstID, 0, FVector2D(
					(FVector::DotProduct(texU_XYZ, pos) + texU_W) / bspTexData.m_Width,
					(FVector::DotProduct(texV_XYZ, pos) + texV_W) / bspTexData.m_Height
				));
			}
			//{
			//	const FVector texU_XYZ = FVector(bspTexInfo.m_LightmapVecs[0][0], bspTexInfo.m_LightmapVecs[0][1], bspTexInfo.m_LightmapVecs[0][2]);
			//	const float texU_W = bspTexInfo.m_LightmapVecs[0][3];
			//	const FVector texV_XYZ = FVector(bspTexInfo.m_LightmapVecs[1][0], bspTexInfo.m_LightmapVecs[1][1], bspTexInfo.m_LightmapVecs[1][2]);
			//	const float texV_W = bspTexInfo.m_LightmapVecs[1][3];
			//	vertexInstanceAttrUV.Set(vertInst, 1, FVector2D(
			//		((FVector::DotProduct(texU_XYZ, pos) + texU_W)/* - bspFace.m_LightmapTextureMinsInLuxels[0]*/) / (bspFace.m_LightmapTextureSizeInLuxels[0] + 1),
			//		((FVector::DotProduct(texV_XYZ, pos) + texV_W)/* - bspFace.m_LightmapTextureMinsInLuxels[1]*/) / (bspFace.m_LightmapTextureSizeInLuxels[1] + 1)
			//	));
			//}

			// Default color
			vertexInstanceAttrCol[vertInstID] = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

			// Push
			polyVerts.Add(vertInstID);
		}

		// Create poly
		if (polyVerts.Num() > 2)
		{
			FPolygonID poly = meshDesc.CreatePolygon(polyGroup, polyVerts);
			FMeshPolygon& polygon = meshDesc.GetPolygon(poly);
			polyToValveFaceMap.Add(poly, faceIndex);
		}
	}

	// Pass 2: Set smoothing groups
	for (const auto& pair : polyToValveFaceMap)
	{
		const Valve::BSP::dface_t& bspFace = bspFile.m_Surfaces[pair.Value];

		// Find all edges
		TArray<FEdgeID> edges;
		meshDesc.GetPolygonEdges(pair.Key, edges);
		for (const FEdgeID& edge : edges)
		{
			// Find polys connected to the edge
			const TArray<FPolygonID>& otherPolys = meshDesc.GetEdgeConnectedPolygons(edge);
			for (const FPolygonID& otherPoly : otherPolys)
			{
				if (otherPoly != pair.Key)
				{
					const Valve::BSP::dface_t& bspOtherFace = bspFile.m_Surfaces[polyToValveFaceMap[otherPoly]];
					const bool isHard = !SharesSmoothingGroup(bspFace.m_SmoothingGroups, bspOtherFace.m_SmoothingGroups);
					edgeAttrIsHard[edge] = isHard;
					edgeCreaseSharpness[edge] = isHard ? 1.0f : 0.0f;
				}
			}
		}
	}
}

void FBSPImporter::RenderDisplacementsToMesh(const Valve::BSPFile& bspFile, const TArray<uint16>& displacements, FMeshDescription& meshDesc)
{
	TAttributesSet<FVertexID>& vertexAttr = meshDesc.VertexAttributes();
	TMeshAttributesRef<FVertexID, FVector> vertexAttrPosition = vertexAttr.GetAttributesRef<FVector>(MeshAttribute::Vertex::Position);

	TAttributesSet<FVertexInstanceID>& vertexInstanceAttr = meshDesc.VertexInstanceAttributes();
	TMeshAttributesRef<FVertexInstanceID, FVector2D> vertexInstanceAttrUV = vertexInstanceAttr.GetAttributesRef<FVector2D>(MeshAttribute::VertexInstance::TextureCoordinate);
	TMeshAttributesRef<FVertexInstanceID, FVector4> vertexInstanceAttrCol = vertexInstanceAttr.GetAttributesRef<FVector4>(MeshAttribute::VertexInstance::Color);

	TAttributesSet<FEdgeID>& edgeAttr = meshDesc.EdgeAttributes();
	TMeshAttributesRef<FEdgeID, bool> edgeAttrIsHard = edgeAttr.GetAttributesRef<bool>(MeshAttribute::Edge::IsHard);
	TMeshAttributesRef<FEdgeID, float> edgeCreaseSharpness = edgeAttr.GetAttributesRef<float>(MeshAttribute::Edge::CreaseSharpness);

	TAttributesSet<FPolygonGroupID>& polyGroupAttr = meshDesc.PolygonGroupAttributes();
	TMeshAttributesRef<FPolygonGroupID, FName> polyGroupMaterial = polyGroupAttr.GetAttributesRef<FName>(MeshAttribute::PolygonGroup::ImportedMaterialSlotName);

	for (const uint16 dispIndex : displacements)
	{
		const Valve::BSP::ddispinfo_t& bspDispinfo = bspFile.m_Dispinfos[dispIndex];
		const Valve::BSP::dface_t& bspFace = bspFile.m_Surfaces[bspDispinfo.m_MapFace];
		const int dispRes = 1 << bspDispinfo.m_Power;
		const FVector dispStartPos = FVector(bspDispinfo.m_StartPosition(0, 0), bspDispinfo.m_StartPosition(0, 1), bspDispinfo.m_StartPosition(0, 2));

		const Valve::BSP::texinfo_t& bspTexInfo = bspFile.m_Texinfos[bspFace.m_Texinfo];
		const uint16 texDataIndex = (uint16)bspTexInfo.m_Texdata;
		const Valve::BSP::texdata_t& bspTexData = bspFile.m_Texdatas[bspTexInfo.m_Texdata];
		const char* bspMaterialName = &bspFile.m_TexdataStringData[0] + bspFile.m_TexdataStringTable[bspTexData.m_NameStringTableID];
		FString parsedMaterialName = ParseMaterialName(bspMaterialName);
		FName material(*parsedMaterialName);

		// Create a unique poly group for us
		const FPolygonGroupID polyGroupID = meshDesc.CreatePolygonGroup();
		polyGroupMaterial[polyGroupID] = material;

		// Gather face verts
		TArray<FVector> faceVerts;
		for (uint16 i = 0; i < bspFace.m_Numedges; ++i)
		{
			const int32 surfEdge = bspFile.m_Surfedges[bspFace.m_Firstedge + i];
			const uint32 edgeIndex = (uint32)(surfEdge < 0 ? -surfEdge : surfEdge);

			const Valve::BSP::dedge_t& bspEdge = bspFile.m_Edges[edgeIndex];
			const uint16 vertIndex = surfEdge < 0 ? bspEdge.m_V[1] : bspEdge.m_V[0];

			const Valve::BSP::mvertex_t& bspVertex = bspFile.m_Vertexes[vertIndex];

			faceVerts.Add(FVector(bspVertex.m_Position(0, 0), bspVertex.m_Position(0, 1), bspVertex.m_Position(0, 2)));
		}

		// Check that it has 4 verts (we can't map a displacement to anything other than a quad)
		if (faceVerts.Num() != 4)
		{
			UE_LOG(LogHL2BSPImporter, Warning, TEXT("Displacement by index $d is mapped to a non-quad face $d (with $d verts)"), dispIndex, bspDispinfo.m_MapFace, faceVerts.Num());
			continue;
		}

		// Find the face vert nearest the given starting position
		int startFaceVertIndex = 0;
		{
			FVector bestFaceVert = faceVerts[startFaceVertIndex];
			float bestFaceVertDist = FVector::DistSquared(bestFaceVert, dispStartPos);
			for (int i = startFaceVertIndex + 1; i < faceVerts.Num(); ++i)
			{
				const FVector faceVert = faceVerts[i];
				const float faceVertDist = FVector::DistSquared(faceVert, dispStartPos);
				if (faceVertDist < bestFaceVertDist)
				{
					bestFaceVert = faceVert;
					bestFaceVertDist = faceVertDist;
					startFaceVertIndex = i;
				}
			}
		}

		// Determine corners vectors
		const FVector p0 = faceVerts[startFaceVertIndex];
		const FVector p1 = faceVerts[(startFaceVertIndex + 1) % faceVerts.Num()];
		const FVector p2 = faceVerts[(startFaceVertIndex + 2) % faceVerts.Num()];
		const FVector p3 = faceVerts[(startFaceVertIndex + 3) % faceVerts.Num()];

		// Create all verts
		TArray<FVertexInstanceID> dispVertices;
		dispVertices.AddDefaulted((dispRes + 1) * (dispRes + 1));
		for (int x = 0; x <= dispRes; ++x)
		{
			const float dX = x / (float)dispRes;
			const FVector mp0 = FMath::Lerp(p0, p1, dX);
			const FVector mp1 = FMath::Lerp(p3, p2, dX);
			for (int y = 0; y <= dispRes; ++y)
			{
				const float dY = y / (float)dispRes;

				const int idx = x * (dispRes + 1) + y;
				const Valve::BSP::ddispvert_t& bspVert = bspFile.m_Dispverts[bspDispinfo.m_DispVertStart + idx];
				const FVector bspVertVec = FVector(bspVert.m_Vec(0, 0), bspVert.m_Vec(0, 1), bspVert.m_Vec(0, 2));

				const FVector basePos = FMath::Lerp(mp0, mp1, dY);
				const FVector dispPos = basePos + bspVertVec * bspVert.m_Dist;

				const FVertexID meshVertID = meshDesc.CreateVertex();
				vertexAttrPosition[meshVertID] = dispPos;

				const FVertexInstanceID meshVertInstID = meshDesc.CreateVertexInstance(meshVertID);

				const FVector texU_XYZ = FVector(bspTexInfo.m_TextureVecs[0][0], bspTexInfo.m_TextureVecs[0][1], bspTexInfo.m_TextureVecs[0][2]);
				const float texU_W = bspTexInfo.m_TextureVecs[0][3];
				const FVector texV_XYZ = FVector(bspTexInfo.m_TextureVecs[1][0], bspTexInfo.m_TextureVecs[1][1], bspTexInfo.m_TextureVecs[1][2]);
				const float texV_W = bspTexInfo.m_TextureVecs[1][3];
				vertexInstanceAttrUV.Set(meshVertInstID, 0, FVector2D(
					(FVector::DotProduct(texU_XYZ, basePos) + texU_W) / bspTexData.m_Width,
					(FVector::DotProduct(texV_XYZ, basePos) + texV_W) / bspTexData.m_Height
				));

				FVector4 col;
				col.X = FMath::Clamp(bspVert.m_Alpha / 255.0f, 0.0f, 1.0f);
				col.Y = 1.0f - col.X;
				col.Z = 0.0f;
				col.W = 1.0f;
				vertexInstanceAttrCol[meshVertInstID] = col;

				dispVertices[idx] = meshVertInstID;
			}
		}

		// Create all polys
		TArray<FVertexInstanceID> polyPoints;
		TArray<FEdgeID> polyEdgeIDs;
		for (int x = 0; x < dispRes; ++x)
		{
			for (int y = 0; y < dispRes; ++y)
			{
				const int idxA = x * (dispRes + 1) + y;
				const int idxB = (x + 1) * (dispRes + 1) + y;
				const int idxC = (x + 1) * (dispRes + 1) + (y + 1);
				const int idxD = x * (dispRes + 1) + (y + 1);

				polyPoints.Empty(4);
				polyPoints.Add(dispVertices[idxA]);
				polyPoints.Add(dispVertices[idxB]);
				polyPoints.Add(dispVertices[idxC]);
				polyPoints.Add(dispVertices[idxD]);

				const FPolygonID polyID = meshDesc.CreatePolygon(polyGroupID, polyPoints);
				polyEdgeIDs.Empty(4);
				meshDesc.GetPolygonEdges(polyID, polyEdgeIDs);
				for (const FEdgeID& edgeID : polyEdgeIDs)
				{
					edgeAttrIsHard[edgeID] = false;
					edgeCreaseSharpness[edgeID] = 0.0f;
				}
			}
		}
	}
}

FString FBSPImporter::ParseMaterialName(const char* bspMaterialName)
{
	// It might be something like "brick/brick06c" which is fine
	// But it might also be like "maps/<mapname>/brick/brick06c_x_y_z" which is not fine
	// Also "nature/red_grass_wvt_patch" -> "nature/red_grass"
	// We need to identify the latter and convert it to the former
	FString bspMaterialNameAsStr(bspMaterialName);
	bspMaterialNameAsStr.ReplaceCharInline('\\', '/');
	{
		const static FRegexPattern patternCubemappedMaterial(TEXT("^maps[\\\\\\/]\\w+[\\\\\\/](.+)(?:_-?(?:\\d*\\.)?\\d+){3}$"));
		FRegexMatcher matchCubemappedMaterial(patternCubemappedMaterial, bspMaterialNameAsStr);
		if (matchCubemappedMaterial.FindNext())
		{
			bspMaterialNameAsStr = matchCubemappedMaterial.GetCaptureGroup(1);
		}
	}
	{
		const static FRegexPattern patternWVTMaterial(TEXT("^maps[\\\\\\/]\\w+[\\\\\\/](.+)_wvt_patch$"));
		FRegexMatcher matchWVTMaterial(patternWVTMaterial, bspMaterialNameAsStr);
		if (matchWVTMaterial.FindNext())
		{
			bspMaterialNameAsStr = matchWVTMaterial.GetCaptureGroup(1);
		}
	}
	return bspMaterialNameAsStr;
}

bool FBSPImporter::SharesSmoothingGroup(uint16 groupA, uint16 groupB)
{
	for (uint16 i = 0; i < 16; ++i)
	{
		const uint16 mask = 1 << i;
		if ((groupA & mask) && (groupB & mask)) { return true; }
	}
	return false;
}

#undef LOCTEXT_NAMESPACE

ABaseEntity* FBSPImporter::ImportEntityToWorld(const Valve::BSPFile& bspFile, UWorld* world, const FHL2EntityData& entityData)
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
	transform.SetLocation(entityData.Origin);

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
			const Valve::BSP::dmodel_t& bspModel = bspFile.m_Models[modelIndex];
			TArray<uint16> faces;
			GatherFaces(bspFile, bspModel.m_Headnode, faces);
			FMeshDescription meshDesc;
			UStaticMesh::RegisterMeshAttributes(meshDesc);
			RenderFacesToMesh(bspFile, faces, meshDesc, false);
			meshDesc.ComputeTangentsAndNormals(EComputeNTBsOptions::Normals & EComputeNTBsOptions::Tangents);
			meshDesc.TriangulateMesh();
			UStaticMesh* staticMesh = RenderMeshToStaticMesh(world, meshDesc);
			entity->WorldModel = staticMesh;
		}
	}

	// Run ctor on the entity
	entity->EntityData = entityData;
	if (!entityData.Targetname.IsEmpty())
	{
		entity->SetActorLabel(entityData.Targetname);
	}
	entity->RerunConstructionScripts();

	return entity;
}