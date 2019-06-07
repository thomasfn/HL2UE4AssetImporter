#pragma once

#include "CoreMinimal.h"
#include "MeshDescription.h"

class FMeshSplitter
{
private:

	FMeshSplitter();
	
public:

	/**
	 * Clips a mesh and removes all geometry behind the specified planes.
	 * Any polygons intersecting a plane will be cut.
	 * Normals, tangents and texture coordinates will be preserved.
	 */
	static void Clip(FMeshDescription& meshDesc, const TArray<FPlane>& clipPlanes);

private:

	static FVertexInstanceID ClipEdge(FMeshDescription& meshDesc, const FVertexInstanceID& a, const FVertexInstanceID& b, const FPlane& clipPlane);

	static void CleanMesh(FMeshDescription& meshDesc);
};
