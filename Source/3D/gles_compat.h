// gles_compat.h
// OpenGL ES 3.0 compatibility layer for Billy Frontier Android port
// Redirects fixed-function OpenGL calls to GLES3 bridge functions

#pragma once

#ifdef __ANDROID__

#include <GLES3/gl3.h>

// GLdouble is not defined in GLES3 headers
typedef double GLdouble;

// Missing pixel format constants
#ifndef GL_BGRA
#define GL_BGRA  0x80E1
#endif
#ifndef GL_BGR
#define GL_BGR   0x80E0
#endif
#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#endif
#ifndef GL_LUMINANCE
#define GL_LUMINANCE 0x1909
#endif
#ifndef GL_LUMINANCE_ALPHA
#define GL_LUMINANCE_ALPHA 0x190A
#endif

// Missing GL constants
#ifndef GL_CLAMP
#define GL_CLAMP GL_CLAMP_TO_EDGE
#endif
#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif
#ifndef GL_QUAD_STRIP
#define GL_QUAD_STRIP 0x0008
#endif
#ifndef GL_POLYGON
#define GL_POLYGON 0x0009
#endif
#ifndef GL_MODELVIEW
#define GL_MODELVIEW 0x1700
#endif
#ifndef GL_PROJECTION
#define GL_PROJECTION 0x1701
#endif
#ifndef GL_TEXTURE
#define GL_TEXTURE 0x1702
#endif
#ifndef GL_AMBIENT
#define GL_AMBIENT 0x1200
#endif
#ifndef GL_DIFFUSE
#define GL_DIFFUSE 0x1201
#endif
#ifndef GL_SPECULAR
#define GL_SPECULAR 0x1202
#endif
#ifndef GL_POSITION
#define GL_POSITION 0x1203
#endif
#ifndef GL_AMBIENT_AND_DIFFUSE
#define GL_AMBIENT_AND_DIFFUSE 0x1602
#endif
#ifndef GL_SHININESS
#define GL_SHININESS 0x1601
#endif
#ifndef GL_EMISSION
#define GL_EMISSION 0x1600
#endif
#ifndef GL_FRONT_AND_BACK
#define GL_FRONT_AND_BACK 0x0408
#endif
#ifndef GL_LIGHTING
#define GL_LIGHTING 0x0B50
#endif
#ifndef GL_LIGHT0
#define GL_LIGHT0 0x4000
#endif
#ifndef GL_LIGHT1
#define GL_LIGHT1 0x4001
#endif
#ifndef GL_LIGHT2
#define GL_LIGHT2 0x4002
#endif
#ifndef GL_LIGHT3
#define GL_LIGHT3 0x4003
#endif
#ifndef GL_FOG
#define GL_FOG 0x0B60
#endif
#ifndef GL_FOG_COLOR
#define GL_FOG_COLOR 0x0B66
#endif
#ifndef GL_FOG_DENSITY
#define GL_FOG_DENSITY 0x0B62
#endif
#ifndef GL_FOG_END
#define GL_FOG_END 0x0B64
#endif
#ifndef GL_FOG_START
#define GL_FOG_START 0x0B63
#endif
#ifndef GL_FOG_MODE
#define GL_FOG_MODE 0x0B65
#endif
#ifndef GL_LINEAR
#define GL_LINEAR 0x2601
#endif
#ifndef GL_EXP
#define GL_EXP 0x0800
#endif
#ifndef GL_EXP2
#define GL_EXP2 0x0801
#endif
#ifndef GL_ALPHA_TEST
#define GL_ALPHA_TEST 0x0BC0
#endif
#ifndef GL_NORMALIZE
#define GL_NORMALIZE 0x0BA1
#endif
#ifndef GL_COLOR_MATERIAL
#define GL_COLOR_MATERIAL 0x0B57
#endif
#ifndef GL_TEXTURE_ENV
#define GL_TEXTURE_ENV 0x2300
#endif
#ifndef GL_TEXTURE_ENV_MODE
#define GL_TEXTURE_ENV_MODE 0x2200
#endif
#ifndef GL_MODULATE
#define GL_MODULATE 0x2100
#endif
#ifndef GL_REPLACE
#define GL_REPLACE 0x1E01
#endif
#ifndef GL_DECAL
#define GL_DECAL 0x2101
#endif
#ifndef GL_ADD
#define GL_ADD 0x0104
#endif
#ifndef GL_TEXTURE_GEN_S
#define GL_TEXTURE_GEN_S 0x0C60
#endif
#ifndef GL_TEXTURE_GEN_T
#define GL_TEXTURE_GEN_T 0x0C61
#endif
#ifndef GL_SPHERE_MAP
#define GL_SPHERE_MAP 0x2402
#endif
#ifndef GL_TEXTURE_GEN_MODE
#define GL_TEXTURE_GEN_MODE 0x2500
#endif
#ifndef GL_S
#define GL_S 0x2000
#endif
#ifndef GL_T
#define GL_T 0x2001
#endif
#ifndef GL_VERTEX_ARRAY
#define GL_VERTEX_ARRAY 0x8074
#endif
#ifndef GL_NORMAL_ARRAY
#define GL_NORMAL_ARRAY 0x8075
#endif
#ifndef GL_COLOR_ARRAY
#define GL_COLOR_ARRAY 0x8076
#endif
#ifndef GL_TEXTURE_COORD_ARRAY
#define GL_TEXTURE_COORD_ARRAY 0x8078
#endif
#ifndef GL_NEVER
#define GL_NEVER 0x0200
#endif
#ifndef GL_LESS
#define GL_LESS 0x0201
#endif
#ifndef GL_EQUAL
#define GL_EQUAL 0x0202
#endif
#ifndef GL_LEQUAL
#define GL_LEQUAL 0x0203
#endif
#ifndef GL_GREATER
#define GL_GREATER 0x0204
#endif
#ifndef GL_NOTEQUAL
#define GL_NOTEQUAL 0x0205
#endif
#ifndef GL_GEQUAL
#define GL_GEQUAL 0x0206
#endif
#ifndef GL_ALWAYS
#define GL_ALWAYS 0x0207
#endif

