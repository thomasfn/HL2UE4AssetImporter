#include "BSPBrushUtils.h"

#include "MeshAttributes.h"
#include "StaticMeshAttributes.h"
#include "Engine/Polys.h"
#include "IHL2Runtime.h"
#include "SourceCoord.h"

constexpr float snapThreshold = 1.0f / 4.0f;

FBSPBrushUtils::FBSPBrushUtils()
{
}

void FBSPBrushUtils::BuildBrushGeometry(const FBSPBrush& brush, FMeshDescription& meshDesc)
{
	const int sideNum = brush.Sides.Num();

	FStaticMeshAttributes staticMeshAttr(meshDesc);
	
	TMeshAttributesRef<FVertexID, FVector3f> vertexPosAttr = staticMeshAttr.GetVertexPositions();
	TMeshAttributesRef<FVertexInstanceID, FVector2f> vertexInstUVAttr = staticMeshAttr.GetVertexInstanceUVs();
	TMeshAttributesRef<FPolygonGroupID, FName> polyGroupMaterialAttr = staticMeshAttr.GetPolygonGroupMaterialSlotNames();
	TMeshAttributesRef<FEdgeID, bool> edgeIsHardAttr = staticMeshAttr.GetEdgeHardnesses();

	// Iterate all planes
	TMap<FPolygonID, int> polyToSideMap;
	polyToSideMap.Reserve(sideNum);
	for (int i = 0; i < sideNum; ++i)
	{
		const FBSPBrushSide& side = brush.Sides[i];
		if (!side.EmitGeometry) { continue; }

		// Check that the texture is aligned to the face - if not, discard the face
		const FVector3f textureNorm = FVector3f::CrossProduct((FVector3f)side.TextureU, (FVector3f)side.TextureV).GetUnsafeNormal();
		if (FMath::Abs(FVector3f::DotProduct(textureNorm, side.Plane)) < 0.1f) { continue; }

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
				if (polyGroupMaterialAttr[otherPolyGroupID] == side.Material)
				{
					found = true;
					polyGroupID = otherPolyGroupID;
					break;
				}
			}
			if (!found)
			{
				polyGroupID = meshDesc.CreatePolygonGroup();
				polyGroupMaterialAttr[polyGroupID] = side.Material;
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
			
		}

		// Create poly
		if (SourceToUnreal.ShouldReverseWinding())
		{
			TArray< FVertexInstanceID> tmp = polyContour;
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

inline void FBSPBrushUtils::SnapVertex(FVector3f& vertex)
{
	vertex.X = FMath::GridSnap(vertex.X, snapThreshold);
	vertex.Y = FMath::GridSnap(vertex.Y, snapThreshold);
	vertex.Z = FMath::GridSnap(vertex.Z, snapThreshold);
}