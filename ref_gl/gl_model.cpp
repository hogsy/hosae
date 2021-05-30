/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// models.c -- model loading and caching

#include "qcommon/qcommon.h"
#include "gl_local.h"

byte mod_novis[ MAX_MAP_LEAFS / 8 ];

static std::vector< hosae::Model > cachedModels;

// the inline * models from the current map are kept seperate
//hosae::Model mod_inline[ MAX_MOD_KNOWN ];

//int registration_sequence;

mleaf_t *hosae::BSPModel::PointInLeaf( const vec3_t p )
{
	mnode_t *node;
	float d;
	cplane_t *plane;

	if( nodes == nullptr )
		VID_Error( ERR_DROP, "Mod_PointInLeaf: bad model" );

	node = nodes;
	while( 1 ) {
		if( node->contents != -1 ) return (mleaf_t *)node;
		plane = node->plane;
		d = DotProduct( p, plane->normal ) - plane->dist;
		if( d > 0 )
			node = node->children[ 0 ];
		else
			node = node->children[ 1 ];
	}

	return nullptr;  // never reached
}

byte *hosae::BSPModel::DecompressVis( byte *in )
{
	static byte decompressed[ MAX_MAP_LEAFS / 8 ];
	int c;
	byte *out;
	int row;

	row = ( vis->numclusters + 7 ) >> 3;
	out = decompressed;

	if( !in ) {  // no vis info, so make all visible
		while( row ) {
			*out++ = 0xff;
			row--;
		}
		return decompressed;
	}

	do {
		if( *in ) {
			*out++ = *in++;
			continue;
		}

		c = in[ 1 ];
		in += 2;
		while( c ) {
			*out++ = 0;
			c--;
		}
	} while( out - decompressed < row );

	return decompressed;
}

byte *hosae::BSPModel::ClusterPVS( int cluster )
{
	if( cluster == -1 || vis == nullptr ) return mod_novis;
	return DecompressVis( (byte *)vis + vis->bitofs[ cluster ][ DVIS_PVS ] );
}

//===============================================================================

void Mod_Modellist_f( void ) {
	int i;
	hosae::Model *mod;
	int total;

	total = 0;
	VID_Printf( PRINT_ALL, "Loaded models:\n" );
	for ( const auto &mod : cachedModels )
	{
		VID_Printf( PRINT_ALL, "%8i : %s\n", mod.GetName() );
	}

	VID_Printf( PRINT_ALL, "Total resident: %i\n", cachedModels.size() );
}

void Mod_Init( void ) { memset( mod_novis, 0xff, sizeof( mod_novis ) ); }

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
hosae::Model *Mod_ForName( const char *name, qboolean crash ) {
	hosae::Model *mod;
	unsigned *buf;
	int i;

	if( !name[ 0 ] ) VID_Error( ERR_DROP, "Mod_ForName: NULL name" );

	//
	// inline models are grabbed only from worldmodel
	//
	if( name[ 0 ] == '*' ) {
		i = atoi( name + 1 );
		if( i < 1 || !r_worldmodel || i >= r_worldmodel->numsubmodels )
			VID_Error( ERR_DROP, "bad inline model number" );
		return &mod_inline[ i ];
	}

	//
	// search the currently loaded models
	//
	for ( auto &i : cachedModels )
	{
		if ( strcmp( i.GetName(), name ) != 0 )
			continue;

		return &i;
	}

	//
	// find a free model slot spot
	//

	for( i = 0, mod = mod_known; i < mod_numknown; i++, mod++ ) {
		if( !mod->name[ 0 ] ) break;  // free spot
	}

	if( i == mod_numknown ) {
		if( mod_numknown == MAX_MOD_KNOWN )
			VID_Error( ERR_DROP, "mod_numknown == MAX_MOD_KNOWN" );
		mod_numknown++;
	}

	strcpy( mod->name, name );

	//
	// load the file
	//
	modfilelen = FS_LoadFile( mod->name, (void **)&buf );
	if( !buf ) {
		if( crash )
			VID_Error( ERR_DROP, "Mod_NumForName: %s not found", mod->name );
		memset( mod->name, 0, sizeof( mod->name ) );
		return NULL;
	}

	loadmodel = mod;

	//
	// fill it in
	//

	// call the apropriate loader

	void Mod_LoadAliasModel( hosae::Model *mod, void *buf );
	void Mod_LoadSpriteModel( hosae::Model * mod, void *buf );
	void Mod_LoadBrushModel( hosae::Model * mod, void *buf );

	switch( LittleLong( *(unsigned *)buf ) ) {
	case IDALIASHEADER:
		loadmodel->extradata = Hunk_Begin( 0x500000 );
		Mod_LoadAliasModel( mod, buf );
		break;

	case IDSPRITEHEADER:
		loadmodel->extradata = Hunk_Begin( 0x10000 );
		Mod_LoadSpriteModel( mod, buf );
		break;

	case IDBSPHEADER:
		loadmodel->extradata = Hunk_Begin( 0x1000000 );
		Mod_LoadBrushModel( mod, buf );
		break;

	default:
#if defined( _DEBUG )
		VID_Printf( PRINT_ALL, "Mod_NumForName: unknown fileid for %s", mod->name );
#else
		VID_Error( ERR_DROP, "Mod_NumForName: unknown fileid for %s", mod->name );
#endif
		break;
	}

	loadmodel->extradatasize = Hunk_End();

	FS_FreeFile( buf );

	return mod;
}

