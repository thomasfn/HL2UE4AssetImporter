#pragma once

#include "MeshSplitter.h"

#include "MeshAttributes.h"

FMeshSplitter::FMeshSplitter() { }

/**
 * Clips a mesh and removes all geometry behind the specified planes.
 * Any polygons intersecting a plane will be cut.
 * Normals, tangents and texture coordinates will be preserved.
 */
void FMeshSplitter::Clip(const FMeshDescription& inMesh, FMeshDescription& outMesh, const TArray<FPlane>& clipPlanes)
{
	outMesh = inMesh;

	// Get attributes
	const TAttributesSet<FVertexID>& inVertexAttr = inMesh.VertexAttributes();
	TMeshAttributesConstRef<FVertexID, FVector> inVertexAttrPosition = inVertexAttr.GetAttributesRef<FVector>(MeshAttribute::Vertex::Position);

	TArray<FVertexInstanceID> arr1, arr2;

	// Iterate all polys
	for (const FPolygonID& polyID : inMesh.Polygons().GetElementIDs())
	{
		FMeshPolygon poly = inMesh.GetPolygon(polyID);
		const FPolygonGroupID& polyGroupID = inMesh.GetPolygonPolygonGroup(polyID);
		TArray<FVertexInstanceID>* oldPoly = &arr1;
		TArray<FVertexInstanceID>* newPoly = &arr2;

		newPoly->Empty(poly.PerimeterContour.VertexInstanceIDs.Num());
		newPoly->Append(poly.PerimeterContour.VertexInstanceIDs);

		// Iterate all planes
		bool changesMade = false;
		for (const FPlane& plane : clipPlanes)
		{
			Swap(oldPoly, newPoly);
			newPoly->Empty(oldPoly->Num());
			bool firstVert = true, lastWasClipped = false;
			FVertexInstanceID lastVertInstID;

			// Go through each vert and identify ones that move from one side of the plane to the other
			for (int i = 0, l = oldPoly->Num(); i <= l; ++i)
			{
				const bool last = i == l;
				const FVertexInstanceID& vertInstID = (*oldPoly)[i % l];

				const FVertexID& vertID = inMesh.GetVertexInstanceVertex(vertInstID);
				const FVector vertPos = inVertexAttrPosition[vertID];

				const bool isClipped = plane.PlaneDot(vertPos) < 0.0f;
				if (isClipped) { changesMade = true; }
				if (firstVert)
				{
					firstVert = false;
				}
				else
				{
					if (isClipped != lastWasClipped)
					{
						newPoly->Add(ClipEdge(outMesh, lastVertInstID, vertInstID, plane));
					}
				}
				if (!isClipped && !last)
				{
					newPoly->Add(vertInstID);
				}
				lastWasClipped = isClipped;
				lastVertInstID = vertInstID;
			}

			// If there's no poly left, early out
			if (newPoly->Num() < 3) { break; }
		}

		// If there's no a poly left, delete it
		if (newPoly->Num() < 3)
		{
			outMesh.DeletePolygon(polyID);
		}
		// If some form of clipping happened, replace it
		else if (changesMade)
		{
			outMesh.DeletePolygon(polyID);
			outMesh.CreatePolygonWithID(polyID, polyGroupID, *newPoly);
		}
	}

	// Delete all empty poly groups
	for (const FPolygonGroupID& polyGroupID : inMesh.PolygonGroups().GetElementIDs())
	{
		if (outMesh.GetPolygonGroup(polyGroupID).Polygons.Num() == 0)
		{
			outMesh.DeletePolygonGroup(polyGroupID);
		}
	}

	// Clean up
	FElementIDRemappings remappings;
	outMesh.Compact(remappings);
}

FVertexInstanceID FMeshSplitter::ClipEdge(FMeshDescription& meshDesc, const FVertexInstanceID& vertAInstID, const FVertexInstanceID& vertBInstID, const FPlane& clipPlane)
{
	// Lookup base vertices
	const FVertexID& vertAID = meshDesc.GetVertexInstanceVertex(vertAInstID);
	const FVertexID& vertBID = meshDesc.GetVertexInstanceVertex(vertBInstID);

	// Grab all mesh attributes
	TAttributesSet<FVertexID>& vertexAttr = meshDesc.VertexAttributes();
	TMeshAttributesRef<FVertexID, FVector> vertexAttrPosition = vertexAttr.GetAttributesRef<FVector>(MeshAttribute::Vertex::Position);
	TAttributesSet<FVertexInstanceID>& vertexInstAttr = meshDesc.VertexInstanceAttributes();
	TMeshAttributesRef<FVertexInstanceID, FVector> vertexInstAttrNormal = vertexInstAttr.GetAttributesRef<FVector>(MeshAttribute::VertexInstance::Normal);
	TMeshAttributesRef<FVertexInstanceID, FVector> vertexInstAttrTangent = vertexInstAttr.GetAttributesRef<FVector>(MeshAttribute::VertexInstance::Tangent);
	TMeshAttributesRef<FVertexInstanceID, FVector2D> vertexInstAttrUV0 = vertexInstAttr.GetAttributesRef<FVector2D>(MeshAttribute::VertexInstance::TextureCoordinate);

	// Lookup vertex positions
	const FVector& vertAPos = vertexAttrPosition[vertAID];
	const FVector& vertBPos = vertexAttrPosition[vertBID];

	// Intersect the line from vertA to vertB with the plane
	float mu;
	{
		const FVector pointOnPlane = FVector::PointPlaneProject(FVector::ZeroVector, clipPlane);
		mu = FMath::Clamp(FVector::DotProduct(pointOnPlane - vertAPos, clipPlane) / FVector::DotProduct(clipPlane, vertBPos - vertAPos), 0.0f, 1.0f);
	}

	// Create new vertex
	const FVertexID newVertID = meshDesc.CreateVertex();
	vertexAttrPosition[newVertID] = FMath::Lerp(vertAPos, vertBPos, mu);

	// Create new vertex instance
	const FVertexInstanceID newVertInstID = meshDesc.CreateVertexInstance(newVertID);
	vertexInstAttrNormal[newVertInstID] = FMath::Lerp(vertexInstAttrNormal[vertAInstID], vertexInstAttrNormal[vertBInstID], mu).GetUnsafeNormal();
	vertexInstAttrTangent[newVertInstID] = FMath::Lerp(vertexInstAttrTangent[vertAInstID], vertexInstAttrTangent[vertBInstID], mu).GetUnsafeNormal();
	vertexInstAttrUV0[newVertInstID] = FMath::Lerp(vertexInstAttrUV0[vertAInstID], vertexInstAttrUV0[vertBInstID], mu);

	return newVertInstID;
}