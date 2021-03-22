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
/*
** QGL_WIN.C
**
** This file implements the operating system binding of GL to QGL function
** pointers.  When doing a port of Quake2 you must implement the following
** two functions:
**
** QGL_Init() - loads libraries, assigns function pointers, etc.
** QGL_Shutdown() - unloads libraries, NULLs function pointers
*/

#include "../ref_gl/gl_local.h"
#include "glw_win.h"

/*
** QGL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.
*/
void QGL_Shutdown( void ) {
	bgfx::shutdown();
}

/*
** QGL_Init
**
** This is responsible for binding our qgl function pointers to
** the appropriate GL stuff.  In Windows this means doing a
** LoadLibrary and a bunch of calls to GetProcAddress.  On other
** operating systems we need to do the right thing, whatever that
** might be.
**
*/
bool QGL_Init( unsigned int w, unsigned int h, void *winHandle ) {
	bgfx::PlatformData platformData;
	platformData.nwh = glw_state.hWnd;
	bgfx::setPlatformData( platformData );

	bgfx::Init init;
	init.type = bgfx::RendererType::Count;
	init.resolution.width = w;
	init.resolution.height = h;
	init.resolution.reset = BGFX_RESET_VSYNC;
	
	if( !bgfx::init( init ) ) {
		return false;
	}

#if defined( _DEBUG )
	bgfx::setDebug( BGFX_DEBUG_TEXT | BGFX_DEBUG_STATS );
#endif

	return true;
}

void GLimp_EnableLogging( qboolean enable ) {
}

void GLimp_LogNewFrame( void ) {
}
