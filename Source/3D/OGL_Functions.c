#include <SDL3/SDL.h>

#include "game.h"
#include "ogl_functions.h"

#ifdef __ANDROID__

// On Android, glActiveTexture is a core GLES3 function, no dynamic loading needed.
void OGL_InitFunctions(void)
{
	// No-op on Android: native GLES3 functions are linked directly.
}

#else

#include <SDL3/SDL_opengl.h>

PFNGLACTIVETEXTUREARBPROC			procptr_glActiveTextureARB			= NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC		procptr_glClientActiveTextureARB	= NULL;

void OGL_InitFunctions(void)
{
	procptr_glActiveTextureARB			= (PFNGLACTIVETEXTUREARBPROC) SDL_GL_GetProcAddress("glActiveTextureARB");
	procptr_glClientActiveTextureARB	= (PFNGLCLIENTACTIVETEXTUREARBPROC) SDL_GL_GetProcAddress("glClientActiveTextureARB");

	GAME_ASSERT(procptr_glActiveTextureARB);
	GAME_ASSERT(procptr_glClientActiveTextureARB);
}

#endif // __ANDROID__
