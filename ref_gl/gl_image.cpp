/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

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

#include "gl_local.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static std::vector< image_t > textures;

static byte			 intensitytable[ 256 ];
static unsigned char gammatable[ 256 ];

cvar_t *intensity;

unsigned	d_8to24table[ 256 ];

static bool GL_Upload8( image_t *image, byte *data, int width, int height, qboolean mipmap, qboolean is_sky );
static bool GL_Upload32( image_t *image, unsigned *data, int width, int height, qboolean mipmap );

int		gl_solid_format = 3;

int		gl_tex_solid_format = 3;
int		gl_tex_alpha_format = 4;

int		gl_filter_min = 0;
int		gl_filter_max = 0;

void GL_EnableMultitexture( qboolean enable ) {
#if 0 // todo
	if( enable ) {
		GL_SelectTexture( GL_TEXTURE1 );
		glEnable( GL_TEXTURE_2D );
		GL_TexEnv( GL_REPLACE );
	} else {
		GL_SelectTexture( GL_TEXTURE1 );
		glDisable( GL_TEXTURE_2D );
		GL_TexEnv( GL_REPLACE );
	}
	GL_SelectTexture( GL_TEXTURE0 );
	GL_TexEnv( GL_REPLACE );
#endif
}

void GL_SelectTexture( unsigned int texture ) {
#if 0 // todo
	int tmu;
	if( texture == GL_TEXTURE0 ) {
		tmu = 0;
	} else {
		tmu = 1;
	}

	if( tmu == gl_state.currenttmu ) {
		return;
	}

	gl_state.currenttmu = tmu;

	glActiveTexture( texture );
	glClientActiveTexture( texture );
#endif
}

void GL_TexEnv( unsigned int mode ) {
#if 0 // todo
	static int lastmodes[ 2 ] = { -1, -1 };

	if( mode != lastmodes[ gl_state.currenttmu ] ) {
		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode );
		lastmodes[ gl_state.currenttmu ] = mode;
	}
#endif
}

void GL_Bind( bgfx::TextureHandle texnum ) {
#if 0 // todo
	extern	image_t *draw_chars;

	if( gl_nobind->value && draw_chars )		// performance evaluation option
		texnum = draw_chars->texnum;
	if( gl_state.currenttextures[ gl_state.currenttmu ] == texnum )
		return;
	gl_state.currenttextures[ gl_state.currenttmu ] = texnum;
	glBindTexture( GL_TEXTURE_2D, texnum );
#endif
}

void GL_MBind( unsigned int target, int texnum ) {
#if 0 // todo
	GL_SelectTexture( target );
	if( target == GL_TEXTURE0 ) {
		if( gl_state.currenttextures[ 0 ] == texnum )
			return;
	} else {
		if( gl_state.currenttextures[ 1 ] == texnum )
			return;
	}
	GL_Bind( texnum );
#endif
}

#if 0 // todo
typedef struct {
	const char *name;
	int	minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

#define NUM_GL_MODES (sizeof(modes) / sizeof (glmode_t))

typedef struct {
	const char *name;
	int mode;
} gltmode_t;

gltmode_t gl_alpha_modes[] = {
	{"default", 4},
	{"GL_RGBA", GL_RGBA},
	{"GL_RGBA8", GL_RGBA8},
	{"GL_RGB5_A1", GL_RGB5_A1},
	{"GL_RGBA4", GL_RGBA4},
	{"GL_RGBA2", GL_RGBA2},
};

#define NUM_GL_ALPHA_MODES (sizeof(gl_alpha_modes) / sizeof (gltmode_t))

gltmode_t gl_solid_modes[] = {
	{"default", 3},
	{"GL_RGB", GL_RGB},
	{"GL_RGB8", GL_RGB8},
	{"GL_RGB5", GL_RGB5},
	{"GL_RGB4", GL_RGB4},
	{"GL_R3_G3_B2", GL_R3_G3_B2},
#ifdef GL_RGB2_EXT
	{"GL_RGB2", GL_RGB2_EXT},
#endif
};

#define NUM_GL_SOLID_MODES (sizeof(gl_solid_modes) / sizeof (gltmode_t))
#endif

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode( char *string ) {
#if 0 // todo
	int		i;
	image_t *glt;

	for( i = 0; i < NUM_GL_MODES; i++ ) {
		if( !Q_stricmp( modes[ i ].name, string ) )
			break;
	}

	if( i == NUM_GL_MODES ) {
		ri.Con_Printf( PRINT_ALL, "bad filter name\n" );
		return;
	}

	gl_filter_min = modes[ i ].minimize;
	gl_filter_max = modes[ i ].maximize;

	// change all the existing mipmap texture objects
	for( i = 0, glt = gltextures; i < numgltextures; i++, glt++ ) {
		if( glt->type != it_pic && glt->type != it_sky ) {
			GL_Bind( glt->texnum );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max );
		}
	}
#endif
}

