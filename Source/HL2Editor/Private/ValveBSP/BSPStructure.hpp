/**
 * @author     ReactiioN
 * @date       08.03.2016
 * @visit      https://github.com/ReactiioN1337
 *             https://reactiion.pw
 */
#pragma once
#include "BSPFlags.hpp"
#include "Matrix.hpp"

namespace Valve {
    using std::array;
    using Vector3 = Matrix< float, 3, 1 >;    
}

namespace Valve { namespace BSP {
    
    enum eLumpIndex : size_t
    {
        LUMP_ENTITIES                       = 0,
        LUMP_PLANES                         = 1,
        LUMP_TEXDATA                        = 2,
        LUMP_VERTEXES                       = 3,
        LUMP_VISIBILITY                     = 4,
        LUMP_NODES                          = 5,
        LUMP_TEXINFO                        = 6,
        LUMP_FACES                          = 7,
        LUMP_LIGHTING                       = 8,
        LUMP_OCCLUSION                      = 9,
        LUMP_LEAFS                          = 10,
        LUMP_EDGES                          = 12,
        LUMP_SURFEDGES                      = 13,
        LUMP_MODELS                         = 14,
        LUMP_WORLDLIGHTS                    = 15,
        LUMP_LEAFFACES                      = 16,
        LUMP_LEAFBRUSHES                    = 17,
        LUMP_BRUSHES                        = 18,
        LUMP_BRUSHSIDES                     = 19,
        LUMP_AREAS                          = 20,
        LUMP_AREAPORTALS                    = 21,
        LUMP_PORTALS                        = 22,
        LUMP_CLUSTERS                       = 23,
        LUMP_PORTALVERTS                    = 24,
        LUMP_CLUSTERPORTALS                 = 25,
        LUMP_DISPINFO                       = 26,
        LUMP_ORIGINALFACES                  = 27,
        LUMP_PHYSCOLLIDE                    = 29,
        LUMP_VERTNORMALS                    = 30,
        LUMP_VERTNORMALINDICES              = 31,
        LUMP_DISP_LIGHTMAP_ALPHAS           = 32,
        LUMP_DISP_VERTS                     = 33,
        LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS = 34,
        LUMP_GAME_LUMP                      = 35,
        LUMP_LEAFWATERDATA                  = 36,
        LUMP_PRIMITIVES                     = 37,
        LUMP_PRIMVERTS                      = 38,
        LUMP_PRIMINDICES                    = 39,
        LUMP_PAKFILE                        = 40,
        LUMP_CLIPPORTALVERTS                = 41,
        LUMP_CUBEMAPS                       = 42,
        LUMP_TEXDATA_STRING_DATA            = 43,
        LUMP_TEXDATA_STRING_TABLE           = 44,
        LUMP_OVERLAYS                       = 45,
        LUMP_LEAFMINDISTTOWATER             = 46,
        LUMP_FACE_MACRO_TEXTURE_INFO        = 47,
        LUMP_DISP_TRIS                      = 48
    };

	enum eGamelumpIndex : int
	{
		GAMELUMP_STATICPROPS = 1936749168, // 'sprp'
		GAMELUMP_DETAILPROPS = 1936749168 // 'dprp'
	};

    class lump_t
    {
    public:
        int32_t          m_Fileofs; /// 0x0
        int32_t          m_Filelen; /// 0x4
        int32_t          m_Version; /// 0x8
        array< char, 4 > m_FourCC;  /// 0xC
    };///Size=0x10

    class dheader_t
    {
    public:
        int32_t                       m_Ident;       /// 0x000
        int32_t                       m_Version;     /// 0x004
        array< lump_t, HEADER_LUMPS > m_Lumps;       /// 0x008
        int32_t                       m_MapRevision; /// 0x408
    };///Size=0x40C

    class dplane_t
    {
    public:
        Vector3 m_Normal;   /// 0x00
        float   m_Distance; /// 0x0C
        int32_t m_Type;     /// 0x10
    };///Size=0x14

    class cplane_t
    {
    public:
        Vector3 m_Normal;     /// 0x00
        float   m_Distance;   /// 0x0C
        uint8_t m_Type;       /// 0x10
        uint8_t m_SignBits;   /// 0x11
    private:
        uint8_t m_Pad[ 0x2 ]; /// 0x12
    };///Size=0x14

    class dedge_t
    {
    public:
        array< uint16_t, 2 > m_V; /// 0x0
    };///Size=0x4

    class mvertex_t
    {
    public:
        Vector3 m_Position; /// 0x0
    };///Size=0xC

