#pragma once

#include "UnrealString.h"
#include "ValveBSP/BSPFile.hpp"
#include "MeshDescription.h"
#include "EntityParser.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHL2BSPImporter, Log, All);

class FBSPImporter
{
public:

	FBSPImporter();
	virtual ~FBSPImporter();
	
#ifdef WITH_EDITOR

	bool ImportToCurrentLevel(const FString& fileName);

	bool ImportToWorld(const Valve::BSPFile& bspFile, UWorld* world);

	bool ImportBrushesToWorld(const Valve::BSPFile& bspFile, UWorld* world);

	bool ImportGeometryToWorld(const Valve::BSPFile& bspFile, UWorld* world);

	bool ImportEntitiesToWorld(const Valve::BSPFile& bspFile, UWorld* world);

#endif

private:

#ifdef WITH_EDITOR

	TSet<uint16> faceDedup;

	AActor* ImportBrush(UWorld* world, const Valve::BSPFile& bspFile, uint16 brushIndex);

	void GatherBrushes(const Valve::BSPFile& bspFile, uint32 nodeIndex, TArray<uint16>& out);

	void GatherFaces(const Valve::BSPFile& bspFile, uint32 nodeIndex, TArray<uint16>& out, TSet<int16>* clusterFilter = nullptr);

	void GatherDisplacements(const Valve::BSPFile& bspFile, const TArray<uint16>& faceIndices, TArray<uint16>& out);

	void GatherClusters(const Valve::BSPFile& bspFile, uint32 nodeIndex, TArray<int16>& out);

	FPlane ValveToUnrealPlane(const Valve::BSP::cplane_t& plane);

	void RenderTreeToActors(const Valve::BSPFile& bspFile, UWorld* world, TArray<AStaticMeshActor*>& out, uint32 nodeIndex);

	AStaticMeshActor* RenderMeshToActor(UWorld* world, const FMeshDescription& meshDesc);

	void RenderFacesToMesh(const Valve::BSPFile& bspFile, const TArray<uint16>& faceIndices, FMeshDescription& meshDesc, bool skyboxFilter);

	void RenderDisplacementsToMesh(const Valve::BSPFile& bspFile, const TArray<uint16>& displacements, FMeshDescription& meshDesc);

	FString ParseMaterialName(const char* bspMaterialName);

	static bool SharesSmoothingGroup(uint16 groupA, uint16 groupB);

	AActor* ImportEntityToWorld(UWorld* world, const FHL2Entity& entityData);

#endif

};