/*
===============
GL_TextureAlphaMode
===============
*/
void GL_TextureAlphaMode( char *string ) {
#if 0 // todo
	unsigned int		i;

	for( i = 0; i < NUM_GL_ALPHA_MODES; i++ ) {
		if( !Q_stricmp( gl_alpha_modes[ i ].name, string ) )
			break;
	}

	if( i == NUM_GL_ALPHA_MODES ) {
		ri.Con_Printf( PRINT_ALL, "bad alpha texture mode name\n" );
		return;
	}

	gl_tex_alpha_format = gl_alpha_modes[ i ].mode;
#endif
}

/*
===============
GL_TextureSolidMode
===============
*/
void GL_TextureSolidMode( char *string ) {
#if 0 // todo
	unsigned int		i;

	for( i = 0; i < NUM_GL_SOLID_MODES; i++ ) {
		if( !Q_stricmp( gl_solid_modes[ i ].name, string ) )
			break;
	}

	if( i == NUM_GL_SOLID_MODES ) {
		ri.Con_Printf( PRINT_ALL, "bad solid texture mode name\n" );
		return;
	}

	gl_tex_solid_format = gl_solid_modes[ i ].mode;
#endif
}

void GL_ImageList_f( void ) {
	ri.Con_Printf( PRINT_ALL, "------------------\n" );

	unsigned int texels = 0;
	for( const auto &i : textures ) {
		if( !bgfx::isValid( i.handle ) ) {
			continue;
		}

		texels += i.upload_width * i.upload_height;

		switch( i.type ) {
		case it_skin:
			ri.Con_Printf( PRINT_ALL, "M" );
			break;
		case it_sprite:
			ri.Con_Printf( PRINT_ALL, "S" );
			break;
		case it_wall:
			ri.Con_Printf( PRINT_ALL, "W" );
			break;
		case it_pic:
			ri.Con_Printf( PRINT_ALL, "P" );
			break;
		default:
			ri.Con_Printf( PRINT_ALL, " " );
			break;
		}

		ri.Con_Printf( PRINT_ALL, " %3i %3i: %s\n", i.upload_width, i.upload_height, i.name );
	}

	ri.Con_Printf( PRINT_ALL, "Total texel count (not counting mipmaps): %i\n", texels );
}

/*
=================================================================

PCX LOADING

=================================================================
*/

