#include "BSPBrushUtils.h"

#include "MeshAttributes.h"
#include "StaticMeshAttributes.h"
#include "Engine/Polys.h"
#include "IHL2Runtime.h"
#include "SourceCoord.h"
#include "MeshUtils.h"

constexpr float snapThreshold = 1.0f / 4.0f;

FBSPBrushUtils::FBSPBrushUtils()
{
}

void FBSPBrushUtils::BuildBrushGeometry(const FBSPBrush& brush, FMeshDescription& meshDesc, FName overrideMaterial, bool alwaysEmitFaces)
{
	const int sideNum = brush.Sides.Num();

	FStaticMeshAttributes staticMeshAttr(meshDesc);
	
	TMeshAttributesRef<FVertexID, FVector3f> vertexPosAttr = staticMeshAttr.GetVertexPositions();
	TMeshAttributesRef<FVertexInstanceID, FVector2f> vertexInstUVAttr = staticMeshAttr.GetVertexInstanceUVs();
	TMeshAttributesRef<FVertexInstanceID, FVector4f> vertexInstColorAttr = staticMeshAttr.GetVertexInstanceColors();
	TMeshAttributesRef<FPolygonGroupID, FName> polyGroupMaterialAttr = staticMeshAttr.GetPolygonGroupMaterialSlotNames();
	TMeshAttributesRef<FEdgeID, bool> edgeIsHardAttr = staticMeshAttr.GetEdgeHardnesses();

	static const FName fnBlack("tools/toolsblack");

	// Iterate all planes
	TMap<FPolygonID, int> polyToSideMap;
	polyToSideMap.Reserve(sideNum);
	for (int i = 0; i < sideNum; ++i)
	{
		const FBSPBrushSide& side = brush.Sides[i];
		if (!side.EmitGeometry && !alwaysEmitFaces) { continue; }

		// Check that the texture is aligned to the face - if not, discard the face
		const FVector3f textureNorm = FVector3f::CrossProduct((FVector3f)side.TextureU, (FVector3f)side.TextureV).GetUnsafeNormal();
		const bool textureUVValid = FMath::Abs(FVector3f::DotProduct(textureNorm, side.Plane)) >= 0.1f;
		if (side.Material != fnBlack && !textureUVValid && !alwaysEmitFaces) { continue; }
		const FName material = overrideMaterial != NAME_None ? overrideMaterial : !textureUVValid ? fnBlack : side.Material;

		// Create a poly for this side
		FPoly poly = FPoly::BuildInfiniteFPoly((FPlane)side.Plane);
		
		// Iterate all other sides
		int numVerts = 0;
		for (int j = 0; j < sideNum; ++j)
		{
			if (j != i)
			{
				const FBSPBrushSide& otherSide = brush.Sides[j];

				// Slice poly with it
				const FVector3f normal = FVector3f(otherSide.Plane) * -1.0f;
				numVerts = poly.Split(normal, normal * otherSide.Plane.W * -1.0f);
				if (numVerts < 3) { break; }
			}
		}

		// Check if we have a valid polygon
		if (numVerts < 3) { continue; }
		numVerts = poly.Fix();
		if (numVerts < 3) { continue; }

		// Get or create polygon group
		FPolygonGroupID polyGroupID;
		{
			bool found = false;
			for (const FPolygonGroupID& otherPolyGroupID : meshDesc.PolygonGroups().GetElementIDs())
			{
				if (polyGroupMaterialAttr[otherPolyGroupID] == material)
				{
					found = true;
					polyGroupID = otherPolyGroupID;
					break;
				}
			}
			if (!found)
			{
				polyGroupID = meshDesc.CreatePolygonGroup();
				polyGroupMaterialAttr[polyGroupID] = material;
			}
		}
		
		// Create vertices
		TArray<FVertexInstanceID> polyContour;
		TSet<FVertexID> visited;
		visited.Reserve(poly.Vertices.Num());
		/*for (FVector& pos : poly.Vertices)
		{
			SnapVertex(pos);
		}*/
		for (const FVector3f& pos : poly.Vertices)
		{
			// Get or create vertex
			FVertexID vertID;
			{
				bool found = false;
				for (const FVertexID& otherVertID : meshDesc.Vertices().GetElementIDs())
				{
					const FVector3f& otherPos = vertexPosAttr[otherVertID];
					if (otherPos.Equals(pos, snapThreshold))
					{
						found = true;
						vertID = otherVertID;

						break;
					}
				}
				if (!found)
				{
					vertID = meshDesc.CreateVertex();
					vertexPosAttr[vertID] = SourceToUnreal.Position(pos);
				}
			}
			if (visited.Contains(vertID)) { continue; }
			visited.Add(vertID);

			// Create vertex instance
			FVertexInstanceID vertInstID = meshDesc.CreateVertexInstance(vertID);
			polyContour.Add(vertInstID);

			// Calculate UV
			const FVector3f vertPos = UnrealToSource.Position(vertexPosAttr[vertID]);
			vertexInstUVAttr.Set(vertInstID, 0, FVector2f(
				(FVector3f::DotProduct(side.TextureU, vertPos) + side.TextureU.W) / side.TextureW,
				(FVector3f::DotProduct(side.TextureV, vertPos) + side.TextureV.W) / side.TextureH
			));
			vertexInstColorAttr[vertInstID] = FVector4f(0.0f, 1.0f, 1.0f, 1.0f);
			
		}

		// Create poly
		if (SourceToUnreal.ShouldReverseWinding())
		{
			TArray<FVertexInstanceID> tmp = polyContour;
			for (int j = 0; j < tmp.Num(); ++j)
			{
				polyContour[tmp.Num() - (j + 1)] = tmp[j];
			}
		}
		polyToSideMap.Add(meshDesc.CreatePolygon(polyGroupID, polyContour), i);
	}

	// Apply smoothing groups
	for (const auto& pair : polyToSideMap)
	{
		const FBSPBrushSide& side = brush.Sides[pair.Value];

		TArray<FEdgeID> edgeIDs;
		meshDesc.GetPolygonPerimeterEdges(pair.Key, edgeIDs);
		for (const FEdgeID& edgeID : edgeIDs)
		{
			const TArray<FPolygonID>& connectedPolyIDs = meshDesc.GetEdgeConnectedPolygons(edgeID);
			uint32 accumSmoothingGroup = side.SmoothingGroups;
			for (const FPolygonID& otherPolyID : connectedPolyIDs)
			{
				if (otherPolyID != pair.Key)
				{
					int* otherSideIndex = polyToSideMap.Find(otherPolyID);
					if (otherSideIndex != nullptr)
					{
						const FBSPBrushSide& otherSide = brush.Sides[*otherSideIndex];
						accumSmoothingGroup &= otherSide.SmoothingGroups;
					}
				}
			}
			if (accumSmoothingGroup > 0)
			{
				edgeIsHardAttr[edgeID] = false;
			}
			else
			{
				edgeIsHardAttr[edgeID] = true;
			}
		}
	}
}

