#pragma once

#include "MeshSplitter.h"

#include "MeshAttributes.h"

FMeshSplitter::FMeshSplitter() { }

/**
 * Clips a mesh and removes all geometry behind the specified planes.
 * Any polygons intersecting a plane will be cut.
 * Normals, tangents and texture coordinates will be preserved.
 */
void FMeshSplitter::Clip(FMeshDescription& meshDesc, const TArray<FPlane>& clipPlanes)
{
	// Get attributes
	const TAttributesSet<FVertexID>& vertexAttr = meshDesc.VertexAttributes();
	TMeshAttributesConstRef<FVertexID, FVector> vertexAttrPosition = vertexAttr.GetAttributesRef<FVector>(MeshAttribute::Vertex::Position);

	TArray<FVertexInstanceID> arr1, arr2;

	// Iterate all polys
	TArray<FPolygonID> allPolyIDs;
	for (const FPolygonID& polyID : meshDesc.Polygons().GetElementIDs())
	{
		allPolyIDs.Add(polyID);
	}
	for (const FPolygonID& polyID : allPolyIDs)
	{
		FMeshPolygon poly = meshDesc.GetPolygon(polyID);
		const FPolygonGroupID& polyGroupID = meshDesc.GetPolygonPolygonGroup(polyID);
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

				const FVertexID& vertID = meshDesc.GetVertexInstanceVertex(vertInstID);
				const FVector vertPos = vertexAttrPosition[vertID];

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
						newPoly->Add(ClipEdge(meshDesc, lastVertInstID, vertInstID, plane));
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
			meshDesc.DeletePolygon(polyID);
		}
		// If some form of clipping happened, replace it
		else if (changesMade)
		{
			meshDesc.DeletePolygon(polyID);
			meshDesc.CreatePolygon(polyGroupID, *newPoly);
		}
	}

	// Clean up after ourselves
	CleanMesh(meshDesc);
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
	TMeshAttributesRef<FVertexInstanceID, FVector4> vertexInstAttrCol = vertexInstAttr.GetAttributesRef<FVector4>(MeshAttribute::VertexInstance::Color);

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
	vertexInstAttrCol[newVertInstID] = FMath::Lerp(vertexInstAttrCol[vertAInstID], vertexInstAttrCol[vertBInstID], mu);

	return newVertInstID;
}

void FMeshSplitter::CleanMesh(FMeshDescription& meshDesc)
{
	// Delete degenerate polygons
	TAttributesSet<FVertexID>& vertexAttr = meshDesc.VertexAttributes();
	TMeshAttributesRef<FVertexID, FVector> vertexAttrPosition = vertexAttr.GetAttributesRef<FVector>(MeshAttribute::Vertex::Position);
	for (const FPolygonID& polyID : meshDesc.Polygons().GetElementIDs())
	{
		FMeshPolygon& poly = meshDesc.GetPolygon(polyID);
		const int numVerts = poly.PerimeterContour.VertexInstanceIDs.Num();
		TArray<FVertexInstanceID> toDelete;
		for (int i = 0; i < numVerts; ++i)
		{
			const FVertexInstanceID& vertAinstID = poly.PerimeterContour.VertexInstanceIDs[i];
			const FVertexID& vertAID = meshDesc.GetVertexInstanceVertex(vertAinstID);
			const FVector& vertAPos = vertexAttrPosition[vertAID];
			for (int j = i + 1; j < numVerts; ++j)
			{
				const FVertexInstanceID& vertBinstID = poly.PerimeterContour.VertexInstanceIDs[j];
				const FVertexID& vertBID = meshDesc.GetVertexInstanceVertex(vertBinstID);
				const FVector& vertBPos = vertexAttrPosition[vertBID];
				if (vertAPos.Equals(vertBPos, 0.00001f))
				{
					toDelete.AddUnique(vertBinstID);
				}
			}
		}
		if (numVerts - toDelete.Num() < 3)
		{
			meshDesc.DeletePolygon(polyID);
		}
		else if (toDelete.Num() > 0)
		{
			TArray<FVertexInstanceID> newPerimeterContour = poly.PerimeterContour.VertexInstanceIDs;
			for (const FVertexInstanceID& vertInstID : toDelete)
			{
				newPerimeterContour.Remove(vertInstID);
			}
			const FPolygonGroupID polyGroupID = meshDesc.GetPolygonPolygonGroup(polyID);
			meshDesc.DeletePolygon(polyID);
			meshDesc.CreatePolygonWithID(polyID, polyGroupID, newPerimeterContour);
		}
	}

	// Delete unused edges
	for (const FEdgeID& edgeID : meshDesc.Edges().GetElementIDs())
	{
		if (meshDesc.GetEdge(edgeID).ConnectedPolygons.Num() == 0)
		{
			meshDesc.DeleteEdge(edgeID);
		}
	}

	//// Delete unused vertex instances
	for (const FVertexInstanceID& vertInstID : meshDesc.VertexInstances().GetElementIDs())
	{
		if (meshDesc.GetVertexInstance(vertInstID).ConnectedPolygons.Num() == 0)
		{
			meshDesc.DeleteVertexInstance(vertInstID);
		}
	}

	//// Delete unused vertices
	for (const FVertexID& vertID : meshDesc.Vertices().GetElementIDs())
	{
		if (meshDesc.GetVertex(vertID).VertexInstanceIDs.Num() == 0)
		{
			meshDesc.DeleteVertex(vertID);
		}
	}

	//// Delete any empty polygon groups
	for (const FPolygonGroupID& polyGroupID : meshDesc.PolygonGroups().GetElementIDs())
	{
		if (meshDesc.GetPolygonGroup(polyGroupID).Polygons.Num() == 0)
		{
			meshDesc.DeletePolygonGroup(polyGroupID);
		}
	}

	// Remap element IDs
	FElementIDRemappings remappings;
	meshDesc.Compact(remappings);

	// Retriangulate mesh
	meshDesc.TriangulateMesh();
}