void LoadPCX( const char *filename, byte **pic, byte **palette, int *width, int *height ) {
	byte *raw;
	pcx_t *pcx;
	int		x, y;
	int		len;
	int		dataByte, runLength;
	byte *out, *pix;

	*pic = NULL;
	*palette = NULL;

	//
	// load the file
	//
	len = ri.FS_LoadFile( filename, (void **)&raw );
	if( !raw ) {
		ri.Con_Printf( PRINT_DEVELOPER, "Bad pcx file %s\n", filename );
		return;
	}

	//
	// parse the PCX file
	//
	pcx = (pcx_t *)raw;

	pcx->xmin = LittleShort( pcx->xmin );
	pcx->ymin = LittleShort( pcx->ymin );
	pcx->xmax = LittleShort( pcx->xmax );
	pcx->ymax = LittleShort( pcx->ymax );
	pcx->hres = LittleShort( pcx->hres );
	pcx->vres = LittleShort( pcx->vres );
	pcx->bytes_per_line = LittleShort( pcx->bytes_per_line );
	pcx->palette_type = LittleShort( pcx->palette_type );

	raw = &pcx->data;

	if( pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->xmax >= 640
		|| pcx->ymax >= 480 ) {
		ri.Con_Printf( PRINT_ALL, "Bad pcx file %s\n", filename );
		return;
	}

	out = static_cast<byte *>( malloc( ( pcx->ymax + 1 ) * ( pcx->xmax + 1 ) ) );

	*pic = out;

	pix = out;

	if( palette ) {
		*palette = static_cast<byte *>( malloc( 768 ) );
		memcpy( *palette, (byte *)pcx + len - 768, 768 );
	}

	if( width )
		*width = pcx->xmax + 1;
	if( height )
		*height = pcx->ymax + 1;

	for( y = 0; y <= pcx->ymax; y++, pix += pcx->xmax + 1 ) {
		for( x = 0; x <= pcx->xmax; ) {
			dataByte = *raw++;

			if( ( dataByte & 0xC0 ) == 0xC0 ) {
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			} else
				runLength = 1;

			while( runLength-- > 0 )
				pix[ x++ ] = dataByte;
		}

	}

	if( raw - (byte *)pcx > len ) {
		ri.Con_Printf( PRINT_DEVELOPER, "PCX file %s was malformed", filename );
		free( *pic );
		*pic = NULL;
	}

	ri.FS_FreeFile( pcx );
}

static void LoadImage32( const char *name, byte **pic, int *width, int *height ) {
	*pic = NULL;

	byte *buffer;
	int length = ri.FS_LoadFile( name, (void **)&buffer );
	if( buffer == NULL ) {
		ri.Con_Printf( PRINT_DEVELOPER, "Bad image file %s\n", name );
		return;
	}

	int comp; /* this gets ignored for now */
	byte *rgbData = stbi_load_from_memory( buffer, length, width, height, &comp, 4 );
	if( rgbData == NULL ) {
		ri.Sys_Error( ERR_DROP, "Failed to read %s!\n%s\n", name, stbi_failure_reason() );
	}

	*pic = rgbData;

	ri.FS_FreeFile( buffer );
}

/*
====================================================================

IMAGE FLOOD FILLING

====================================================================
*/


/*
=================
Mod_FloodFillSkin

Fill background pixels so mipmapping doesn't have haloes
=================
*/

typedef struct {
	short		x, y;
} floodfill_t;

// must be a power of 2
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP( off, dx, dy ) \
{ \
	if (pos[off] == fillcolor) \
	{ \
		pos[off] = 255; \
		fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy); \
		inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
	} \
	else if (pos[off] != 255) fdc = pos[off]; \
}