/*
===============================================================================

										BRUSHMODEL LOADING

===============================================================================
*/

byte *mod_base;

void hosae::BSPModel::LoadLighting( const lump_t *l ) {
	if( !l->filelen ) {
		lightdata = NULL;
		return;
	}
	lightdata = static_cast<byte *>( Hunk_Alloc( l->filelen ) );
	memcpy( lightdata, mod_base + l->fileofs, l->filelen );
}

void hosae::BSPModel::LoadVisibility( const lump_t *l ) {
	int i;

	if( !l->filelen ) {
		vis = NULL;
		return;
	}
	vis = static_cast<dvis_t *>( Hunk_Alloc( l->filelen ) );
	memcpy( vis, mod_base + l->fileofs, l->filelen );

	vis->numclusters = LittleLong( vis->numclusters );
	for( i = 0; i < vis->numclusters; i++ ) {
		vis->bitofs[ i ][ 0 ] = LittleLong( vis->bitofs[ i ][ 0 ] );
		vis->bitofs[ i ][ 1 ] = LittleLong( vis->bitofs[ i ][ 1 ] );
	}
}

void hosae::BSPModel::LoadVertices( const lump_t *l )
{
	dvertex_t *in;
	mvertex_t *out;
	int i, count;

	in = (dvertex_t *)( mod_base + l->fileofs );
	if( l->filelen % sizeof( *in ) )
		VID_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", GetName() );

	count = l->filelen / sizeof( *in );
	out = static_cast<mvertex_t *>( Hunk_Alloc( count * sizeof( *out ) ) );

	vertexes = out;
	numvertexes = count;

	for( i = 0; i < count; i++, in++, out++ ) {
		out->position[ 0 ] = LittleFloat( in->point[ 0 ] );
		out->position[ 1 ] = LittleFloat( in->point[ 1 ] );
		out->position[ 2 ] = LittleFloat( in->point[ 2 ] );
	}
}

float RadiusFromBounds( vec3_t mins, vec3_t maxs ) {
	int i;
	vec3_t corner;

	for( i = 0; i < 3; i++ ) {
		corner[ i ] = fabs( mins[ i ] ) > fabs( maxs[ i ] ) ? fabs( mins[ i ] ) : fabs( maxs[ i ] );
	}

	return VectorLength( corner );
}

void hosae::BSPModel::LoadSubModels( const lump_t *l ) {
	dmodel_t *in;
	mmodel_t *out;
	int i, j, count;

	in = (dmodel_t *)( mod_base + l->fileofs );
	if( l->filelen % sizeof( *in ) )
		VID_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", GetName() );
	count = l->filelen / sizeof( *in );
	out = static_cast<mmodel_t *>( Hunk_Alloc( count * sizeof( *out ) ) );

	submodels = out;
	numsubmodels = count;

	for( i = 0; i < count; i++, in++, out++ ) {
		for( j = 0; j < 3; j++ ) {  // spread the mins / maxs by a pixel
			out->mins[ j ] = LittleFloat( in->mins[ j ] ) - 1;
			out->maxs[ j ] = LittleFloat( in->maxs[ j ] ) + 1;
			out->origin[ j ] = LittleFloat( in->origin[ j ] );
		}
		out->radius = RadiusFromBounds( out->mins, out->maxs );
		out->headnode = LittleLong( in->headnode );
		out->firstface = LittleLong( in->firstface );
		out->numfaces = LittleLong( in->numfaces );
	}
}

