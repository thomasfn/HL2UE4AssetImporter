#pragma once

#include "BSPImporter.h"
#include "Paths.h"

#ifdef WITH_EDITOR
#include "EditorActorFolders.h"
#include "Engine/World.h"
#include "Engine/Brush.h"
#include "Engine/Polys.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Builders/EditorBrushBuilder.h"
#include "Components/BrushComponent.h"
#include "VBSPBrushBuilder.h"
#include "MeshAttributes.h"
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

	UStaticMesh* staticMesh = NewObject<UStaticMesh>(world);

	FMeshDescription* worldModelMesh = new FMeshDescription();
	RenderNodeToMesh(bspFile, bspWorldModel.m_Headnode, *worldModelMesh);
	FStaticMeshSourceModel& staticMeshSourceModel = staticMesh->AddSourceModel();
	staticMeshSourceModel.MeshDescription = TUniquePtr<FMeshDescription>(worldModelMesh);

	AStaticMeshActor* staticMeshActor = world->SpawnActor<AStaticMeshActor>();
	staticMeshActor->GetStaticMeshComponent()->SetStaticMesh(staticMesh);

	GEditor->SelectActor(staticMeshActor, true, false, true, false);
	folders.SetSelectedFolderPath(geometryFolder);

	return true;
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

void FBSPImporter::GatherFaces(const Valve::BSPFile& bspFile, uint32 nodeIndex, TArray<uint16>& out)
{
	const Valve::BSP::snode_t& node = bspFile.m_Nodes[nodeIndex];
	if (node.m_Children[0] < 0)
	{
		const Valve::BSP::dleaf_t& leaf = bspFile.m_Leaves[-1 - node.m_Children[0]];
		for (uint32 i = 0; i < leaf.m_Numleaffaces; ++i)
		{
			out.AddUnique(bspFile.m_Leaffaces[leaf.m_Firstleafface + i]);
		}
	}
	else
	{
		GatherFaces(bspFile, (uint32)node.m_Children[0], out);
	}
	if (node.m_Children[1] < 0)
	{
		const Valve::BSP::dleaf_t& leaf = bspFile.m_Leaves[-1 - node.m_Children[1]];
		for (uint32 i = 0; i < leaf.m_Numleaffaces; ++i)
		{
			out.AddUnique(bspFile.m_Leaffaces[leaf.m_Firstleafface + i]);
		}
	}
	else
	{
		GatherFaces(bspFile, (uint32)node.m_Children[1], out);
	}
}

FPlane FBSPImporter::ValveToUnrealPlane(const Valve::BSP::cplane_t& plane)
{
	return FPlane(plane.m_Normal(0, 0), plane.m_Normal(0, 1), plane.m_Normal(0, 2), plane.m_Distance);
}

