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

	/* Imports geometry only into the target world. */
	bool ImportGeometryToWorld(UWorld* targetWorld);

	/* Imports entities only into the target world. */
	bool ImportEntitiesToWorld(UWorld* targetWorld);

private:

	void GatherBrushes(uint32 nodeIndex, TArray<uint16>& out);
	
	void GatherFaces(uint32 nodeIndex, TArray<uint16>& out);
	
	void GatherDisplacements(const TArray<uint16>& faceIndices, TArray<uint16>& out);
	
	FPlane ValveToUnrealPlane(const Valve::BSP::cplane_t& plane);
	
	void RenderModelToActors(TArray<AStaticMeshActor*>& out, uint32 modelIndex);
	
	UStaticMesh* RenderMeshToStaticMesh(const FMeshDescription& meshDesc, const FString& assetName, int lightmapResolution);

	AStaticMeshActor* RenderMeshToActor(const FMeshDescription& meshDesc, const FString& assetName, int lightmapResolution);
	
	void RenderFacesToMesh(const TArray<uint16>& faceIndices, FMeshDescription& meshDesc, bool skyboxFilter);

	void RenderBrushesToMesh(const TArray<uint16>& brushIndices, FMeshDescription& meshDesc);
	
	void RenderDisplacementsToMesh(const TArray<uint16>& displacements, FMeshDescription& meshDesc);
	
	void RenderTreeToVBSPInfo(uint32 nodeIndex);

	float FindFaceArea(const Valve::BSP::dface_t& bspFace);

	static FString ParseMaterialName(const char* bspMaterialName);
	
	static bool SharesSmoothingGroup(uint16 groupA, uint16 groupB);
	
	ABaseEntity* ImportEntityToWorld(const FHL2EntityData& entityData);

};