    class dleaf_t
    {
    public:
        int32_t             m_Contents;        /// 0x00
        int16_t             m_Cluster;         /// 0x04
        int16_t             m_Area : 9;        /// 0x06
        int16_t             m_Flags : 7;       /// 0x11
        array< int16_t, 3 > m_Mins;            /// 0x1A
        array< int16_t, 3 > m_Maxs;            /// 0x20
        uint16_t            m_Firstleafface;   /// 0x26
        uint16_t            m_Numleaffaces;    /// 0x28
        uint16_t            m_Firstleafbrush;  /// 0x2A
        uint16_t            m_Numleafbrushes;  /// 0x2C
        int16_t             m_LeafWaterDataID; /// 0x2E
    };///Size=0x30

    class dnode_t
    {
    public:
        int32_t             m_Planenum;   /// 0x00
        array< int32_t, 2 >	m_Children;   /// 0x04
        array< int16_t, 3 > m_Mins;       /// 0x0C
        array< int16_t, 3 > m_Maxs;       /// 0x12
        uint16_t            m_Firstface;  /// 0x18
        uint16_t            m_Numfaces;   /// 0x1A
        int16_t             m_Area;       /// 0x1C
    private:
        uint8_t             m_Pad[ 0x2 ]; /// 0x1E
    };///Size=0x20

    class snode_t
    {
    public:
        int32_t             m_PlaneNum;     /// 0x00
        cplane_t*           m_pPlane;       /// 0x04
        array< int32_t, 2 > m_Children;     /// 0x08
        dleaf_t*            m_LeafChildren; /// 0x10
        snode_t*            m_NodeChildren; /// 0x14
        array< int16_t, 3 > m_Mins;         /// 0x18
        array< int16_t, 3 > m_Maxs;         /// 0x1E
        uint16_t            m_Firstface;    /// 0x24
        uint16_t            m_Numfaces;     /// 0x26
        int16_t             m_Area;         /// 0x28
        uint8_t             m_Pad[ 0x2 ];   /// 0x2A
    };///Size=0x2C

    class dface_t
    {
    public:
        uint16_t            m_Planenum;                    /// 0x00
        uint8_t             m_Side;                        /// 0x02
        uint8_t             m_OnNode;                      /// 0x03
        int32_t             m_Firstedge;                   /// 0x04
        int16_t             m_Numedges;                    /// 0x08
        int16_t             m_Texinfo;                     /// 0x0A
        int16_t             m_Dispinfo;                    /// 0x0C
        int16_t             m_SurfaceFogVolumeID;          /// 0x0E
        array< uint8_t, 4 > m_Styles;                      /// 0x10
        int32_t             m_Lightofs;                    /// 0x18
        float               m_Area;                        /// 0x1C
        array< int32_t, 2 > m_LightmapTextureMinsInLuxels; /// 0x20
        array< int32_t, 2 > m_LightmapTextureSizeInLuxels; /// 0x28
        int32_t             m_OrigFace;                    /// 0x30
        uint16_t            m_NumPrims;                    /// 0x34
        uint16_t            m_FirstPrimID;                 /// 0x36
        uint16_t            m_SmoothingGroups;             /// 0x38
    };///Size=0x3A

    class dbrush_t
    {
    public:
        int32_t m_Firstside; /// 0x0
        int32_t m_Numsides;  /// 0x4
        int32_t m_Contents;  /// 0x8
    };///Size=0xC

    class dbrushside_t
    {
    public:
        uint16_t m_Planenum; /// 0x0
        int16_t  m_Texinfo;  /// 0x2
        int16_t  m_Dispinfo; /// 0x4
        uint8_t  m_Bevel;    /// 0x6
        uint8_t  m_Thin;     /// 0x7
    };///Size=0x8

	class dmodel_t
	{
	public:
		Vector3	m_Mins;			/// 0x00
		Vector3 m_Maxs;			/// 0x0C
		Vector3	m_Origin;		/// 0x18
		int32_t	m_Headnode;		/// 0x24
		int32_t	m_Firstface;	/// 0x28
		int32_t m_Numfaces;		/// 0x2C
	};///Size=0x30

    class texinfo_t
    {
    public:
        array< array< float, 4 >, 2 > m_TextureVecs;  /// 0x00
        array< array< float, 4 >, 2 > m_LightmapVecs; /// 0x20
        int32_t                       m_Flags;        /// 0x40
        int32_t                       m_Texdata;      /// 0x44
    };///Size=0x48

	class texdata_t
	{
	public:
		Vector3	m_Reflectivity;				// 0x00
		int	m_NameStringTableID;			// 0x0C
		int	m_Width, m_Height;				// 0x10
		int	m_View_width, m_View_height;	// 0x18
	};///Size=0x2C

	struct CDispSubNeighbor
	{
	public:
		unsigned short		m_iNeighbor;		// This indexes into ddispinfos.
												// 0xFFFF if there is no neighbor here.

		unsigned char		m_NeighborOrientation;		// (CCW) rotation of the neighbor wrt this displacement.

		// These use the NeighborSpan type.
		unsigned char		m_Span;						// Where the neighbor fits onto this side of our displacement.
		unsigned char		m_NeighborSpan;				// Where we fit onto our neighbor.
	};