/*
=================
Mod_LoadEdges
=================
*/
void hosae::BSPModel::LoadEdges( const lump_t *l ) {
	dedge_t *in;
	medge_t *out;
	int i, count;

	in = (dedge_t *)( mod_base + l->fileofs );
	if( l->filelen % sizeof( *in ) )
		VID_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", GetName() );
	count = l->filelen / sizeof( *in );
	out = static_cast<medge_t *>( Hunk_Alloc( ( count + 1 ) * sizeof( *out ) ) );

	edges = out;
	numedges = count;

	for( i = 0; i < count; i++, in++, out++ ) {
		out->v[ 0 ] = (unsigned short)LittleShort( in->v[ 0 ] );
		out->v[ 1 ] = (unsigned short)LittleShort( in->v[ 1 ] );
	}
}

/*
=================
Mod_LoadTexinfo
=================
*/
void hosae::BSPModel::LoadTextureInfo( const lump_t *l )
{
	texinfo_t *in;
	mtexinfo_t *out, *step;
	int i, j, count;
	char name[ MAX_QPATH ];
	int next;

	in = (texinfo_t *)( mod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
		VID_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", GetName() );
	count = l->filelen / sizeof( *in );
	out = static_cast<mtexinfo_t *>( Hunk_Alloc( count * sizeof( *out ) ) );

	texinfo = out;
	numtexinfo = count;

	for( i = 0; i < count; i++, in++, out++ ) {
		for( j = 0; j < 8; j++ ) out->vecs[ 0 ][ j ] = LittleFloat( in->vecs[ 0 ][ j ] );

		out->flags = LittleLong( in->flags );
		next = LittleLong( in->nexttexinfo );
		if( next > 0 )
			out->next = texinfo + next;
		else
			out->next = NULL;

		Com_sprintf( name, sizeof( name ), "textures/%s.tga", in->texture );
		out->image = GL_FindImage( name, it_wall );
		if( !out->image ) {
			VID_Printf( PRINT_ALL, "Couldn't load %s\n", name );
			out->image = r_notexture;
		}
	}

	// count animation frames
	for( i = 0; i < count; i++ ) {
		out = &texinfo[ i ];
		out->numframes = 1;
		for( step = out->next; step && step != out; step = step->next )
			out->numframes++;
	}
}

/*
Fills in s->texturemins[] and s->extents[]
*/
void hosae::BSPModel::CalculateSurfaceExtents( msurface_t *s ) {
	float mins[ 2 ], maxs[ 2 ], val;
	int i, j, e;
	mvertex_t *v;
	mtexinfo_t *tex;
	int bmins[ 2 ], bmaxs[ 2 ];

	mins[ 0 ] = mins[ 1 ] = 999999;
	maxs[ 0 ] = maxs[ 1 ] = -99999;

	tex = s->texinfo;

	for( i = 0; i < s->numedges; i++ ) {
		e = surfedges[ s->firstedge + i ];
		if( e >= 0 )
			v = &vertexes[ edges[ e ].v[ 0 ] ];
		else
			v = &vertexes[ edges[ -e ].v[ 1 ] ];

		for( j = 0; j < 2; j++ ) {
			val = v->position[ 0 ] * tex->vecs[ j ][ 0 ] +
				v->position[ 1 ] * tex->vecs[ j ][ 1 ] +
				v->position[ 2 ] * tex->vecs[ j ][ 2 ] + tex->vecs[ j ][ 3 ];
			if( val < mins[ j ] ) mins[ j ] = val;
			if( val > maxs[ j ] ) maxs[ j ] = val;
		}
	}

	for( i = 0; i < 2; i++ ) {
		bmins[ i ] = floor( mins[ i ] / 16 );
		bmaxs[ i ] = ceil( maxs[ i ] / 16 );

		s->texturemins[ i ] = bmins[ i ] * 16;
		s->extents[ i ] = ( bmaxs[ i ] - bmins[ i ] ) * 16;

		//		if ( !(tex->flags & TEX_SPECIAL) && s->extents[i] > 512 /* 256
		//*/ ) 			VID_Error (ERR_DROP, "Bad surface extents");
	}
}

void GL_BuildPolygonFromSurface( msurface_t *fa );
void GL_CreateSurfaceLightmap( msurface_t *surf );
void GL_EndBuildingLightmaps( void );
void GL_BeginBuildingLightmaps( hosae::BSPModel *m );

void hosae::BSPModel::LoadFaces( const lump_t *l ) {
	dface_t *in;
	msurface_t *out;
	int i, count, surfnum;
	int planenum, side;
	int ti;

	in = (dface_t *)( mod_base + l->fileofs );
	if( l->filelen % sizeof( *in ) )
		VID_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",
			GetName() );
	count = l->filelen / sizeof( *in );
	out = static_cast<msurface_t *>( Hunk_Alloc( count * sizeof( *out ) ) );

	surfaces = out;
	numsurfaces = count;

	GL_BeginBuildingLightmaps( this );

	for( surfnum = 0; surfnum < count; surfnum++, in++, out++ ) {
		out->firstedge = LittleLong( in->firstedge );
		out->numedges = LittleShort( in->numedges );
		out->flags = 0;
		out->polys = NULL;

		planenum = LittleShort( in->planenum );
		side = LittleShort( in->side );
		if( side ) out->flags |= SURF_PLANEBACK;

		out->plane = planes + planenum;

		ti = LittleShort( in->texinfo );
		if( ti < 0 || ti >= numtexinfo )
			VID_Error( ERR_DROP, "MOD_LoadBmodel: bad texinfo number" );
		out->texinfo = texinfo + ti;

		CalculateSurfaceExtents( out );

		// lighting info

		for( i = 0; i < MAXLIGHTMAPS; i++ ) out->styles[ i ] = in->styles[ i ];
		i = LittleLong( in->lightofs );
		if( i == -1 )
			out->samples = NULL;
		else
			out->samples = lightdata + i;

		// set the drawing flags

		if( out->texinfo->flags & SURF_WARP ) {
			out->flags |= SURF_DRAWTURB;
			for( i = 0; i < 2; i++ ) {
				out->extents[ i ] = 16384;
				out->texturemins[ i ] = -8192;
			}
			GL_SubdivideSurface( out );  // cut up polygon for warps
		}

		// create lightmaps and polygons
		if( !( out->texinfo->flags &
			( SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP ) ) )
			GL_CreateSurfaceLightmap( out );

		if( !( out->texinfo->flags & SURF_WARP ) ) GL_BuildPolygonFromSurface( out );
	}

	GL_EndBuildingLightmaps();
}

