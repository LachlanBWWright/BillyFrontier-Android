#pragma once

#ifdef __ANDROID__

// On Android/GLES3: glActiveTexture is available natively (no ARB extensions needed).
// glClientActiveTexture is a fixed-function concept not needed with the VBO bridge.
#ifndef GL_TEXTURE0_ARB
#define GL_TEXTURE0_ARB 0x84C0
#define GL_TEXTURE1_ARB 0x84C1
#define GL_TEXTURE2_ARB 0x84C2
#endif
#define glActiveTextureARB  glActiveTexture
#define glClientActiveTextureARB(x) ((void)(x))

void OGL_InitFunctions(void);

#else

#include <SDL3/SDL_opengl.h>

extern PFNGLACTIVETEXTUREARBPROC			procptr_glActiveTextureARB;
extern PFNGLCLIENTACTIVETEXTUREARBPROC		procptr_glClientActiveTextureARB;

#define glActiveTextureARB					procptr_glActiveTextureARB
#define glClientActiveTextureARB			procptr_glClientActiveTextureARB

void OGL_InitFunctions(void);

#endif // __ANDROID__