// Bridge function declarations
#ifdef __cplusplus
extern "C" {
#endif

// Initialization
void GLESBridge_Init(void);
void GLESBridge_Shutdown(void);

// Matrix operations
void bridge_MatrixMode(GLenum mode);
void bridge_PushMatrix(void);
void bridge_PopMatrix(void);
void bridge_LoadIdentity(void);
void bridge_MultMatrixf(const GLfloat *m);
void bridge_Translatef(GLfloat x, GLfloat y, GLfloat z);
void bridge_Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void bridge_Scalef(GLfloat x, GLfloat y, GLfloat z);
void bridge_Ortho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f);
void bridge_Frustum(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f);
void bridge_LoadMatrixf(const GLfloat *m);

// Immediate mode
void bridge_Begin(GLenum mode);
void bridge_End(void);
void bridge_Vertex2f(GLfloat x, GLfloat y);
void bridge_Vertex3f(GLfloat x, GLfloat y, GLfloat z);
void bridge_Vertex3fv(const GLfloat *v);
void bridge_Normal3f(GLfloat x, GLfloat y, GLfloat z);
void bridge_Normal3fv(const GLfloat *v);
void bridge_TexCoord2f(GLfloat s, GLfloat t);
void bridge_TexCoord2fv(const GLfloat *v);
void bridge_Color3f(GLfloat r, GLfloat g, GLfloat b);
void bridge_Color4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void bridge_Color4fv(const GLfloat *v);
void bridge_Color4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a);

// Lighting
void bridge_Lightfv(GLenum light, GLenum pname, const GLfloat *params);
void bridge_Lightf(GLenum light, GLenum pname, GLfloat param);
void bridge_LightModelfv(GLenum pname, const GLfloat *params);
void bridge_Materialfv(GLenum face, GLenum pname, const GLfloat *params);
void bridge_Materialf(GLenum face, GLenum pname, GLfloat param);

// Fog
void bridge_Fogfv(GLenum pname, const GLfloat *params);
void bridge_Fogf(GLenum pname, GLfloat param);
void bridge_Fogi(GLenum pname, GLint param);

// Alpha test
void bridge_AlphaFunc(GLenum func, GLfloat ref);

// Texture environment
void bridge_TexEnvi(GLenum target, GLenum pname, GLint param);
void bridge_TexEnvfv(GLenum target, GLenum pname, const GLfloat *params);

// Texture generation
void bridge_TexGeni(GLenum coord, GLenum pname, GLint param);

// Enable/Disable state
void bridge_Enable(GLenum cap);
void bridge_Disable(GLenum cap);

// Client state (vertex arrays)
void bridge_EnableClientState(GLenum array);
void bridge_DisableClientState(GLenum array);
void bridge_VertexPointer(GLint size, GLenum type, GLsizei stride, const void *pointer);
void bridge_NormalPointer(GLenum type, GLsizei stride, const void *pointer);
void bridge_TexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *pointer);
void bridge_ColorPointer(GLint size, GLenum type, GLsizei stride, const void *pointer);