void Mod_SetParent( mnode_t *node, mnode_t *parent ) {
	node->parent = parent;
	if( node->contents != -1 ) return;
	Mod_SetParent( node->children[ 0 ], node );
	Mod_SetParent( node->children[ 1 ], node );
}

/*
=================
Mod_LoadNodes
=================
*/
void hosae::BSPModel::LoadNodes( const lump_t *l ) {
	int i, j, count, p;
	dnode_t *in;
	mnode_t *out;

	in = (dnode_t *)( mod_base + l->fileofs );
	if( l->filelen % sizeof( *in ) )
		VID_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", GetName() );
	count = l->filelen / sizeof( *in );
	out = static_cast<mnode_t *>( Hunk_Alloc( count * sizeof( *out ) ) );

	nodes = out;
	numnodes = count;

	for( i = 0; i < count; i++, in++, out++ ) {
		for( j = 0; j < 3; j++ ) {
			out->minmaxs[ j ] = LittleShort( in->mins[ j ] );
			out->minmaxs[ 3 + j ] = LittleShort( in->maxs[ j ] );
		}

		p = LittleLong( in->planenum );
		out->plane = planes + p;

		out->firstsurface = LittleShort( in->firstface );
		out->numsurfaces = LittleShort( in->numfaces );
		out->contents = -1;  // differentiate from leafs

		for( j = 0; j < 2; j++ ) {
			p = LittleLong( in->children[ j ] );
			if( p >= 0 )
				out->children[ j ] = nodes + p;
			else
				out->children[ j ] = (mnode_t *)( leafs + ( -1 - p ) );
		}
	}

	Mod_SetParent( nodes, NULL );  // sets nodes and leafs
}

