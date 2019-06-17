#pragma once

#include "CoreMinimal.h"
#include "ValveBSP/BSPFile.hpp"
#include "MeshDescription.h"
#include "HL2EntityData.h"
#include "EntityParser.h"
#include "BaseEntity.h"
#include "VBSPInfo.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHL2BSPImporter, Log, All);

class FBSPImporter
{
private:

	FString bspFileName;
	bool bspLoaded;
	Valve::BSPFile bspFile;

	FString mapName;
	UWorld* world;
	AVBSPInfo* vbspInfo;

public:

	FBSPImporter(const FString& fileName);

	/* Loads the BSP file and parses the data. */
	bool Load();

	/* Imports the entire BSP map into the currently opened level. */
	bool ImportToCurrentLevel();

	/* Imports the entire BSP map into the target world. */
	bool ImportAllToWorld(UWorld* targetWorld);

	/* Imports brushes only into the target world. */
	bool ImportBrushesToWorld(UWorld* targetWorld);

	/* Imports geometry only into the target world. */
	bool ImportGeometryToWorld(UWorld* targetWorld);

	/* Imports entities only into the target world. */
	bool ImportEntitiesToWorld(UWorld* targetWorld);

private:

	AActor* ImportBrush(uint16 brushIndex);

	void GatherBrushes(uint32 nodeIndex, TArray<uint16>& out);
	
	void GatherFaces(uint32 nodeIndex, TArray<uint16>& out, TSet<int16>* clusterFilter = nullptr);
	
	void GatherDisplacements(const TArray<uint16>& faceIndices, TArray<uint16>& out);
	
	void GatherClusters(uint32 nodeIndex, TArray<int16>& out);
	
	FPlane ValveToUnrealPlane(const Valve::BSP::cplane_t& plane);
	
	void RenderTreeToActors(TArray<AStaticMeshActor*>& out, uint32 nodeIndex);
	
	UStaticMesh* RenderMeshToStaticMesh(const FMeshDescription& meshDesc, const FString& assetName);

	AStaticMeshActor* RenderMeshToActor(const FMeshDescription& meshDesc, const FString& assetName);
	
	void RenderFacesToMesh(const TArray<uint16>& faceIndices, FMeshDescription& meshDesc, bool skyboxFilter);
	
	void RenderDisplacementsToMesh(const TArray<uint16>& displacements, FMeshDescription& meshDesc);
	
	void RenderTreeToVBSPInfo(uint32 nodeIndex);

	static FString ParseMaterialName(const char* bspMaterialName);
	
	static bool SharesSmoothingGroup(uint16 groupA, uint16 groupB);
	
	ABaseEntity* ImportEntityToWorld(const FHL2EntityData& entityData);

};