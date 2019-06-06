#pragma once

#include "BSPImporter.h"
#include "Paths.h"
#include "IHL2Editor.h"

#ifdef WITH_EDITOR
#include "EditorActorFolders.h"
#include "Engine/World.h"
#include "Engine/Brush.h"
#include "Engine/Polys.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Selection.h"
#include "Builders/EditorBrushBuilder.h"
#include "Components/BrushComponent.h"
#include "VBSPBrushBuilder.h"
#include "MeshAttributes.h"
#include "Internationalization/Regex.h"
#endif

DEFINE_LOG_CATEGORY(LogHL2BSPImporter);

FBSPImporter::FBSPImporter()
{

}

FBSPImporter::~FBSPImporter()
{

}

#ifdef WITH_EDITOR

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
	//if (!ImportBrushesToWorld(bspFile, world)) { return false; }
	if (!ImportGeometryToWorld(bspFile, world)) { return false; }
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
	//RenderTreeToActors(bspFile, world, staticMeshes, bspWorldModel.m_Headnode);
	staticMeshes.Add(RenderNodeToActor(bspFile, world, bspWorldModel.m_Headnode));
	for (AStaticMeshActor* actor : staticMeshes)
	{
		GEditor->SelectActor(actor, true, false, true, false);
		folders.SetSelectedFolderPath(geometryFolder);
	}

	return true;
}

void FBSPImporter::RenderTreeToActors(const Valve::BSPFile& bspFile, UWorld* world, TArray<AStaticMeshActor*>& out, uint32 nodeIndex, TArray<uint16>& faceIndices)
{
	//TArray<int16> allClusters;
	//GatherClusters(bspFile, nodeIndex, allClusters);
	//TSet<int16> clusterFilter;
	/*for (const int16 clusterIndex : allClusters)
	{
		if (clusterIndex >= 0)
		{
			clusterFilter.Empty(1);
			clusterFilter.Add(clusterIndex);
			out.Add(RenderNodeToActor(bspFile, world, nodeIndex, &clusterFilter));
		}
	}*/

	const Valve::BSP::snode_t& node = bspFile.m_Nodes[nodeIndex];
	if (node.m_Children[0] < 0)
	{
		const Valve::BSP::dleaf_t& leaf = bspFile.m_Leaves[-1 - node.m_Children[0]];
		for (uint32 i = 0; i < leaf.m_Numleaffaces; ++i)
		{
			const uint16 faceIndex = bspFile.m_Leaffaces[leaf.m_Firstleafface + i];
			if (!faceDedup.Contains(faceIndex))
			{
				faceIndices.Add(faceIndex);
				faceDedup.Add(faceIndex);
			}
		}
	}
	else
	{
		RenderTreeToActors(bspFile, world, out, node.m_Children[0], faceIndices);
	}
	if (node.m_Children[1] < 0)
	{
		const Valve::BSP::dleaf_t& leaf = bspFile.m_Leaves[-1 - node.m_Children[1]];
		for (uint32 i = 0; i < leaf.m_Numleaffaces; ++i)
		{
			const uint16 faceIndex = bspFile.m_Leaffaces[leaf.m_Firstleafface + i];
			if (!faceDedup.Contains(faceIndex))
			{
				faceIndices.Add(faceIndex);
				faceDedup.Add(faceIndex);
			}
		}
	}
	else
	{
		RenderTreeToActors(bspFile, world, out, node.m_Children[1], faceIndices);
	}

	if (faceIndices.Num() >= 500)
	{
		out.Add(RenderFacesToActor(bspFile, world, faceIndices));
		faceIndices.Empty();
	}

	//const Valve::BSP::snode_t& bspNode = bspFile.m_Nodes[nodeIndex];

	//// If we've reached maximum depth or either of this node's children is a leaf, stop here and render the rest of the tree to a single mesh
	//if (maxDepth == 0 || bspNode.m_Children[0] < 0 || bspNode.m_Children[1] < 0)
	//{
	//	out.Add(RenderNodeToActor(bspFile, world, nodeIndex));
	//	return;
	//}
	//
	//// Travel down both sides
	//RenderTreeToActors(bspFile, world, out, bspNode.m_Children[0], maxDepth - 1);
	//RenderTreeToActors(bspFile, world, out, bspNode.m_Children[1], maxDepth - 1);
}

void FBSPImporter::RenderTreeToActors(const Valve::BSPFile& bspFile, UWorld* world, TArray<AStaticMeshActor*>& out, uint32 nodeIndex)
{
	faceDedup.Empty();
	TArray<uint16> faceIndices;
	RenderTreeToActors(bspFile, world, out, nodeIndex, faceIndices);
	if (faceIndices.Num() > 0)
	{
		out.Add(RenderFacesToActor(bspFile, world, faceIndices));
	}
}