void hosae::BSPModel::LoadLeafs( const lump_t *l ) {
	dleaf_t *in;
	mleaf_t *out;
	int i, j, count, p;
	//	glpoly_t	*poly;

	in = (dleaf_t *)( mod_base + l->fileofs );
	if( l->filelen % sizeof( *in ) )
		VID_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", GetName() );
	count = l->filelen / sizeof( *in );
	out = static_cast<mleaf_t *>( Hunk_Alloc( count * sizeof( *out ) ) );

	leafs = out;
	numleafs = count;

	for( i = 0; i < count; i++, in++, out++ ) {
		for( j = 0; j < 3; j++ ) {
			out->minmaxs[ j ] = LittleShort( in->mins[ j ] );
			out->minmaxs[ 3 + j ] = LittleShort( in->maxs[ j ] );
		}

		p = LittleLong( in->contents );
		out->contents = p;

		out->cluster = LittleShort( in->cluster );
		out->area = LittleShort( in->area );

		out->firstmarksurface =
			marksurfaces + LittleShort( in->firstleafface );
		out->nummarksurfaces = LittleShort( in->numleaffaces );

		// gl underwater warp
#if 0
		if( out->contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA | CONTENTS_THINWATER ) ) {
			for( j = 0; j < out->nummarksurfaces; j++ ) {
				out->firstmarksurface[ j ]->flags |= SURF_UNDERWATER;
				for( poly = out->firstmarksurface[ j ]->polys; poly; poly = poly->next )
					poly->flags |= SURF_UNDERWATER;
			}
		}
#endif
	}
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
void hosae::BSPModel::LoadMarkSurfaces( const lump_t *l ) {
	int i, j, count;
	short *in;
	msurface_t **out;

	in = (short *)( mod_base + l->fileofs );
	if( l->filelen % sizeof( *in ) )
		VID_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", GetName() );
	count = l->filelen / sizeof( *in );
	out = static_cast<msurface_t **>( Hunk_Alloc( count * sizeof( *out ) ) );

	marksurfaces = out;
	nummarksurfaces = count;

	for( i = 0; i < count; i++ ) {
		j = LittleShort( in[ i ] );
		if( j < 0 || j >= numsurfaces )
			VID_Error( ERR_DROP, "Mod_ParseMarksurfaces: bad surface number" );
		out[ i ] = surfaces + j;
	}
}

/*
=================
Mod_LoadSurfedges
=================
*/
void hosae::BSPModel::LoadSurfaceEdges( const lump_t *l ) {
	int i, count;
	int *in, *out;

	in = (int *)( mod_base + l->fileofs );
	if( l->filelen % sizeof( *in ) )
		VID_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", GetName() );
	count = l->filelen / sizeof( *in );
	if( count < 1 || count >= MAX_MAP_SURFEDGES )
		VID_Error( ERR_DROP, "MOD_LoadBmodel: bad surfedges count in %s: %i", GetName(), count );

	out = static_cast<int *>( Hunk_Alloc( count * sizeof( *out ) ) );

	surfedges = out;
	numsurfedges = count;

	for( i = 0; i < count; i++ ) out[ i ] = LittleLong( in[ i ] );
}

void hosae::BSPModel::LoadPlanes( const lump_t *l ) {
	int i, j;
	cplane_t *out;
	dplane_t *in;
	int count;
	int bits;

	in = (dplane_t *)( mod_base + l->fileofs );
	if( l->filelen % sizeof( *in ) )
		VID_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", GetName() );
	count = l->filelen / sizeof( *in );
	out = static_cast<cplane_t *>( Hunk_Alloc( count * 2 * sizeof( *out ) ) );

	planes = out;
	numplanes = count;

	for( i = 0; i < count; i++, in++, out++ ) {
		bits = 0;
		for( j = 0; j < 3; j++ ) {
			out->normal[ j ] = LittleFloat( in->normal[ j ] );
			if( out->normal[ j ] < 0 ) bits |= 1 << j;
		}

		out->dist = LittleFloat( in->dist );
		out->type = LittleLong( in->type );
		out->signbits = bits;
	}
}

