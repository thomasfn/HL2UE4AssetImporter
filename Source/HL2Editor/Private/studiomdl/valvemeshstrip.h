#pragma once;

#include "CoreMinimal.h"

namespace Valve
{
	namespace VTX
	{
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
		};

		struct BodyPartHeader_t
		{
			//Model array
			int numModels;
			int modelOffset;
		};

		// This maps one to one with models in the mdl file.
		struct ModelHeader_t
		{
			//LOD mesh array
			int numLODs;   //This is also specified in FileHeader_t
			int lodOffset;
		};

		struct ModelLODHeader_t
		{
			//Mesh array
			int numMeshes;
			int meshOffset;

			float switchPoint;
		};

		enum class MeshFlags : uint8
		{
			STRIPGROUP_IS_FLEXED			= 0x01,
			STRIPGROUP_IS_HWSKINNED			= 0x02,
			STRIPGROUP_IS_DELTA_FLEXED		= 0x04,
			STRIPGROUP_SUPPRESS_HW_MORPH	= 0x08
		};

		struct MeshHeader_t
		{
			int numStripGroups;
			int stripGroupHeaderOffset;

			uint8 flags;
		};

		struct StripGroupHeader_t
		{
			// These are the arrays of all verts and indices for this mesh.  strips index into this.
			int numVerts;
			int vertOffset;

			int numIndices;
			int indexOffset;

			int numStrips;
			int stripOffset;

			uint8 flags;
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
	}
}