void FBSPImporter::RenderNodeToMesh(const Valve::BSPFile& bspFile, uint16 nodeIndex, FMeshDescription& meshDesc)
{
	// Gather all faces to render out
	TArray<uint16> faceIndices;
	GatherFaces(bspFile, nodeIndex, faceIndices);

	TMap<uint32, FVertexID> valveToUnrealVertexMap;
	//TMap<int32, FEdgeID> valveToUnrealEdgeMap;
	TMap<uint16, FPolygonGroupID> texDataToPolyGroupMap;

	TAttributesSet<FVertexID>& vertexAttr = meshDesc.VertexAttributes();
	vertexAttr.RegisterAttribute<FVector>(MeshAttribute::Vertex::Position);
	TMeshAttributesRef<FVertexID, FVector> vertexAttrPosition = vertexAttr.GetAttributesRef<FVector>(MeshAttribute::Vertex::Position);

	// Iterate all faces
	for (const uint16 faceIndex : faceIndices)
	{
		const Valve::BSP::dface_t& bspFace = bspFile.m_Surfaces[faceIndex];
		const Valve::BSP::texinfo_t& bspTexInfo = bspFile.m_Texinfos[bspFace.m_Texinfo];
		const uint16 texDataIndex = (uint16)bspTexInfo.m_Texdata;
		const Valve::BSP::texdata_t& bspTexData = bspFile.m_Texdatas[bspTexInfo.m_Texdata];

		// Create polygroup if needed (we make one per material/texdata)
		FPolygonGroupID polyGroup;
		if (!texDataToPolyGroupMap.Contains(texDataIndex))
		{
			polyGroup = meshDesc.CreatePolygonGroup();
			texDataToPolyGroupMap.Add(texDataIndex, polyGroup);
		}
		else
		{
			polyGroup = texDataToPolyGroupMap[texDataIndex];
		}

		TArray<FVertexInstanceID> polyVerts;
		TSet<uint16> bspVerts; // track all vbsp verts we've visited as it likes to revisit them sometimes and cause degenerate polys
		
		// Iterate all edges
		for (uint16 i = 0; i < bspFace.m_Numedges; ++i)
		{
			const int32 surfEdge = bspFile.m_Surfedges[bspFace.m_Firstedge + i];
			const uint32 edgeIndex = (uint32)(surfEdge < 0 ? -surfEdge : surfEdge);

			const Valve::BSP::dedge_t& bspEdge = bspFile.m_Edges[edgeIndex];
			const uint16 vertAIndex = surfEdge < 0 ? bspEdge.m_V[1] : bspEdge.m_V[0];
			//const uint16 vertBIndex = surfEdge < 0 ? bspEdge.m_V[0] : bspEdge.m_V[1];

			if (bspVerts.Contains(vertAIndex)) { break; }
			bspVerts.Add(vertAIndex);

			// Create vertices if needed
			FVertexID vertA;
			if (!valveToUnrealVertexMap.Contains(vertAIndex))
			{
				vertA = meshDesc.CreateVertex();
				valveToUnrealVertexMap.Add(vertAIndex, vertA);
				const Valve::BSP::mvertex_t& bspVertex = bspFile.m_Vertexes[vertAIndex];
				vertexAttrPosition[vertA] = FVector(bspVertex.m_Position(0, 0), bspVertex.m_Position(0, 1), bspVertex.m_Position(0, 2));
			}
			else
			{
				vertA = valveToUnrealVertexMap[vertAIndex];
			}
			/*FVertexID vertB;
			if (!valveToUnrealVertexMap.Contains(vertBIndex))
			{
				vertB = meshDesc.CreateVertex();
				valveToUnrealVertexMap.Add(vertBIndex, vertB);
				const Valve::BSP::mvertex_t& bspVertex = bspFile.m_Vertexes[vertBIndex];
				vertexAttrPosition[vertB] = FVector(bspVertex.m_Position(0, 0), bspVertex.m_Position(0, 1), bspVertex.m_Position(0, 2));
			}
			else
			{
				vertB = valveToUnrealVertexMap[vertBIndex];
			}*/

			// Create edge if needed
			/*FEdgeID edge;
			if (!valveToUnrealEdgeMap.Contains(surfEdge))
			{
				edge = meshDesc.CreateEdge(vertA, vertB);
				valveToUnrealEdgeMap.Add(surfEdge, edge);
			}
			else
			{
				edge = valveToUnrealEdgeMap[surfEdge];
			}*/

			// Create vertex instance for vertA only
			FVertexInstanceID vertInst = meshDesc.CreateVertexInstance(vertA);

			// TODO: Texture coord
			/* texcoords.Add(new Vector2(
				(poly.TextureU.XYZ.Dot(poly.Points[i]) + poly.TextureU.W) / poly.TexSize.X,
				(poly.TextureV.XYZ.Dot(poly.Points[i]) + poly.TextureV.W) / poly.TexSize.Y
			));*/

			// Push
			polyVerts.Add(vertInst);
		}

		// Create poly
		if (polyVerts.Num() > 2)
		{
			meshDesc.CreatePolygon(polyGroup, polyVerts);
		}
	}

	// Clean up
	//meshDesc.ComputePolygonTangentsAndNormals();
	
}

#endif