/*
=================
Mod_LoadBrushModel
=================
*/
void hosae::BSPModel::LoadBrushModels( model_t *mod, void *buffer ) {
	dheader_t *header;
	mmodel_t *bm;

	loadmodel->type = mod_brush;
	if( loadmodel != mod_known )
		VID_Error( ERR_DROP, "Loaded a brush model after the world" );

	header = (dheader_t *)buffer;

	unsigned int version = LittleLong( header->version );
	if( version != BSPVERSION )
		VID_Error(
			ERR_DROP,
			"Mod_LoadBrushModel: %s has wrong version number (%i should be %i)",
			mod->name, version

		        , BSPVERSION );

	// swap all the lumps
	mod_base = (byte *)header;

	for( unsigned int i = 0; i < sizeof( dheader_t ) / 4; i++ )
		( (int *)header )[ i ] = LittleLong( ( (int *)header )[ i ] );

	// load into heap

	Mod_LoadVertexes( &header->lumps[ LUMP_VERTEXES ] );
	Mod_LoadEdges( &header->lumps[ LUMP_EDGES ] );
	Mod_LoadSurfedges( &header->lumps[ LUMP_SURFEDGES ] );
	Mod_LoadLighting( &header->lumps[ LUMP_LIGHTING ] );
	Mod_LoadPlanes( &header->lumps[ LUMP_PLANES ] );
	Mod_LoadTexinfo( &header->lumps[ LUMP_TEXINFO ] );
	Mod_LoadFaces( &header->lumps[ LUMP_FACES ] );
	Mod_LoadMarksurfaces( &header->lumps[ LUMP_LEAFFACES ] );
	Mod_LoadVisibility( &header->lumps[ LUMP_VISIBILITY ] );
	Mod_LoadLeafs( &header->lumps[ LUMP_LEAFS ] );
	Mod_LoadNodes( &header->lumps[ LUMP_NODES ] );
	Mod_LoadSubmodels( &header->lumps[ LUMP_MODELS ] );
	mod->numframes = 2;  // regular and alternate animation

	//
	// set up the submodels
	//
	for( int i = 0; i < mod->numsubmodels; i++ ) {
		model_t *starmod;

		bm = &mod->submodels[ i ];
		starmod = &mod_inline[ i ];

		*starmod = *loadmodel;

		starmod->firstmodelsurface = bm->firstface;
		starmod->nummodelsurfaces = bm->numfaces;
		starmod->firstnode = bm->headnode;
		if( starmod->firstnode >= loadmodel->numnodes )
			VID_Error( ERR_DROP, "Inline model %i has bad firstnode", i );

		VectorCopy( bm->maxs, starmod->maxs );
		VectorCopy( bm->mins, starmod->mins );
		starmod->radius = bm->radius;

		if( i == 0 ) *loadmodel = *starmod;

		starmod->numleafs = bm->visleafs;
	}
}

/*
==============================================================================

ALIAS MODELS

==============================================================================
*/

