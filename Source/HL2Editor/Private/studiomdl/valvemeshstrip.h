#pragma once;

#include "CoreMinimal.h"
#include "utils.h"

#pragma pack(1)

namespace Valve
{
	namespace VTX
	{
		struct Vertex_t
		{
			// these index into the mesh's vert[origMeshVertID]'s bones
			uint8 boneWeightIndex[3];
			uint8 numBones;

			uint16 origMeshVertID;

			// for sw skinned verts, these are indices into the global list of bones
			// for hw skinned verts, these are hardware bone indices
			int8 boneID[3];
		};

		// A strip is a piece of a stripgroup which is divided by bones 
		struct StripHeader_t
		{
			int numIndices;
			int indexOffset;

			int numVerts;
			int vertOffset;

			int16 numBones;

			uint8 flags;

			int numBoneStateChanges;
			int boneStateChangeOffset;
		};

		struct StripGroupHeader_t
		{
			// These are the arrays of all verts and indices for this mesh.  strips index into this.
			int numVerts;
			int vertOffset;
			void GetVertices(TArray<Vertex_t>& out) const { ReadArray(this, vertOffset, numVerts, out); }

			int numIndices;
			int indexOffset;
			void GetIndices(TArray<uint16>& out) const { ReadArray(this, indexOffset, numIndices, out); }

			int numStrips;
			int stripOffset;
			void GetStrips(TArray<const StripHeader_t*>& out) const { ReadArray<StripHeader_t>(this, stripOffset, numStrips, out); }

			uint8 flags;
		};

		enum class MeshFlags : uint8
		{
			STRIPGROUP_IS_FLEXED = 0x01,
			STRIPGROUP_IS_HWSKINNED = 0x02,
			STRIPGROUP_IS_DELTA_FLEXED = 0x04,
			STRIPGROUP_SUPPRESS_HW_MORPH = 0x08
		};

		struct MeshHeader_t
		{
			int numStripGroups;
			int stripGroupHeaderOffset;
			void GetStripGroups(TArray<const StripGroupHeader_t*>& out) const { ReadArray<StripGroupHeader_t>(this, stripGroupHeaderOffset, numStripGroups, out); }

			uint8 flags;
		};

		struct ModelLODHeader_t
		{
			//Mesh array
			int numMeshes;
			int meshOffset;
			void GetMeshes(TArray<const MeshHeader_t*>& out) const { ReadArray<MeshHeader_t>(this, meshOffset, numMeshes, out); }

			float switchPoint;
		};

		// This maps one to one with models in the mdl file.
		struct ModelHeader_t
		{
			//LOD mesh array
			int numLODs;   //This is also specified in FileHeader_t
			int lodOffset;
			void GetLODs(TArray<const ModelLODHeader_t*>& out) const { ReadArray<ModelLODHeader_t>(this, lodOffset, numLODs, out); }
		};

		struct BodyPartHeader_t
		{
			//Model array
			int numModels;
			int modelOffset;
			void GetModels(TArray<const ModelHeader_t*>& out) const { ReadArray<ModelHeader_t>(this, modelOffset, numModels, out); }
		};

		struct FileHeader_t
		{
			// file version as defined by OPTIMIZED_MODEL_FILE_VERSION (currently 7)
			int version;

			// hardware params that affect how the model is to be optimized.
			int vertCacheSize;
			uint16 maxBonesPerStrip;
			uint16 maxBonesPerTri;
			int maxBonesPerVert;

			// must match checkSum in the .mdl
			int checkSum;

			int numLODs; // Also specified in ModelHeader_t's and should match

			// Offset to materialReplacementList Array. one of these for each LOD, 8 in total
			int materialReplacementListOffset;

			//Defines the size and location of the body part array
			int numBodyParts;
			int bodyPartOffset;
			void GetBodyParts(TArray<const BodyPartHeader_t*>& out) const { ReadArray<BodyPartHeader_t>(this, bodyPartOffset, numBodyParts, out); }
		};
	}
}

#pragma pack()