AStaticMeshActor* FBSPImporter::RenderNodeToActor(const Valve::BSPFile& bspFile, UWorld* world, uint16 nodeIndex, TSet<int16>* clusterFilter)
{
	// Gather all faces to render out
	TArray<uint16> faceIndices;
	GatherFaces(bspFile, nodeIndex, faceIndices, clusterFilter);
	AStaticMeshActor* staticMeshActor = RenderFacesToActor(bspFile, world, faceIndices);
	if (clusterFilter != nullptr)
	{
		TArray<FString> clusterNames;
		for (const int16 clusterIndex : *clusterFilter)
		{
			clusterNames.Add(FString::FromInt(clusterIndex));
		}
		FString clusterGroupName = FString::Join(clusterNames, TEXT(","));
		staticMeshActor->SetActorLabel(FString::Printf(TEXT("VBSPNode_%d_Cluster_%s"), nodeIndex, *clusterGroupName));
	}
	else
	{
		staticMeshActor->SetActorLabel(FString::Printf(TEXT("VBSPNode_%d"), nodeIndex));
	}
	return staticMeshActor;
}

AStaticMeshActor* FBSPImporter::RenderFacesToActor(const Valve::BSPFile& bspFile, UWorld* world, const TArray<uint16>& faceIndices)
{
	UStaticMesh* staticMesh = NewObject<UStaticMesh>(world);
	FStaticMeshSourceModel& staticMeshSourceModel = staticMesh->AddSourceModel();
	FMeshBuildSettings& settings = staticMeshSourceModel.BuildSettings;
	settings.bRecomputeNormals = false;
	settings.bRecomputeTangents = false;
	settings.bGenerateLightmapUVs = true;
	settings.SrcLightmapIndex = 1;
	settings.DstLightmapIndex = 2;
	settings.bRemoveDegenerates = false;
	settings.bUseFullPrecisionUVs = true;
	settings.MinLightmapResolution = 1024;
	staticMesh->LightMapCoordinateIndex = 2;
	staticMesh->LightMapResolution = 1024;
	FMeshDescription* worldModelMesh = staticMesh->CreateMeshDescription(0);
	staticMesh->RegisterMeshAttributes(*worldModelMesh);
	TArray<FName> materials;
	RenderFacesToMesh(bspFile, faceIndices, *worldModelMesh, materials);
	for (const FName material : materials)
	{
		const int32 meshSlot = staticMesh->StaticMaterials.Emplace(nullptr, material, material);
		staticMesh->SectionInfoMap.Set(0, meshSlot, FMeshSectionInfo(meshSlot));
	}
	staticMesh->CommitMeshDescription(0);
	staticMesh->Build();

	FTransform transform = FTransform::Identity;
	transform.SetScale3D(FVector(1.0f, -1.0f, 1.0f));

	AStaticMeshActor* staticMeshActor = world->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), transform);
	UStaticMeshComponent* staticMeshComponent = staticMeshActor->GetStaticMeshComponent();
	staticMeshComponent->SetStaticMesh(staticMesh);
	FLightmassPrimitiveSettings& lightmassSettings = staticMeshComponent->LightmassSettings;
	lightmassSettings.bUseEmissiveForStaticLighting = true;
	staticMeshComponent->bCastShadowAsTwoSided = true;

	for (const FName material : materials)
	{
		staticMeshComponent->SetMaterialByName(material, Cast<UMaterialInterface>(IHL2Editor::Get().TryResolveHL2Material(material.ToString())));
	}

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

void FBSPImporter::RenderNodeToMesh(const Valve::BSPFile& bspFile, uint32 nodeIndex, FMeshDescription& meshDesc, TArray<FName>& materials, TSet<int16>* clusterFilter)
{
	// Gather all faces to render out
	TArray<uint16> faceIndices;
	GatherFaces(bspFile, nodeIndex, faceIndices, clusterFilter);
	RenderFacesToMesh(bspFile, faceIndices, meshDesc, materials);
}