/*
=================
Mod_LoadAliasModel
=================
*/
void Mod_LoadAliasModel( model_t *mod, void *buffer ) {
	dstvert_t *pinst, *poutst;

	int *pincmd, *poutcmd;
	int version;

	dmdl_t *pinmodel = (dmdl_t *)buffer;

	version = LittleLong( pinmodel->version );
	if( version != ALIAS_VERSION ) {
#ifndef _DEBUG
		VID_Error( ERR_DROP, "%s has wrong version number (%i should be %i)",
			mod->name, version, ALIAS_VERSION );
#else
		Com_Printf( "%s has wrong version number (%i should be %i)\n", mod->name,
			version, ALIAS_VERSION );
#endif
	}

	dmdl_t *pheader = static_cast<dmdl_t *>( Hunk_Alloc( LittleLong( pinmodel->ofs_end ) ) );
	// byte swap the header fields and sanity check
	for( unsigned int i = 0; i < sizeof( dmdl_t ) / 4; i++ )
		( (int *)pheader )[ i ] = LittleLong( ( (int *)buffer )[ i ] );

	if( pheader->skinheight > MAX_LBM_HEIGHT )
		VID_Error( ERR_DROP, "model %s has a skin taller than %d", mod->name,
			MAX_LBM_HEIGHT );

	if( pheader->num_xyz <= 0 )
		VID_Error( ERR_DROP, "model %s has no vertices", mod->name );

	if( pheader->num_st <= 0 )
		VID_Error( ERR_DROP, "model %s has no st vertices", mod->name );

	if( pheader->num_tris <= 0 )
		VID_Error( ERR_DROP, "model %s has no triangles", mod->name );

	if( pheader->num_frames <= 0 )
		VID_Error( ERR_DROP, "model %s has no frames", mod->name );

	if( pheader->resolution < 0 || pheader->resolution > 2 ) {
		VID_Error( ERR_DROP, "model %s has invalid resolution", mod->name );
	}

	//
	// load base s and t vertices (not used in gl version)
	//
	pinst = (dstvert_t *)( (byte *)pinmodel + pheader->ofs_st );
	poutst = (dstvert_t *)( (byte *)pheader + pheader->ofs_st );
	for( int i = 0; i < pheader->num_st; i++ ) {
		poutst[ i ].s = LittleShort( pinst[ i ].s );
		poutst[ i ].t = LittleShort( pinst[ i ].t );
	}

	//
	// load triangle lists
	//
	dtriangle_t *pintri = (dtriangle_t *)( (byte *)pinmodel + pheader->ofs_tris );
	dtriangle_t *pouttri = (dtriangle_t *)( (byte *)pheader + pheader->ofs_tris );
	for( int i = 0; i < pheader->num_tris; i++ ) {
		for( unsigned int j = 0; j < 3; j++ ) {
			pouttri[ i ].index_xyz[ j ] = LittleShort( pintri[ i ].index_xyz[ j ] );
			pouttri[ i ].index_st[ j ] = LittleShort( pintri[ i ].index_st[ j ] );
		}
	}

	//
	// load the frames
	//
	for( int i = 0; i < pheader->num_frames; i++ ) {
		switch( pheader->resolution ) {
		case 0:
		{
			Md2FrameHeader *pinframe = (Md2FrameHeader *)( (byte *)pinmodel + pheader->ofs_frames + i * pheader->framesize );
			Md2FrameHeader *poutframe = (Md2FrameHeader *)( (byte *)pheader + pheader->ofs_frames + i * pheader->framesize );
			memcpy( poutframe->name, pinframe->name, sizeof( poutframe->name ) );

			for( unsigned int j = 0; j < 3; j++ ) {
				poutframe->scale[ j ] = LittleFloat( pinframe->scale[ j ] );
				poutframe->translate[ j ] = LittleFloat( pinframe->translate[ j ] );
			}

			for( int j = 0; j < pheader->num_xyz; ++j ) {
				for( unsigned int k = 0; k < 3; ++k ) {
					poutframe->verts[ j ].vertexIndices[ k ] = pinframe->verts[ j ].vertexIndices[ k ];
				}
				poutframe->verts[ j ].normalIndex = LittleShort( pinframe->verts->normalIndex );
			}
			break;
		}
		case 1:
		{
			Md2FrameHeader4 *pinframe = (Md2FrameHeader4 *)( (byte *)pinmodel + pheader->ofs_frames + i * pheader->framesize );
			Md2FrameHeader4 *poutframe = (Md2FrameHeader4 *)( (byte *)pheader + pheader->ofs_frames + i * pheader->framesize );
			memcpy( poutframe->name, pinframe->name, sizeof( poutframe->name ) );

			for( unsigned int j = 0; j < 3; j++ ) {
				poutframe->scale[ j ] = LittleFloat( pinframe->scale[ j ] );
				poutframe->translate[ j ] = LittleFloat( pinframe->translate[ j ] );
			}

			for( int j = 0; j < pheader->num_xyz; ++j ) {
				poutframe->verts[ j ].vertexIndices = pinframe->verts[ j ].vertexIndices;
				poutframe->verts[ j ].normalIndex = LittleShort( pinframe->verts->normalIndex );
			}
			break;
		}
		case 2:
		{
			Md2FrameHeader6 *pinframe = (Md2FrameHeader6 *)( (byte *)pinmodel + pheader->ofs_frames + i * pheader->framesize );
			Md2FrameHeader6 *poutframe = (Md2FrameHeader6 *)( (byte *)pheader + pheader->ofs_frames + i * pheader->framesize );
			memcpy( poutframe->name, pinframe->name, sizeof( poutframe->name ) );

			for( unsigned int j = 0; j < 3; j++ ) {
				poutframe->scale[ j ] = LittleFloat( pinframe->scale[ j ] );
				poutframe->translate[ j ] = LittleFloat( pinframe->translate[ j ] );
			}

			for( int j = 0; j < pheader->num_xyz; ++j ) {
				for( unsigned int k = 0; k < 3; ++k ) {
					poutframe->verts[ j ].vertexIndices[ k ] = pinframe->verts[ j ].vertexIndices[ k ];
				}
				poutframe->verts[ j ].normalIndex = LittleShort( pinframe->verts->normalIndex );
			}
			break;
		}
		default:
			VID_Error( ERR_DROP, "Unhandled frame resolution %d in %s!\n", pheader->resolution, mod->name );
		}
	}

	mod->type = mod_alias;

	//
	// load the glcmds
	//
	pincmd = (int *)( (byte *)pinmodel + pheader->ofs_glcmds );
	poutcmd = (int *)( (byte *)pheader + pheader->ofs_glcmds );
	for( int i = 0; i < pheader->num_glcmds; i++ ) poutcmd[ i ] = LittleLong( pincmd[ i ] );

	// register all skins
	memcpy( (char *)pheader + pheader->ofs_skins,
		(char *)pinmodel + pheader->ofs_skins,
		pheader->num_skins * MAX_SKINNAME );
	for( int i = 0; i < pheader->num_skins; i++ ) {
		char skin_path[ MAX_QPATH ];
		snprintf( skin_path, sizeof( skin_path ), "%s", mod->name );
		strcpy( strrchr( skin_path, '/' ) + 1, (char *)pheader + pheader->ofs_skins + i * MAX_SKINNAME );
		mod->skins[ i ] = GL_FindImage( skin_path, it_skin );

		// PGM
		mod->numframes = pheader->num_frames;
		// PGM
	}

	mod->mins[ 0 ] = -32;
	mod->mins[ 1 ] = -32;
	mod->mins[ 2 ] = -32;
	mod->maxs[ 0 ] = 32;
	mod->maxs[ 1 ] = 32;
	mod->maxs[ 2 ] = 32;

	/* Okay, now that that's all out of the way, we need to do the fun new bits... */

//Md2MultipleSurfaceHeader *pInSurfaces = ( int * )
}

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/

