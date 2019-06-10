#pragma once;

#include "CoreMinimal.h"
#include "utils.h"

#pragma pack(1)

namespace Valve
{
	namespace VVD
	{
		constexpr int MAX_NUM_LODS = 8;
		constexpr int MAX_NUM_BONES_PER_VERT = 3;

		// 16 bytes
		struct mstudioboneweight_t
		{
			float	weight[MAX_NUM_BONES_PER_VERT];
			int8	bone[MAX_NUM_BONES_PER_VERT];
			uint8	numbones;
		};

		struct mstudiovertex_t
		{
			mstudioboneweight_t	m_BoneWeights;
			FVector			m_vecPosition;
			FVector			m_vecNormal;
			FVector2D		m_vecTexCoord;
		};

		// apply sequentially to lod sorted vertex and tangent pools to re-establish mesh order
		struct vertexFileFixup_t
		{
			int	lod;			// used to skip culled root lod
			int	sourceVertexID;		// absolute index from start of vertex/tangent blocks
			int	numVertexes;
		};

		struct vertexFileHeader_t
		{
			int	id;				// MODEL_VERTEX_FILE_ID
			int	version;			// MODEL_VERTEX_FILE_VERSION
			long	checksum;			// same as studiohdr_t, ensures sync
			int	numLODs;			// num of valid lods
			int	numLODVertexes[MAX_NUM_LODS];	// num verts for desired root lod
			int	numFixups;			// num of vertexFileFixup_t
			int	fixupTableStart;		// offset from base to fixup table
			void GetFixups(TArray<vertexFileFixup_t>& out) const { ReadArray(this, fixupTableStart, numFixups, out); }

			int	vertexDataStart;		// offset from base to vertex block
			int	tangentDataStart;		// offset from base to tangent block
			void GetVertex(int index, mstudiovertex_t& outVertex, FVector4& outTangent) const
			{
				outVertex = ((mstudiovertex_t*)(((uint8*)this) + vertexDataStart))[index];
				outTangent = ((FVector4*)(((uint8*)this) + tangentDataStart))[index];
			}
		};
	}
}

#pragma pack()