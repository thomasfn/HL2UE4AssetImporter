#pragma once

#include "CoreMinimal.h"
#include "utils.h"

namespace Valve
{
	namespace PHY
	{
		// this structure can be found in <mod folder>/src/public/phyfile.h
		struct phyheader_t
		{
			int		size;           // Size of this header section (generally 16)
			int		id;             // Often zero, unknown purpose.
			int		solidCount;     // Number of solids in file
			long	        checkSum;	// checksum of source .mdl file (4-bytes)
		};

		// new phy format
		struct compactsurfaceheader_t
		{
			int	size;			// Size of the content after this byte
			int	vphysicsID;		// Generally the ASCII for "VPHY" in newer files
			int16	version;
			int16	modelType;
			int	surfaceSize;
			FVector3f	dragAxisAreas;
			int	axisMapSize;
			int dummy[12]; // dummy[11] is "IVPS"
		};

		// old style phy format
		struct legacysurfaceheader_t
		{
			int	size;
			float	mass_center[3];
			float	rotation_inertia[3];
			float	upper_limit_radius;
			int	max_deviation : 8;
			int	byte_size : 24;
			int	offset_ledgetree_root;
			int	dummy[3]; 		// dummy[2] is "IVPS" or 0
		};

		struct trianglevertex_t
		{
			uint16 index;
			uint16 unknown0;
		};

		struct triangle_t
		{
			uint8 index;
			uint8 unknown0;
			uint16 unknown1;
			trianglevertex_t vertices[3];
		};

		struct sectionheader_t
		{
			int vertexDataOffset;
			int boneIndex;
			int unknown0;
			int triangleCount;

			uint16 GetTriangles(TArray<triangle_t>& out) const
			{
				ReadArray(this, sizeof(sectionheader_t), triangleCount, out);
				return sizeof(triangle_t) * triangleCount;
			}
		};
	}
}