/*
=================
Mod_LoadSpriteModel
=================
*/
void Mod_LoadSpriteModel( model_t *mod, void *buffer ) {
	dsprite_t *sprin, *sprout;
	int i;

	sprin = (dsprite_t *)buffer;
	sprout = static_cast<dsprite_t *>( Hunk_Alloc( modfilelen ) );

	sprout->ident = LittleLong( sprin->ident );
	sprout->version = LittleLong( sprin->version );
	sprout->numframes = LittleLong( sprin->numframes );

	if( sprout->version != SPRITE_VERSION )
		VID_Error( ERR_DROP, "%s has wrong version number (%i should be %i)",
			mod->name, sprout->version, SPRITE_VERSION );

	if( sprout->numframes > MAX_MD2SKINS )
		VID_Error( ERR_DROP, "%s has too many frames (%i > %i)", mod->name,
			sprout->numframes, MAX_MD2SKINS );

	// byte swap everything
	for( i = 0; i < sprout->numframes; i++ ) {
		sprout->frames[ i ].width = LittleLong( sprin->frames[ i ].width );
		sprout->frames[ i ].height = LittleLong( sprin->frames[ i ].height );
		sprout->frames[ i ].origin_x = LittleLong( sprin->frames[ i ].origin_x );
		sprout->frames[ i ].origin_y = LittleLong( sprin->frames[ i ].origin_y );
		memcpy( sprout->frames[ i ].name, sprin->frames[ i ].name, MAX_SKINNAME );
		mod->skins[ i ] = GL_FindImage( sprout->frames[ i ].name, it_sprite );
	}

	mod->type = mod_sprite;
}

//=============================================================================

/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginRegistration

Specifies the model that will be used as the world
@@@@@@@@@@@@@@@@@@@@@
*/
void Mod_BeginRegistration( const char *model ) {
	char fullname[ MAX_QPATH ];
	cvar_t *flushmap;

	registration_sequence++;
	r_oldviewcluster = -1;  // force markleafs

	Com_sprintf( fullname, sizeof( fullname ), "maps/%s.bsp", model );

	// explicitly free the old map if different
	// this guarantees that mod_known[0] is the world map
	flushmap = Cvar_Get( "flushmap", "0", 0 );
	if( strcmp( mod_known[ 0 ].name, fullname ) || flushmap->value )
		Mod_Free( &mod_known[ 0 ] );
	r_worldmodel = Mod_ForName( fullname, true );

	r_viewcluster = -1;
}

struct model_s *Mod_RegisterModel( const char *name )
{
	model_t *mod = Mod_ForName( name, false );
	if( mod ) {
		mod->registration_sequence = registration_sequence;

		// register any images used by the models
		if( mod->type == mod_sprite ) {
			dsprite_t *sprout = (dsprite_t *)mod->extradata;
			for( int i = 0; i < sprout->numframes; i++ )
				mod->skins[ i ] = GL_FindImage( sprout->frames[ i ].name, it_sprite );
		} else if( mod->type == mod_brush ) {
			for( int i = 0; i < mod->numtexinfo; i++ )
				mod->texinfo[ i ].image->registration_sequence = registration_sequence;
		}
	}
	return mod;
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_EndRegistration

@@@@@@@@@@@@@@@@@@@@@
*/
void Mod_EndRegistration( void ) {
	int i;
	model_t *mod;

	for( i = 0, mod = mod_known; i < mod_numknown; i++, mod++ ) {
		if( !mod->name[ 0 ] ) continue;
		if( mod->registration_sequence !=
			registration_sequence ) {  // don't need this model
			Mod_Free( mod );
		}
	}

	GL_FreeUnusedImages();
}

//=============================================================================

/*
================
Mod_Free
================
*/
void Mod_Free( model_t *mod ) {
	Hunk_Free( mod->extradata );
	memset( mod, 0, sizeof( *mod ) );
}

/*
================
Mod_FreeAll
================
*/
void Mod_FreeAll( void ) {
	int i;

	for( i = 0; i < mod_numknown; i++ ) {
		if( mod_known[ i ].extradatasize ) Mod_Free( &mod_known[ i ] );
	}

	
}