void FBSPBrushUtils::BuildBrushCollision(const FBSPBrush& brush, UBodySetup* bodySetup)
{
	const int sideNum = brush.Sides.Num();

	TArray<FVector3f> vertices;
	TArray<int> indices;
	
	// Iterate all planes
	TMap<FPolygonID, int> polyToSideMap;
	polyToSideMap.Reserve(sideNum);
	for (int i = 0; i < sideNum; ++i)
	{
		const FBSPBrushSide& side = brush.Sides[i];

		// Create a poly for this side
		FPoly poly = FPoly::BuildInfiniteFPoly((FPlane)side.Plane);

		// Iterate all other sides
		int numVerts = 0;
		for (int j = 0; j < sideNum; ++j)
		{
			if (j != i)
			{
				const FBSPBrushSide& otherSide = brush.Sides[j];

				// Slice poly with it
				const FVector3f normal = FVector3f(otherSide.Plane) * -1.0f;
				numVerts = poly.Split(normal, normal * otherSide.Plane.W * -1.0f);
				if (numVerts < 3) { break; }
			}
		}

		// Check if we have a valid polygon
		if (numVerts < 3) { continue; }
		numVerts = poly.Fix();
		if (numVerts < 3) { continue; }

		// Build contour
		TArray<int> contour;
		contour.Reserve(numVerts);
		for (const FVector3f& pos : poly.Vertices)
		{
			// Get or create vertex
			int vertIndex;
			bool found = false;
			for (int otherVertIndex = 0; otherVertIndex < vertices.Num(); ++otherVertIndex)
			{
				const FVector3f& otherPos = vertices[otherVertIndex];
				if (otherPos.Equals(pos, snapThreshold))
				{
					found = true;
					vertIndex = otherVertIndex;
					break;
				}
			}
			if (!found)
			{
				vertIndex = vertices.Add(SourceToUnreal.Position(pos));
			}
			contour.Add(vertIndex);
		}

		// Triangulate
		const int numTris = numVerts - 2;
		indices.Reserve(indices.Num() + 3 * numTris);
		for (int j = 0; j < numTris; ++j)
		{
			if (SourceToUnreal.ShouldReverseWinding())
			{
				indices.Add(contour[j + 2]);
				indices.Add(contour[j + 1]);
				indices.Add(contour[0]);
			}
			else
			{
				indices.Add(contour[0]);
				indices.Add(contour[j + 1]);
				indices.Add(contour[j + 2]);
			}
		}
	}

	FMeshUtils::DecomposeUCXMesh(vertices, indices, bodySetup);
}

inline void FBSPBrushUtils::SnapVertex(FVector3f& vertex)
{
	vertex.X = FMath::GridSnap(vertex.X, snapThreshold);
	vertex.Y = FMath::GridSnap(vertex.Y, snapThreshold);
	vertex.Z = FMath::GridSnap(vertex.Z, snapThreshold);
}