void R_FloodFillSkin( byte *skin, int skinwidth, int skinheight ) {
	byte				fillcolor = *skin; // assume this is the pixel to fill
	floodfill_t			fifo[ FLOODFILL_FIFO_SIZE ];
	int					inpt = 0, outpt = 0;
	int					filledcolor = -1;
	int					i;

	if( filledcolor == -1 ) {
		filledcolor = 0;
		// attempt to find opaque black
		for( i = 0; i < 256; ++i )
			if( d_8to24table[ i ] == ( 255 << 0 ) ) // alpha 1.0
			{
				filledcolor = i;
				break;
			}
	}

	// can't fill to filled color or to transparent color (used as visited marker)
	if( ( fillcolor == filledcolor ) || ( fillcolor == 255 ) ) {
		//printf( "not filling skin from %d to %d\n", fillcolor, filledcolor );
		return;
	}

	fifo[ inpt ].x = 0, fifo[ inpt ].y = 0;
	inpt = ( inpt + 1 ) & FLOODFILL_FIFO_MASK;

	while( outpt != inpt ) {
		int			x = fifo[ outpt ].x, y = fifo[ outpt ].y;
		int			fdc = filledcolor;
		byte *pos = &skin[ x + skinwidth * y ];

		outpt = ( outpt + 1 ) & FLOODFILL_FIFO_MASK;

		if( x > 0 )				FLOODFILL_STEP( -1, -1, 0 );
		if( x < skinwidth - 1 )	FLOODFILL_STEP( 1, 1, 0 );
		if( y > 0 )				FLOODFILL_STEP( -skinwidth, 0, -1 );
		if( y < skinheight - 1 )	FLOODFILL_STEP( skinwidth, 0, 1 );
		skin[ x + skinwidth * y ] = fdc;
	}
}

//=======================================================


/*
================
GL_ResampleTexture
================
*/
void GL_ResampleTexture( unsigned *in, int inwidth, int inheight, unsigned *out, int outwidth, int outheight ) {
	int		i, j;
	unsigned *inrow, *inrow2;
	unsigned	frac, fracstep;
	unsigned	p1[ 1024 ], p2[ 1024 ];
	byte *pix1, *pix2, *pix3, *pix4;

	fracstep = inwidth * 0x10000 / outwidth;

	frac = fracstep >> 2;
	for( i = 0; i < outwidth; i++ ) {
		p1[ i ] = 4 * ( frac >> 16 );
		frac += fracstep;
	}
	frac = 3 * ( fracstep >> 2 );
	for( i = 0; i < outwidth; i++ ) {
		p2[ i ] = 4 * ( frac >> 16 );
		frac += fracstep;
	}

	for( i = 0; i < outheight; i++, out += outwidth ) {
		inrow = in + inwidth * (int)( ( i + 0.25 ) * inheight / outheight );
		inrow2 = in + inwidth * (int)( ( i + 0.75 ) * inheight / outheight );
		frac = fracstep >> 1;
		for( j = 0; j < outwidth; j++ ) {
			pix1 = (byte *)inrow + p1[ j ];
			pix2 = (byte *)inrow + p2[ j ];
			pix3 = (byte *)inrow2 + p1[ j ];
			pix4 = (byte *)inrow2 + p2[ j ];
			( (byte *)( out + j ) )[ 0 ] = ( pix1[ 0 ] + pix2[ 0 ] + pix3[ 0 ] + pix4[ 0 ] ) >> 2;
			( (byte *)( out + j ) )[ 1 ] = ( pix1[ 1 ] + pix2[ 1 ] + pix3[ 1 ] + pix4[ 1 ] ) >> 2;
			( (byte *)( out + j ) )[ 2 ] = ( pix1[ 2 ] + pix2[ 2 ] + pix3[ 2 ] + pix4[ 2 ] ) >> 2;
			( (byte *)( out + j ) )[ 3 ] = ( pix1[ 3 ] + pix2[ 3 ] + pix3[ 3 ] + pix4[ 3 ] ) >> 2;
		}
	}
}

