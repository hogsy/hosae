/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com>

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

#include <SDL2/SDL.h>

#if 0
#if defined( _WIN32 )
#	include <Windows.h>
#endif
#endif

#include <conio.h>
#include <direct.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <io.h>
#include <stdio.h>

#include "../qcommon/qcommon.h"
#include "../win32/conproc.h"

#define MINIMUM_WIN_MEMORY 0x0a00000
#define MAXIMUM_WIN_MEMORY 0x1000000

int starttime;
qboolean ActiveApp;
qboolean Minimized;

static HANDLE hinput, houtput;

unsigned sys_msg_time;
unsigned sys_frame_time;

#define MAX_NUM_ARGVS 128
int argc;
char *argv[ MAX_NUM_ARGVS ];

/*
===============================================================================

SYSTEM IO

===============================================================================
*/

namespace anox {
	class App {
	public:
		App( int argc, char **argv );
		~App();

		void DisplayMessageBox( MessageBoxType type, const char *message, ... );

		void Error( const char *error, ... );

		void Quit();

	private:
		SDL_TimerID timerId;

		SDL_Window *mainWindow{ nullptr };
	};

	extern App app;
};

anox::App anox::app;

anox::App::App( int argc, char **argv ) {
}

anox::App::~App() {
}

/**
 * Display a simple message box.
 */
void anox::App::DisplayMessageBox( MessageBoxType type, const char *message, ... ) {
	static bool isRecursive = false;
	if( isRecursive ) {
		return;
	}

	// Allocate buffer to store our message

	va_list argptr;
	va_start( argptr, message );

	int bufferLength = Q_vscprintf( message, argptr ) + 1;
	char *msgBuffer = new char[ bufferLength ];
	vsnprintf( msgBuffer, bufferLength, message, argptr );

	va_end( argptr );

	int sdlFlag;
	switch( type ) {
	default:
		sdlFlag = SDL_MESSAGEBOX_INFORMATION;
		break;
	case MessageBoxType::WARNING:
		sdlFlag = SDL_MESSAGEBOX_WARNING;
		break;
	case MessageBoxType::ERROR:
		sdlFlag = SDL_MESSAGEBOX_ERROR;
		break;
	}

	if( SDL_ShowSimpleMessageBox( sdlFlag, ENGINE_NAME, msgBuffer, nullptr ) == -1 ) {
		isRecursive = true;
		Sys_Error( "Failed to display message box for error (%s)!\nSDL: %s\n", msgBuffer, SDL_GetError() );
	}

	delete msgBuffer;
}

void anox::App::Error( const char *error, ... ) {
	va_list argptr;
	va_start( argptr, error );

	int bufferLength = Q_vscprintf( error, argptr ) + 1;
	char *msgBuffer = new char[ bufferLength ];
	vsprintf( msgBuffer, error, argptr );

	va_end( argptr );

	Sys_MessageBox( MessageBoxType::ERROR, msgBuffer );

	delete msgBuffer;

	Sys_Quit();
}

void anox::App::Quit() {
	SDL_RemoveTimer( timerId );

	CL_Shutdown();
	Qcommon_Shutdown();

	// shut down QHOST hooks if necessary
	DeinitConProc();

	SDL_Quit();

	exit( 0 );
}

static char console_text[ 256 ];
static int console_textlen;

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput( void ) {
	return nullptr;
}

/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput( char *string ) {
#if defined( _WIN32 )
	OutputDebugString( string );
#endif
}

/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents( void ) {
#if 0
	MSG msg;

	while( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
		if( !GetMessage( &msg, NULL, 0, 0 ) ) Sys_Quit();
		sys_msg_time = msg.time;
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	// grab frame time
	sys_frame_time = timeGetTime();  // FIXME: should this be at start?
#endif
}

/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( void ) {
	return SDL_GetClipboardText();
}

/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/

/*
=================
Sys_AppActivate
=================
*/
void Sys_AppActivate( void ) {
	SDL_ShowWindow( mainWindow );

	ShowWindow( cl_hwnd, SW_RESTORE );
	SetForegroundWindow( cl_hwnd );
}

/*
========================================================================

GAME DLL

========================================================================
*/

static void *game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame( void ) {
	SDL_UnloadObject( game_library );
	game_library = nullptr;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI( void *parms ) {
	void *( *GetGameAPI )( void * );
	char name[ MAX_OSPATH ];
	char *path;
	char cwd[ MAX_OSPATH ];
#if defined _M_IX86
	const char *gamename = "gamex86.dll";

#ifdef NDEBUG
	const char *debugdir = "release";
#else
	const char *debugdir = "debug";
#endif

#elif defined _M_ALPHA
	const char *gamename = "gameaxp.dll";

#ifdef NDEBUG
	const char *debugdir = "releaseaxp";
#else
	const char *debugdir = "debugaxp";
#endif

#endif

	if( game_library )
		Com_Error( ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame" );

	// check the current debug directory first for development purposes
	if( _getcwd( cwd, sizeof( cwd ) ) == NULL ) {
		Com_Error( ERR_FATAL, "Failed to get current working directory!\n" );
	}
	Com_sprintf( name, sizeof( name ), "%s/%s/%s", cwd, debugdir, gamename );
	game_library = SDL_LoadObject( name );
	if( game_library ) {
		Com_DPrintf( "LoadLibrary (%s)\n", name );
	} else {
		// now run through the search paths
		path = NULL;
		while( 1 ) {
			path = FS_NextPath( path );
			if( !path ) return NULL;  // couldn't find one anywhere
			Com_sprintf( name, sizeof( name ), "%s/%s", path, gamename );
			game_library = SDL_LoadObject( name );
			if( game_library ) {
				Com_DPrintf( "LoadLibrary (%s)\n", name );
				break;
			}
		}
	}

	GetGameAPI = ( void *( * )( void * ) )SDL_LoadFunction( game_library, "GetGameAPI" );
	if( !GetGameAPI ) {
		Sys_UnloadGame();
		return NULL;
	}

	return GetGameAPI( parms );
}

int main( int argc, char **argv ) {
	if( SDL_Init( SDL_INIT_EVERYTHING ) != 0 ) {
		Sys_Error( "Failed to initialize SDL2!\nSDL2: %s\n", SDL_GetError() );
		return EXIT_FAILURE;
	}

	Qcommon_Init( argc, argv );
	int oldtime = Sys_Milliseconds();

	/* main window message loop */
	int time, newtime;
	while( 1 ) {
		// if at a full screen console, don't update unless needed
		if( Minimized || ( dedicated && dedicated->value ) ) {
			Sleep( 1 );
		}

		while( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
			if( !GetMessage( &msg, NULL, 0, 0 ) ) Com_Quit();
			sys_msg_time = msg.time;
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}

		do {
			newtime = Sys_Milliseconds();
			time = newtime - oldtime;
		} while( time < 1 );
		//			Con_Printf ("time:%5.2f - %5.2f = %5.2f\n", newtime,
		//oldtime, time);

		_controlfp( _PC_24, _MCW_PC );
		Qcommon_Frame( time );

		oldtime = newtime;
	}

	// never gets here
	return EXIT_SUCCESS;
}
