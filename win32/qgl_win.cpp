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

#if 0 // todo
static void GLAPIENTRY QGL_ErrorCallback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam ) {
	Q_unused( source, type, id, severity, length, userParam );

	fprintf( glw_state.log_fp, "GL ERROR: %s", message );
}
#endif

/*
** QGL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.
*/
void QGL_Shutdown( void ) {
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
	
	return bgfx::init( init );
}

void GLimp_EnableLogging( qboolean enable ) {
#if 0 // todo
	if( !enable ) {
		glDisable( GL_DEBUG_OUTPUT );

		if( glw_state.log_fp != NULL ) {
			fclose( glw_state.log_fp );
			glw_state.log_fp = NULL;
		}

		return;
	}

	if( glw_state.log_fp != NULL ) {
		// Assume logging has already been enabled
		return;
	}

	glEnable( GL_DEBUG_OUTPUT );

	time_t aclock;
	time( &aclock );

	struct tm *newtime = localtime( &aclock );
	asctime( newtime );

	char buffer[ 1024 ];
	Com_sprintf( buffer, sizeof( buffer ), "%s/gl.log", ri.FS_Gamedir() );
	glw_state.log_fp = fopen( buffer, "wt" );

	fprintf( glw_state.log_fp, "%s\n", asctime( newtime ) );
#endif
}

void GLimp_LogNewFrame( void ) {
#if 0 // todo
	if( glw_state.log_fp == NULL && gl_log->value ) {
		// Probably lost handle to the log file!
		GLimp_EnableLogging( gl_log->value );
	}

	fprintf( glw_state.log_fp, "*** R_BeginFrame ***\n" );
#endif
}