/*
================
GL_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void GL_LightScaleTexture( unsigned *in, int inwidth, int inheight, qboolean only_gamma ) {
	if( only_gamma ) {
		int		i, c;
		byte *p;

		p = (byte *)in;

		c = inwidth * inheight;
		for( i = 0; i < c; i++, p += 4 ) {
			p[ 0 ] = gammatable[ p[ 0 ] ];
			p[ 1 ] = gammatable[ p[ 1 ] ];
			p[ 2 ] = gammatable[ p[ 2 ] ];
		}
	} else {
		int		i, c;
		byte *p;

		p = (byte *)in;

		c = inwidth * inheight;
		for( i = 0; i < c; i++, p += 4 ) {
			p[ 0 ] = gammatable[ intensitytable[ p[ 0 ] ] ];
			p[ 1 ] = gammatable[ intensitytable[ p[ 1 ] ] ];
			p[ 2 ] = gammatable[ intensitytable[ p[ 2 ] ] ];
		}
	}
}

/*
================
GL_MipMap

Operates in place, quartering the size of the texture
================
*/
void GL_MipMap( byte *in, int width, int height ) {
	int		i, j;
	byte *out;

	width <<= 2;
	height >>= 1;
	out = in;
	for( i = 0; i < height; i++, in += width ) {
		for( j = 0; j < width; j += 8, out += 4, in += 8 ) {
			out[ 0 ] = ( in[ 0 ] + in[ 4 ] + in[ width + 0 ] + in[ width + 4 ] ) >> 2;
			out[ 1 ] = ( in[ 1 ] + in[ 5 ] + in[ width + 1 ] + in[ width + 5 ] ) >> 2;
			out[ 2 ] = ( in[ 2 ] + in[ 6 ] + in[ width + 2 ] + in[ width + 6 ] ) >> 2;
			out[ 3 ] = ( in[ 3 ] + in[ 7 ] + in[ width + 3 ] + in[ width + 7 ] ) >> 2;
		}
	}
}

static bool GL_Upload32( image_t *image, unsigned *data, int width, int height, qboolean mipmap ) {
	unsigned int scaled_width, scaled_height;
	for( scaled_width = 1; scaled_width < width; scaled_width <<= 1 )
		;
	if( gl_round_down->value && scaled_width > width && mipmap )
		scaled_width >>= 1;
	for( scaled_height = 1; scaled_height < height; scaled_height <<= 1 )
		;
	if( gl_round_down->value && scaled_height > height && mipmap )
		scaled_height >>= 1;

	// let people sample down the world textures for speed
	if( mipmap ) {
		scaled_width >>= (int)gl_picmip->value;
		scaled_height >>= (int)gl_picmip->value;
	}

	// don't ever bother with >256 textures
	if( scaled_width > 256 )
		scaled_width = 256;
	if( scaled_height > 256 )
		scaled_height = 256;

	if( scaled_width < 1 )
		scaled_width = 1;
	if( scaled_height < 1 )
		scaled_height = 1;

	image->upload_width = scaled_width;
	image->upload_height = scaled_height;

	unsigned int scaled[ 256 * 256 ];
	if( scaled_width * scaled_height > sizeof( scaled ) / 4 ) {
		ri.Sys_Error( ERR_DROP, "GL_Upload32: too big" );
	}

	// scan the texture for any non-255 alpha
	unsigned int c = width * height;
	byte *scan = ( (byte *)data ) + 3;
	bool hasAlpha = false;
	for( unsigned int i = 0; i < c; i++, scan += 4 ) {
		if( *scan != 255 ) {
			hasAlpha = true;
			break;
		}
	}

#if 0 // todo
	if( mipmap ) {
		unsigned int miplevel = 0;
		while( scaled_width > 1 || scaled_height > 1 ) {
			GL_MipMap( (byte *)scaled, scaled_width, scaled_height );
			scaled_width >>= 1;
			scaled_height >>= 1;
			if( scaled_width < 1 )
				scaled_width = 1;
			if( scaled_height < 1 )
				scaled_height = 1;
			miplevel++;
			glTexImage2D( GL_TEXTURE_2D, miplevel, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled );
		}
	}
#endif
	
	const bgfx::Memory *memoryPtr = bgfx::makeRef( data, width * height * ( hasAlpha ? 4 : 3 ) );
	image->handle = bgfx::createTexture2D( width, height, false, 1, hasAlpha ? bgfx::TextureFormat::RGBA8 : bgfx::TextureFormat::RGB8, 0U, memoryPtr );

#if 0
	if( scaled_width == width && scaled_height == height ) {
		if( !mipmap ) {
			glTexImage2D( GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
			goto done;
		}
		memcpy( scaled, data, width * height * 4 );
	} else
		GL_ResampleTexture( data, width, height, scaled, scaled_width, scaled_height );

	glTexImage2D( GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled );
done:;

	if( mipmap ) {
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max );
	} else {
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max );
	}
