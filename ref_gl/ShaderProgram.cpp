/*
Copyright (C) 2021 Mark E Sowden <hogsy@oldtimes-software.com>

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

#include <cassert>

#include "qgl.h"

namespace q2 {
	class ShaderProgram {
	public:
		ShaderProgram( const char *vsPath, const char *fsPath );
		~ShaderProgram();

	protected:
	private:
		bgfx::ShaderHandle LoadShader( const char *path );

		bgfx::ProgramHandle myProgram;
	};

	ShaderProgram::ShaderProgram( const char *vsPath, const char *fsPath ) {
		bgfx::ShaderHandle vertexHandle = LoadShader( vsPath );
		bgfx::ShaderHandle fragmentHandle = LoadShader( fsPath );

		myProgram = bgfx::createProgram( vertexHandle, fragmentHandle, true );
	}

	ShaderProgram::~ShaderProgram() {
		bgfx::destroy( myProgram );
	}

	bgfx::ShaderHandle ShaderProgram::LoadShader( const char *path ) {
		// Use bgfx's sample setup here, since it's probably more familiar
		const char *dir;
		switch( bgfx::getRendererType() ) {
		default:
			assert( 0 );
			break;
		case bgfx::RendererType::OpenGL:
			dir = "shaders/glsl/";
			break;
		case bgfx::RendererType::OpenGLES:
			dir = "shaders/essl/";
			break;
		}

		return bgfx::ShaderHandle();
	}
}
