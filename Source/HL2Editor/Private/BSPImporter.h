#pragma once

#include "CoreMinimal.h"
#include "ValveBSP/BSPFile.hpp"
#include "MeshDescription.h"
#include "HL2EntityData.h"
#include "EntityParser.h"
#include "BaseEntity.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHL2BSPImporter, Log, All);

class FBSPImporter
{
private:

	FBSPImporter();
	
public:

	static bool ImportToCurrentLevel(const FString& fileName);
	static bool ImportToWorld(const Valve::BSPFile& bspFile, UWorld* world);
	static bool ImportBrushesToWorld(const Valve::BSPFile& bspFile, UWorld* world);
	static bool ImportGeometryToWorld(const Valve::BSPFile& bspFile, UWorld* world);
	static bool ImportEntitiesToWorld(const Valve::BSPFile& bspFile, UWorld* world);

private:

	static TSet<uint16> faceDedup;

	static AActor* ImportBrush(UWorld* world, const Valve::BSPFile& bspFile, uint16 brushIndex);

	static void GatherBrushes(const Valve::BSPFile& bspFile, uint32 nodeIndex, TArray<uint16>& out);
	 
	static void GatherFaces(const Valve::BSPFile& bspFile, uint32 nodeIndex, TArray<uint16>& out, TSet<int16>* clusterFilter = nullptr);
	 
	static void GatherDisplacements(const Valve::BSPFile& bspFile, const TArray<uint16>& faceIndices, TArray<uint16>& out);
	 
	static void GatherClusters(const Valve::BSPFile& bspFile, uint32 nodeIndex, TArray<int16>& out);
	 
	static FPlane ValveToUnrealPlane(const Valve::BSP::cplane_t& plane);
	 
	static void RenderTreeToActors(const Valve::BSPFile& bspFile, UWorld* world, TArray<AStaticMeshActor*>& out, uint32 nodeIndex);
	 
	static AStaticMeshActor* RenderMeshToActor(UWorld* world, const FMeshDescription& meshDesc);
	 
	static void RenderFacesToMesh(const Valve::BSPFile& bspFile, const TArray<uint16>& faceIndices, FMeshDescription& meshDesc, bool skyboxFilter);
	 
	static void RenderDisplacementsToMesh(const Valve::BSPFile& bspFile, const TArray<uint16>& displacements, FMeshDescription& meshDesc);
	 
	static FString ParseMaterialName(const char* bspMaterialName);
	 
	static bool SharesSmoothingGroup(uint16 groupA, uint16 groupB);
	 
	static ABaseEntity* ImportEntityToWorld(UWorld* world, const FHL2EntityData& entityData);

};