#endif

	return hasAlpha;
}

static bool GL_Upload8( image_t *image, byte *data, int width, int height, qboolean mipmap, qboolean is_sky ) {
	unsigned	trans[ 640 * 256 ];
	int			i, s;
	int			p;

	s = width * height;

	if( s > sizeof( trans ) / 4 )
		ri.Sys_Error( ERR_DROP, "GL_Upload8: too large" );

	for( i = 0; i < s; i++ ) {
		p = data[ i ];
		trans[ i ] = d_8to24table[ p ];

		if( p == 255 ) {	// transparent, so scan around for another color
			// to avoid alpha fringes
			// FIXME: do a full flood fill so mips work...
			if( i > width && data[ i - width ] != 255 )
				p = data[ i - width ];
			else if( i < s - width && data[ i + width ] != 255 )
				p = data[ i + width ];
			else if( i > 0 && data[ i - 1 ] != 255 )
				p = data[ i - 1 ];
			else if( i < s - 1 && data[ i + 1 ] != 255 )
				p = data[ i + 1 ];
			else
				p = 0;
			// copy rgb components
			( (byte *)&trans[ i ] )[ 0 ] = ( (byte *)&d_8to24table[ p ] )[ 0 ];
			( (byte *)&trans[ i ] )[ 1 ] = ( (byte *)&d_8to24table[ p ] )[ 1 ];
			( (byte *)&trans[ i ] )[ 2 ] = ( (byte *)&d_8to24table[ p ] )[ 2 ];
		}
	}

	return GL_Upload32( image, trans, width, height, mipmap );
}

/**
 * This is also used as an entry point for the generated r_notexture
 */
image_t *GL_LoadPic( const char *name, byte *pic, int width, int height, imagetype_t type, int bits ) {
	image_t image = image_t( name, width, height, type );

	if( type == it_skin && bits == 8 ) {
		R_FloodFillSkin( pic, width, height );
	}

	if( bits == 8 ) {
		image.has_alpha = GL_Upload8( &image, pic, width, height, ( image.type != it_pic && image.type != it_sky ), image.type == it_sky );
	} else {
		image.has_alpha = GL_Upload32( &image, (unsigned *)pic, width, height, ( image.type != it_pic && image.type != it_sky ) );
	}

	textures.push_back( image );

	return &textures.back();
}

/*
===============
GL_FindImage

Finds or loads the given image
===============
*/
image_t *GL_FindImage( const char *name, imagetype_t type ) {
	byte *pic, *palette;
	int		width, height;

	if( !name ) {
		return NULL;	//	ri.Sys_Error (ERR_DROP, "GL_FindImage: NULL name");
	}

	size_t len = strlen( name );
	if( len < 5 ) {
		return NULL;	//	ri.Sys_Error (ERR_DROP, "GL_FindImage: bad name: %s", name);
	}

	// look for it
	for( auto &i : textures ) {
		if( i.name == name ) {
			return &i;
		}
	}

	image_t *image = NULL;

	//
	// load the pic from disk
	//

  // Right, this is kinda gross but Anachronox seems to strip the extension
  // from the filename and then load whatever standard formats it supports

	struct ImageLoader {
		const char *extension;
		unsigned int depth;
		void( *Load8bpp )( const char *filename, byte **pic, byte **palette, int *width, int *height );
		void( *Load32bpp )( const char *filename, byte **pic, int *width, int *height );
	} loaders[] = {
		{ "tga", 32, NULL, LoadImage32 },
		{ "png", 32, NULL, LoadImage32 },
		{ "bmp", 32, NULL, LoadImage32 },
		{ "pcx", 8, LoadPCX, NULL },
	};

	char uname[ MAX_QPATH ];
	strcpy( uname, name );

	for( auto &loader : loaders ) {
		uname[ len - 3 ] = '\0';
		strcat( uname, loader.extension );

		pic = nullptr;
		palette = nullptr;
		if( loader.depth == 8 ) {
			loader.Load8bpp( uname, &pic, &palette, &width, &height );
		} else {
			loader.Load32bpp( uname, &pic, &width, &height );
		}

		if( pic == nullptr ) {
			continue;
		}

		image = GL_LoadPic( uname, pic, width, height, type, loader.depth );
		// HACK: store the original name for comparing later!
		image->name = name;
	}

	if( image == nullptr ) {
		Com_Printf( "WARNING: Failed to find \"%s\"!\n", uname );
		return nullptr;
	}

	free( pic );
	free( palette );

	return image;
}