	// NOTE: see the section above titled "displacement neighbor rules".
	class CDispNeighbor
	{
	public:
		// Note: if there is a neighbor that fills the whole side (CORNER_TO_CORNER),
		//       then it will always be in CDispNeighbor::m_Neighbors[0]
		CDispSubNeighbor	m_SubNeighbors[2];
	};

	class CDispCornerNeighbors
	{
	public:
		unsigned short	m_Neighbors[MAX_DISP_CORNER_NEIGHBORS];	// indices of neighbors.
		unsigned char	m_nNeighbors;
	};

	class ddispinfo_t
	{
	public:
		Vector3			m_StartPosition;		// start position used for orientation
		int			m_DispVertStart;		// Index into LUMP_DISP_VERTS.
		int			m_DispTriStart;		// Index into LUMP_DISP_TRIS.
		int			m_Power;			// power - indicates size of surface (2^power	1)
		int			m_MinTess;		// minimum tesselation allowed
		float			m_SmoothingAngle;		// lighting smoothing angle
		int			m_Contents;		// surface contents
		unsigned short		m_MapFace;		// Which map face this displacement comes from.
		int			m_LightmapAlphaStart;	// Index into ddisplightmapalpha.
		int			m_LightmapSamplePositionStart;	// Index into LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS.
		CDispNeighbor		m_EdgeNeighbors[4];	// Indexed by NEIGHBOREDGE_ defines.
		CDispCornerNeighbors	m_CornerNeighbors[4];	// Indexed by CORNER_ defines.
		unsigned int		m_AllowedVerts[10];	// active verticies
	};

	class ddispvert_t
	{
	public:
		Vector3	m_Vec;	// Vector field defining displacement volume.
		float	m_Dist;	// Displacement distances.
		float	m_Alpha;	// "per vertex" alpha values.
	};

	class ddisptri_t
	{
	public:
		unsigned short m_Tags;	// Displacement triangle tags.
	};

	class dcubemapsample_t
	{
	public:
		int		m_Origin[3];	// position of light snapped to the nearest integer
		int	        m_Size;		// resolution of cubemap, 0 - default
	};

	class dgamelump_t
	{
	public:
		int		m_ID;		// gamelump ID
		unsigned short	m_Flags;		// flags
		unsigned short	m_Version;	// gamelump version
		int		m_Fileofs;	// offset to this gamelump
		int		m_Filelen;	// length
	};

	class StaticPropName_t
	{
	public:
		char	m_Str[128];
	};

	class StaticProp_v4_t
	{
	public:
		Vector3         m_Origin;            // origin
		Vector3         m_Angles;            // orientation (pitch roll yaw)
		unsigned short  m_PropType;          // index into model name dictionary
		unsigned short  m_FirstLeaf;         // index into leaf array
		unsigned short  m_LeafCount;
		unsigned char   m_Solid;             // solidity type
		unsigned char   m_Flags;
		int             m_Skin;              // model skin numbers
		float           m_FadeMinDist;
		float           m_FadeMaxDist;
		Vector3         m_LightingOrigin;    // for lighting
	};

	class StaticProp_v5_t : public StaticProp_v4_t
	{
	public:
		float           m_ForcedFadeScale;   // fade distance scale
	};

	class StaticProp_v6_t : public StaticProp_v5_t
	{
	public:
		unsigned short  m_MinDXLevel;        // minimum DirectX version to be visible
		unsigned short  m_MaxDXLevel;        // maximum DirectX version to be visible
	};

    class VPlane
    {
    public:
        Vector3 m_Origin = 0.f;
        float   m_Distance = 0.f;

    public:
        VPlane( void ) = default;
        VPlane( const Vector3& origin, const float distance ) :
            m_Origin( origin ),
            m_Distance( distance )
        {
        }

        VPlane( const VPlane& other )
        {
            *this = other;
        }

        VPlane( VPlane&& other )
        {
            *this = other;
        }

        VPlane& operator = ( const VPlane& other )
        {
            init( other.m_Origin, other.m_Distance );
            return *this;
        }

        VPlane& operator = ( VPlane&& other )
        {
            init( other.m_Origin, other.m_Distance );
            return *this;
        }

        float dist_to( const Vector3& location ) const
        {
            return m_Origin.dot( location ) - m_Distance;
        }

        void init( const Vector3& origin, const float distance )
        {
            m_Origin   = origin;
            m_Distance = distance;
        }
    };

    class Polygon
    {
    public:
        array< Vector3, MAX_SURFINFO_VERTS > m_Verts;
        size_t                               m_nVerts;
        VPlane                               m_Plane;
        array< VPlane, MAX_SURFINFO_VERTS >  m_EdgePlanes;
        array< Vector3, MAX_SURFINFO_VERTS > m_Vec2D;
        int32_t                              m_Skip;
    };
} }