void FBSPImporter::RenderFacesToMesh(const Valve::BSPFile& bspFile, const TArray<uint16>& faceIndices, FMeshDescription& meshDesc, TArray<FName>& materials)
{
	TMap<uint32, FVertexID> valveToUnrealVertexMap;
	TMap<FName, FPolygonGroupID> materialToPolyGroupMap;
	TMap<FPolygonID, uint16> polyToValveFaceMap;

	TAttributesSet<FVertexID>& vertexAttr = meshDesc.VertexAttributes();
	TMeshAttributesRef<FVertexID, FVector> vertexAttrPosition = vertexAttr.GetAttributesRef<FVector>(MeshAttribute::Vertex::Position);

	TAttributesSet<FVertexInstanceID>& vertexInstanceAttr = meshDesc.VertexInstanceAttributes();
	TMeshAttributesRef<FVertexInstanceID, FVector2D> vertexInstanceAttrUV = vertexInstanceAttr.GetAttributesRef<FVector2D>(MeshAttribute::VertexInstance::TextureCoordinate);
	vertexInstanceAttrUV.SetNumIndices(2);

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
		if (parsedMaterialName.Contains(TEXT("tools/"), ESearchCase::IgnoreCase)) { continue; }
		if (parsedMaterialName.Contains(TEXT("tools\\"), ESearchCase::IgnoreCase)) { continue; }
		FName material(*parsedMaterialName);

		// Create polygroup if needed (we make one per material/texdata)
		FPolygonGroupID polyGroup;
		if (!materialToPolyGroupMap.Contains(material))
		{
			polyGroup = meshDesc.CreatePolygonGroup();
			materialToPolyGroupMap.Add(material, polyGroup);
			polyGroupMaterial[polyGroup] = material;
			materials.Add(material);
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
			FVertexID vert;
			if (!valveToUnrealVertexMap.Contains(vertIndex))
			{
				vert = meshDesc.CreateVertex();
				valveToUnrealVertexMap.Add(vertIndex, vert);
				const Valve::BSP::mvertex_t& bspVertex = bspFile.m_Vertexes[vertIndex];
				vertexAttrPosition[vert] = FVector(bspVertex.m_Position(0, 0), bspVertex.m_Position(0, 1), bspVertex.m_Position(0, 2));
			}
			else
			{
				vert = valveToUnrealVertexMap[vertIndex];
			}

			// Create vertex instance
			FVertexInstanceID vertInst = meshDesc.CreateVertexInstance(vert);

			// Calculate texture coords
			FVector pos = vertexAttrPosition[vert];
			{
				const FVector texU_XYZ = FVector(bspTexInfo.m_TextureVecs[0][0], bspTexInfo.m_TextureVecs[0][1], bspTexInfo.m_TextureVecs[0][2]);
				const float texU_W = bspTexInfo.m_TextureVecs[0][3];
				const FVector texV_XYZ = FVector(bspTexInfo.m_TextureVecs[1][0], bspTexInfo.m_TextureVecs[1][1], bspTexInfo.m_TextureVecs[1][2]);
				const float texV_W = bspTexInfo.m_TextureVecs[1][3];
				vertexInstanceAttrUV.Set(vertInst, 0, FVector2D(
					(FVector::DotProduct(texU_XYZ, pos) + texU_W) / bspTexData.m_Width,
					(FVector::DotProduct(texV_XYZ, pos) + texV_W) / bspTexData.m_Height
				));
			}
			{
				const FVector texU_XYZ = FVector(bspTexInfo.m_LightmapVecs[0][0], bspTexInfo.m_LightmapVecs[0][1], bspTexInfo.m_LightmapVecs[0][2]);
				const float texU_W = bspTexInfo.m_LightmapVecs[0][3];
				const FVector texV_XYZ = FVector(bspTexInfo.m_LightmapVecs[1][0], bspTexInfo.m_LightmapVecs[1][1], bspTexInfo.m_LightmapVecs[1][2]);
				const float texV_W = bspTexInfo.m_LightmapVecs[1][3];
				vertexInstanceAttrUV.Set(vertInst, 1, FVector2D(
					((FVector::DotProduct(texU_XYZ, pos) + texU_W)/* - bspFace.m_LightmapTextureMinsInLuxels[0]*/) / (bspFace.m_LightmapTextureSizeInLuxels[0] + 1),
					((FVector::DotProduct(texV_XYZ, pos) + texV_W)/* - bspFace.m_LightmapTextureMinsInLuxels[1]*/) / (bspFace.m_LightmapTextureSizeInLuxels[1] + 1)
				));
			}

			// Push
			polyVerts.Add(vertInst);
		}

		// Create poly
		if (polyVerts.Num() > 2)
		{
			FPolygonID poly = meshDesc.CreatePolygon(polyGroup, polyVerts);
			FMeshPolygon& polygon = meshDesc.GetPolygon(poly);
			meshDesc.ComputePolygonTriangulation(poly, polygon.Triangles);
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

	// Compute tangents and normals
	meshDesc.ComputeTangentsAndNormals(EComputeNTBsOptions::Normals & EComputeNTBsOptions::Tangents);
}

FString FBSPImporter::ParseMaterialName(const char* bspMaterialName)
{
	// It might be something like "brick/brick06c" which is fine
	// But it might also be like "maps/<mapname>/brick/brick06c_x_y_z" which is not fine
	// We need to identify the latter and convert it to the former
	FString bspMaterialNameAsStr(bspMaterialName);
	const static FRegexPattern patternCubemappedMaterial(TEXT("^maps[\\\\\\/]\\w+[\\\\\\/](.+)(?:_-?(?:\\d*\\.)?\\d+){3}$"));
	FRegexMatcher matchCubemappedMaterial(patternCubemappedMaterial, bspMaterialNameAsStr);
	if (matchCubemappedMaterial.FindNext())
	{
		bspMaterialNameAsStr = matchCubemappedMaterial.GetCaptureGroup(1);
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

#endif