// Draw with client arrays (must upload to VBO first)
void bridge_DrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices);
void bridge_DrawArrays(GLenum mode, GLint first, GLsizei count);

// Color material
void bridge_ColorMaterial(GLenum face, GLenum mode);

// Polygon mode (stub - not supported in GLES)
void bridge_PolygonMode(GLenum face, GLenum mode);

// Texture image with format conversion
void bridge_TexImage2D(GLenum target, GLint level, GLint internalformat,
                       GLsizei width, GLsizei height, GLint border,
                       GLenum format, GLenum type, const void *pixels);
void bridge_TexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                          GLsizei width, GLsizei height, GLenum format, GLenum type,
                          const void *pixels);

// Flush the current bridge shader state to GL
void bridge_FlushState(void);

#ifdef __cplusplus
}
#endif

// Redirect all fixed-function calls to bridge functions
// (not applied inside GLESBridge.c itself to avoid infinite recursion)
#ifndef GLES_BRIDGE_IMPLEMENTATION

// Matrix
#define glMatrixMode        bridge_MatrixMode
#define glPushMatrix        bridge_PushMatrix
#define glPopMatrix         bridge_PopMatrix
#define glLoadIdentity      bridge_LoadIdentity
#define glMultMatrixf       bridge_MultMatrixf
#define glTranslatef        bridge_Translatef
#define glRotatef           bridge_Rotatef
#define glScalef            bridge_Scalef
#define glOrtho(l,r,b,t,n,f) bridge_Ortho((GLdouble)(l),(GLdouble)(r),(GLdouble)(b),(GLdouble)(t),(GLdouble)(n),(GLdouble)(f))
#define glFrustum(l,r,b,t,n,f) bridge_Frustum((GLdouble)(l),(GLdouble)(r),(GLdouble)(b),(GLdouble)(t),(GLdouble)(n),(GLdouble)(f))
#define glLoadMatrixf       bridge_LoadMatrixf

// Immediate mode
#define glBegin             bridge_Begin
#define glEnd               bridge_End
#define glVertex2f          bridge_Vertex2f
#define glVertex3f          bridge_Vertex3f
#define glVertex3fv         bridge_Vertex3fv
#define glNormal3f          bridge_Normal3f
#define glNormal3fv         bridge_Normal3fv
#define glTexCoord2f        bridge_TexCoord2f
#define glTexCoord2fv       bridge_TexCoord2fv
#define glColor3f(r,g,b)    bridge_Color3f(r,g,b)
#define glColor4f           bridge_Color4f
#define glColor4fv          bridge_Color4fv
#define glColor4ub          bridge_Color4ub

// Lighting
#define glLightfv           bridge_Lightfv
#define glLightf            bridge_Lightf
#define glLightModelfv      bridge_LightModelfv
#define glMaterialfv        bridge_Materialfv
#define glMaterialf         bridge_Materialf

// Fog
#define glFogfv             bridge_Fogfv
#define glFogf              bridge_Fogf
#define glFogi              bridge_Fogi

// Alpha test
#define glAlphaFunc         bridge_AlphaFunc

// Texture env
#define glTexEnvi           bridge_TexEnvi
#define glTexEnvfv          bridge_TexEnvfv

// Texture gen
#define glTexGeni           bridge_TexGeni

// Enable/Disable (intercept GLES-unsupported caps)
#define glEnable            bridge_Enable
#define glDisable           bridge_Disable

// Client state
#define glEnableClientState  bridge_EnableClientState
#define glDisableClientState bridge_DisableClientState
#define glVertexPointer      bridge_VertexPointer
#define glNormalPointer      bridge_NormalPointer
#define glTexCoordPointer    bridge_TexCoordPointer
#define glColorPointer       bridge_ColorPointer

// Draw calls (must go through VBO upload)
#define glDrawElements      bridge_DrawElements
#define glDrawArrays        bridge_DrawArrays

// Color material
#define glColorMaterial     bridge_ColorMaterial

// Polygon mode (stub)
#define glPolygonMode       bridge_PolygonMode

// Texture operations
#define glTexImage2D        bridge_TexImage2D
#define glTexSubImage2D     bridge_TexSubImage2D

#endif // !GLES_BRIDGE_IMPLEMENTATION

#endif // __ANDROID__