struct image_t *R_RegisterSkin( const char *name ) {
	return GL_FindImage( name, it_skin );
}


/*
================
GL_FreeUnusedImages

Any image that was not touched on this registration sequence
will be freed.
================
*/
void GL_FreeUnusedImages( void ) {
#if 0 // todo
	int		i;
	image_t *image;

	// never free r_notexture or particle texture
	r_notexture->registration_sequence = registration_sequence;
	r_particletexture->registration_sequence = registration_sequence;

	for( i = 0, image = gltextures; i < numgltextures; i++, image++ ) {
		if( image->registration_sequence == registration_sequence )
			continue;		// used this sequence
		if( !image->registration_sequence )
			continue;		// free image_t slot
		if( image->type == it_pic )
			continue;		// don't free pics
		// free it
		glDeleteTextures( 1, (GLuint *)&image->texnum );
		memset( image, 0, sizeof( *image ) );
	}
#endif
}

int Draw_GetPalette( void ) {
	int		i;
	int		r, g, b;
	unsigned	v;
	byte *pic, *pal;
	int		width, height;

	// get the palette

	LoadPCX( "graphics/colormap.pcx", &pic, &pal, &width, &height );
	if( !pal )
		ri.Sys_Error( ERR_FATAL, "Couldn't load graphics/colormap.pcx" );

	for( i = 0; i < 256; i++ ) {
		r = pal[ i * 3 + 0 ];
		g = pal[ i * 3 + 1 ];
		b = pal[ i * 3 + 2 ];

		v = ( 255 << 24 ) + ( r << 0 ) + ( g << 8 ) + ( b << 16 );
		d_8to24table[ i ] = LittleLong( v );
	}

	d_8to24table[ 255 ] &= LittleLong( 0xffffff );	// 255 is transparent

	free( pic );
	free( pal );

	return 0;
}

void GL_InitImages( void ) {
	int		i, j;
	float	g = vid_gamma->value;

	registration_sequence = 1;

	// init intensity conversions
	intensity = ri.Cvar_Get( "intensity", "2", 0 );

	if( intensity->value <= 1 )
		ri.Cvar_Set( "intensity", "1" );

	gl_state.inverse_intensity = 1 / intensity->value;

	Draw_GetPalette();

	if( gl_config.renderer & GL_RENDERER_VOODOO ) {
		g = 1.0F;
	}

	for( i = 0; i < 256; i++ ) {
		if( g == 1 ) {
			gammatable[ i ] = i;
		} else {
			float inf;

			inf = 255 * pow( ( i + 0.5 ) / 255.5, g ) + 0.5;
			if( inf < 0 )
				inf = 0;
			if( inf > 255 )
				inf = 255;
			gammatable[ i ] = inf;
		}
	}

	for( i = 0; i < 256; i++ ) {
		j = i * intensity->value;
		if( j > 255 )
			j = 255;
		intensitytable[ i ] = j;
	}
}

void GL_ShutdownImages( void ) {
	for( auto &i : textures ) {
		bgfx::destroy( i.handle );
	}

	textures.clear();
}
