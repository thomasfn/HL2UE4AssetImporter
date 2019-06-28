#pragma once

#include "CoreMinimal.h"
#include "MeshDescription.h"

#include "PhysicsEngine/BodySetup.h"

struct FMeshCleanSettings
{
	bool RemoveDegeneratePolys : 1;
	bool RemoveUnusedEdges : 1;
	bool RemoveUnusedVertexInstances : 1;
	bool RemoveUnusedVertices : 1;
	bool RemoveEmptyPolyGroups : 1;
	bool Compact : 1;
	bool Retriangulate : 1;

	static const FMeshCleanSettings Default;
	static const FMeshCleanSettings None;
	static const FMeshCleanSettings All;

	FMeshCleanSettings(bool removeDegeneratePolys, bool removeUnusedEdges, bool removeUnusedVertexInstances, bool removeUnusedVertices, bool removeEmptyPolyGroups, bool compact, bool retriangulate);
};

class FMeshUtils
{
private:

	FMeshUtils();
	
public:

	/**
	 * Clips a mesh and removes all geometry behind the specified planes.
	 * Any polygons intersecting a plane will be cut.
	 * Normals, tangents and texture coordinates will be preserved.
	 */
	static void Clip(FMeshDescription& meshDesc, const TArray<FPlane>& clipPlanes);

	/**
	 * Cleans a mesh, removing degenerate edges and polys, and removing unused elements.
	 */
	static void Clean(FMeshDescription& meshDesc, const FMeshCleanSettings& settings = FMeshCleanSettings::Default);

	/**
	 * Generates lightmap coordinates into uv channel 1, using only topology (e.g. not using uvs).
	 */
	static void GenerateLightmapCoords(FMeshDescription& meshDesc, int lightmapResolution);

	/**
	 * Decomposes a UCX mesh into a body setup.
	 */
	static void DecomposeUCXMesh(const TArray<FVector>& CollisionVertices, const TArray<int32>& CollisionFaceIdx, UBodySetup* BodySetup);

	/**
	 * Finds the area of a triangle with the given points.
	 */
	static inline float AreaOfTriangle(const FVector& v0, const FVector& v1, const FVector& v2);

	/**
	 * Finds the total surface area of a mesh.
	 */
	static float FindSurfaceArea(const FMeshDescription& meshDesc);

private:

	static FVertexInstanceID ClipEdge(FMeshDescription& meshDesc, const FVertexInstanceID& a, const FVertexInstanceID& b, const FPlane& clipPlane);
	
	static FPlane DerivePolygonPlane(const FMeshDescription& meshDesc, const FPolygonID polyID);

	static void DerivePlanarProjection(const FPlane& plane, FMatrix& projectionMatrix);
};
