// GLESBridge.c
// OpenGL ES 3.0 compatibility bridge for Billy Frontier Android port
// Emulates fixed-function OpenGL using GLES3 shaders

#ifdef __ANDROID__

#include <GLES3/gl3.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <android/log.h>

// Define this before including gles_compat.h so the macro redirects are NOT
// applied inside this file (would cause infinite recursion).
#define GLES_BRIDGE_IMPLEMENTATION
#include "gles_compat.h"

#define BRIDGE_TAG "BillyFrontier"
#define BRIDGE_LOGI(...) __android_log_print(ANDROID_LOG_INFO, BRIDGE_TAG, __VA_ARGS__)
#define BRIDGE_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, BRIDGE_TAG, __VA_ARGS__)

// ============================================================================
// Math types
// ============================================================================

typedef float Mat4[16];

static void Mat4_Identity(Mat4 m) {
    memset(m, 0, sizeof(Mat4));
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void Mat4_Multiply(Mat4 result, const Mat4 a, const Mat4 b) {
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += a[k*4 + row] * b[col*4 + k];
            }
            result[col*4 + row] = sum;
        }
    }
}

static void Mat4_Copy(Mat4 dst, const Mat4 src) {
    memcpy(dst, src, sizeof(Mat4));
}

// ============================================================================
// Matrix stacks
// ============================================================================

#define MATRIX_STACK_DEPTH 16

typedef struct {
    Mat4 stack[MATRIX_STACK_DEPTH];
    int top;
} MatrixStack;

static MatrixStack gMVStack;
static MatrixStack gProjStack;
static MatrixStack gTexStack;
static GLenum gCurrentMatrixMode = GL_MODELVIEW;

static MatrixStack* CurrentStack(void) {
    switch (gCurrentMatrixMode) {
        case GL_PROJECTION: return &gProjStack;
        case GL_TEXTURE:    return &gTexStack;
        default:            return &gMVStack;
    }
}

static Mat4* CurrentTop(void) {
    return &CurrentStack()->stack[CurrentStack()->top];
}

// ============================================================================
// Shader uniforms state
// ============================================================================

typedef struct {
    float pos[4];
    float ambient[4];
    float diffuse[4];
    float specular[4];
    int   enabled;
} BridgeLight;

#define MAX_LIGHTS 4

typedef struct {
    // Matrices
    Mat4 modelview;
    Mat4 projection;
    Mat4 texture;
    Mat4 mvp;           // computed = projection * modelview

    // Lighting
    int  lightingEnabled;
    int  normalizeEnabled;
    int  colorMaterialEnabled;
    BridgeLight lights[MAX_LIGHTS];
    float ambientLight[4];
    float materialAmbient[4];
    float materialDiffuse[4];
    float materialSpecular[4];
    float materialEmission[4];
    float materialShininess;

    // Fog
    int   fogEnabled;
    int   fogMode;
    float fogColor[4];
    float fogStart;
    float fogEnd;
    float fogDensity;

    // Alpha test
    int   alphaTestEnabled;
    GLenum alphaFunc;
    float alphaRef;

    // Texture
    int   texture2DEnabled;
    int   texEnvMode;      // GL_MODULATE, GL_REPLACE, GL_DECAL, GL_ADD
    int   texGenEnabled;   // sphere map

    // Current color (for no-lighting mode)
    float currentColor[4];

    // Current normal and texcoord (for immediate mode)
    float currentNormal[3];
    float currentTexcoord[2];

    // Dirty flag - need to re-upload uniforms
    int dirty;
} BridgeState;

static BridgeState gState;

// ============================================================================
// Shader program
// ============================================================================

static GLuint gProgram = 0;
static GLuint gVAO = 0;
static GLuint gStreamVBO = 0;
static GLuint gStreamIBO = 0;

// Attribute locations
static GLint gAttribPos = 0;
static GLint gAttribNormal = 1;
static GLint gAttribTexcoord = 2;
static GLint gAttribColor = 3;

// Uniform locations
static GLint gUniMVP = -1;
static GLint gUniMV = -1;
static GLint gUniTexMatrix = -1;
static GLint gUniLightingEnabled = -1;
static GLint gUniNormalizeEnabled = -1;
static GLint gUniLightPos[MAX_LIGHTS];
static GLint gUniLightAmbient[MAX_LIGHTS];
static GLint gUniLightDiffuse[MAX_LIGHTS];
static GLint gUniLightSpecular[MAX_LIGHTS];
static GLint gUniLightEnabled[MAX_LIGHTS];
static GLint gUniAmbientLight = -1;
static GLint gUniMatAmbient = -1;
static GLint gUniMatDiffuse = -1;
static GLint gUniMatSpecular = -1;
static GLint gUniMatEmission = -1;
static GLint gUniMatShininess = -1;
static GLint gUniColorMaterialEnabled = -1;
static GLint gUniFogEnabled = -1;
static GLint gUniFogMode = -1;
static GLint gUniFogColor = -1;
static GLint gUniFogStart = -1;
static GLint gUniFogEnd = -1;
static GLint gUniFogDensity = -1;
static GLint gUniAlphaTestEnabled = -1;
static GLint gUniAlphaFunc = -1;
static GLint gUniAlphaRef = -1;
static GLint gUniTexture2DEnabled = -1;
static GLint gUniTexEnvMode = -1;
static GLint gUniTexGenEnabled = -1;
static GLint gUniTexture0 = -1;
static GLint gUniCurrentColor = -1;

static const char* kVertexShaderSrc =
    "#version 300 es\n"
    "precision highp float;\n"
    "\n"
    "layout(location = 0) in vec3 a_position;\n"
    "layout(location = 1) in vec3 a_normal;\n"
    "layout(location = 2) in vec2 a_texcoord;\n"
    "layout(location = 3) in vec4 a_color;\n"
    "\n"
    "uniform mat4 u_mvp;\n"
    "uniform mat4 u_mv;\n"
    "uniform mat4 u_texMatrix;\n"
    "uniform bool u_lightingEnabled;\n"
    "uniform bool u_normalizeEnabled;\n"
    "uniform bool u_texGenEnabled;\n"
    "uniform bool u_colorMaterialEnabled;\n"
    "\n"
    "struct Light {\n"
    "    vec4 position;\n"
    "    vec4 ambient;\n"
    "    vec4 diffuse;\n"
    "    vec4 specular;\n"
    "    bool enabled;\n"
    "};\n"
    "uniform Light u_lights[4];\n"
    "uniform vec4 u_ambientLight;\n"
    "uniform vec4 u_matAmbient;\n"
    "uniform vec4 u_matDiffuse;\n"
    "uniform vec4 u_matSpecular;\n"
    "uniform vec4 u_matEmission;\n"
    "uniform float u_matShininess;\n"
    "uniform vec4 u_currentColor;\n"
    "\n"
    "out vec4 v_color;\n"
    "out vec2 v_texcoord;\n"
    "\n"
    "void main() {\n"
    "    gl_Position = u_mvp * vec4(a_position, 1.0);\n"
    "\n"
    "    if (u_texGenEnabled) {\n"
    "        // Sphere map\n"
    "        vec3 eyePos = (u_mv * vec4(a_position, 1.0)).xyz;\n"
    "        vec3 n = a_normal;\n"
    "        if (u_normalizeEnabled) n = normalize(n);\n"
    "        vec3 eyeNormal = normalize(mat3(u_mv) * n);\n"
    "        vec3 r = reflect(normalize(eyePos), eyeNormal);\n"
    "        float m = 2.0 * sqrt(r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0));\n"
    "        v_texcoord = vec2(r.x/m + 0.5, r.y/m + 0.5);\n"
    "    } else {\n"
    "        v_texcoord = (u_texMatrix * vec4(a_texcoord, 0.0, 1.0)).xy;\n"
    "    }\n"
    "\n"
    "    if (u_lightingEnabled) {\n"
    "        vec4 matAmb = u_colorMaterialEnabled ? a_color : u_matAmbient;\n"
    "        vec4 matDif = u_colorMaterialEnabled ? a_color : u_matDiffuse;\n"
    "\n"
    "        vec3 n = a_normal;\n"
    "        if (u_normalizeEnabled) n = normalize(n);\n"
    "        vec3 eyeNormal = normalize(mat3(u_mv) * n);\n"
    "        vec3 eyePos = (u_mv * vec4(a_position, 1.0)).xyz;\n"
    "\n"
    "        vec4 color = u_matEmission + u_ambientLight * matAmb;\n"
    "        for (int i = 0; i < 4; i++) {\n"
    "            if (!u_lights[i].enabled) continue;\n"
    "            vec3 lightDir;\n"
    "            if (u_lights[i].position.w == 0.0) {\n"
    "                lightDir = normalize(u_lights[i].position.xyz);\n"
    "            } else {\n"
    "                lightDir = normalize(u_lights[i].position.xyz - eyePos);\n"
    "            }\n"
    "            float diff = max(dot(eyeNormal, lightDir), 0.0);\n"
    "            color += u_lights[i].ambient * matAmb;\n"
    "            color += u_lights[i].diffuse * matDif * diff;\n"
    "            if (u_matShininess > 0.0 && diff > 0.0) {\n"
    "                vec3 halfVec = normalize(lightDir - normalize(eyePos));\n"
    "                float spec = pow(max(dot(eyeNormal, halfVec), 0.0), u_matShininess);\n"
    "                color += u_lights[i].specular * u_matSpecular * spec;\n"
    "            }\n"
    "        }\n"
    "        v_color = clamp(color, 0.0, 1.0);\n"
    "        v_color.a = matDif.a;\n"
    "    } else {\n"
    "        v_color = u_colorMaterialEnabled ? a_color : u_currentColor;\n"
    "    }\n"
    "}\n";

static const char* kFragmentShaderSrc =
    "#version 300 es\n"
    "precision highp float;\n"
    "\n"
    "in vec4 v_color;\n"
    "in vec2 v_texcoord;\n"
    "\n"
    "uniform sampler2D u_texture0;\n"
    "uniform bool u_texture2DEnabled;\n"
    "uniform int  u_texEnvMode;\n"
    "uniform bool u_alphaTestEnabled;\n"
    "uniform int  u_alphaFunc;\n"
    "uniform float u_alphaRef;\n"
    "uniform bool u_fogEnabled;\n"
    "uniform int  u_fogMode;\n"
    "uniform vec4 u_fogColor;\n"
    "uniform float u_fogStart;\n"
    "uniform float u_fogEnd;\n"
    "uniform float u_fogDensity;\n"
    "\n"
    "out vec4 fragColor;\n"
    "\n"
    "void main() {\n"
    "    vec4 color = v_color;\n"
    "\n"
    "    if (u_texture2DEnabled) {\n"
    "        vec4 texColor = texture(u_texture0, v_texcoord);\n"
    "        if (u_texEnvMode == 0x2100) {\n"         // GL_MODULATE
    "            color *= texColor;\n"
    "        } else if (u_texEnvMode == 0x1E01) {\n"  // GL_REPLACE
    "            color = texColor;\n"
    "        } else if (u_texEnvMode == 0x2101) {\n"  // GL_DECAL
    "            color.rgb = mix(color.rgb, texColor.rgb, texColor.a);\n"
    "        } else if (u_texEnvMode == 0x0104) {\n"  // GL_ADD
    "            color.rgb += texColor.rgb;\n"
    "            color.a *= texColor.a;\n"
    "        } else {\n"
    "            color *= texColor;\n"
    "        }\n"
    "    }\n"
    "\n"
    "    if (u_alphaTestEnabled) {\n"
    "        float a = color.a;\n"
    "        float ref = u_alphaRef;\n"
    "        if (u_alphaFunc == 0x0200) discard;\n"                        // GL_NEVER
    "        else if (u_alphaFunc == 0x0201 && !(a < ref)) discard;\n"    // GL_LESS
    "        else if (u_alphaFunc == 0x0202 && !(a == ref)) discard;\n"   // GL_EQUAL
    "        else if (u_alphaFunc == 0x0203 && !(a <= ref)) discard;\n"   // GL_LEQUAL
    "        else if (u_alphaFunc == 0x0204 && !(a > ref)) discard;\n"    // GL_GREATER
    "        else if (u_alphaFunc == 0x0205 && !(a != ref)) discard;\n"   // GL_NOTEQUAL
    "        else if (u_alphaFunc == 0x0206 && !(a >= ref)) discard;\n"   // GL_GEQUAL
    "    }\n"
    "\n"
    "    if (u_fogEnabled) {\n"
    "        float depth = gl_FragCoord.z / gl_FragCoord.w;\n"
    "        float fogFactor;\n"
    "        if (u_fogMode == 0x2601) {\n"  // GL_LINEAR
    "            fogFactor = clamp((u_fogEnd - depth) / (u_fogEnd - u_fogStart), 0.0, 1.0);\n"
    "        } else if (u_fogMode == 0x0800) {\n"  // GL_EXP
    "            fogFactor = clamp(exp(-u_fogDensity * depth), 0.0, 1.0);\n"
    "        } else {\n"  // GL_EXP2
    "            fogFactor = clamp(exp(-(u_fogDensity * depth) * (u_fogDensity * depth)), 0.0, 1.0);\n"
    "        }\n"
    "        color.rgb = mix(u_fogColor.rgb, color.rgb, fogFactor);\n"
    "    }\n"
    "\n"
    "    fragColor = color;\n"
    "}\n";

// ============================================================================
// Immediate mode vertex buffer
// ============================================================================

#define IM_MAX_VERTS 8192

typedef struct {
    float x, y, z;
    float nx, ny, nz;
    float s, t;
    float r, g, b, a;
} BridgeVertex;

static BridgeVertex gImVerts[IM_MAX_VERTS];
static int          gImVertCount = 0;
static GLenum       gImPrimMode = GL_TRIANGLES;

// ============================================================================
// Client array state (for glVertexPointer etc.)
// ============================================================================

typedef struct {
    int      enabled;
    GLint    size;
    GLenum   type;
    GLsizei  stride;
    const void *pointer;
} ClientArrayState;

static ClientArrayState gVertexArray;
static ClientArrayState gNormalArray;
static ClientArrayState gTexCoordArray;
static ClientArrayState gColorArray;

// ============================================================================
// Helpers
// ============================================================================

static GLuint CompileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        BRIDGE_LOGE("Shader compile error: %s", log);
    }
    return shader;
}

static GLuint LinkProgram(GLuint vs, GLuint fs) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint status;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (!status) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), NULL, log);
        BRIDGE_LOGE("Program link error: %s", log);
    }
    return prog;
}

static void ComputeMVP(void) {
    Mat4_Multiply(gState.mvp, gState.projection, gState.modelview);
}

// ============================================================================
// Initialization / Shutdown
// ============================================================================

void GLESBridge_Init(void) {
    memset(&gState, 0, sizeof(gState));

    // Init matrix stacks
    Mat4_Identity(gMVStack.stack[0]);
    Mat4_Identity(gProjStack.stack[0]);
    Mat4_Identity(gTexStack.stack[0]);
    gMVStack.top = gProjStack.top = gTexStack.top = 0;

    // Init default state
    gState.currentColor[0] = gState.currentColor[1] = gState.currentColor[2] = gState.currentColor[3] = 1.0f;
    gState.materialAmbient[0] = gState.materialAmbient[1] = gState.materialAmbient[2] = 0.2f;
    gState.materialAmbient[3] = 1.0f;
    gState.materialDiffuse[0] = gState.materialDiffuse[1] = gState.materialDiffuse[2] = 0.8f;
    gState.materialDiffuse[3] = 1.0f;
    gState.materialShininess = 0.0f;
    gState.ambientLight[0] = gState.ambientLight[1] = gState.ambientLight[2] = 0.2f;
    gState.ambientLight[3] = 1.0f;
    gState.fogStart = 0.0f;
    gState.fogEnd = 1.0f;
    gState.fogDensity = 1.0f;
    gState.fogMode = GL_EXP;
    gState.texEnvMode = GL_MODULATE;
    for (int i = 0; i < MAX_LIGHTS; i++) {
        gState.lights[i].diffuse[0] = gState.lights[i].diffuse[1] = gState.lights[i].diffuse[2] = 1.0f;
        gState.lights[i].diffuse[3] = 1.0f;
        gState.lights[i].specular[0] = gState.lights[i].specular[1] = gState.lights[i].specular[2] = 1.0f;
        gState.lights[i].specular[3] = 1.0f;
        gState.lights[i].pos[3] = 1.0f; // positional by default
    }

    // Compile shaders
    GLuint vs = CompileShader(GL_VERTEX_SHADER, kVertexShaderSrc);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, kFragmentShaderSrc);
    gProgram = LinkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Get uniform locations
    gUniMVP              = glGetUniformLocation(gProgram, "u_mvp");
    gUniMV               = glGetUniformLocation(gProgram, "u_mv");
    gUniTexMatrix        = glGetUniformLocation(gProgram, "u_texMatrix");
    gUniLightingEnabled  = glGetUniformLocation(gProgram, "u_lightingEnabled");
    gUniNormalizeEnabled = glGetUniformLocation(gProgram, "u_normalizeEnabled");
    gUniColorMaterialEnabled = glGetUniformLocation(gProgram, "u_colorMaterialEnabled");
    gUniTexGenEnabled    = glGetUniformLocation(gProgram, "u_texGenEnabled");
    gUniAmbientLight     = glGetUniformLocation(gProgram, "u_ambientLight");
    gUniMatAmbient       = glGetUniformLocation(gProgram, "u_matAmbient");
    gUniMatDiffuse       = glGetUniformLocation(gProgram, "u_matDiffuse");
    gUniMatSpecular      = glGetUniformLocation(gProgram, "u_matSpecular");
    gUniMatEmission      = glGetUniformLocation(gProgram, "u_matEmission");
    gUniMatShininess     = glGetUniformLocation(gProgram, "u_matShininess");
    gUniFogEnabled       = glGetUniformLocation(gProgram, "u_fogEnabled");
    gUniFogMode          = glGetUniformLocation(gProgram, "u_fogMode");
    gUniFogColor         = glGetUniformLocation(gProgram, "u_fogColor");
    gUniFogStart         = glGetUniformLocation(gProgram, "u_fogStart");
    gUniFogEnd           = glGetUniformLocation(gProgram, "u_fogEnd");
    gUniFogDensity       = glGetUniformLocation(gProgram, "u_fogDensity");
    gUniAlphaTestEnabled = glGetUniformLocation(gProgram, "u_alphaTestEnabled");
    gUniAlphaFunc        = glGetUniformLocation(gProgram, "u_alphaFunc");
    gUniAlphaRef         = glGetUniformLocation(gProgram, "u_alphaRef");
    gUniTexture2DEnabled = glGetUniformLocation(gProgram, "u_texture2DEnabled");
    gUniTexEnvMode       = glGetUniformLocation(gProgram, "u_texEnvMode");
    gUniTexture0         = glGetUniformLocation(gProgram, "u_texture0");
    gUniCurrentColor     = glGetUniformLocation(gProgram, "u_currentColor");

    char lightName[64];
    for (int i = 0; i < MAX_LIGHTS; i++) {
        snprintf(lightName, sizeof(lightName), "u_lights[%d].position", i);
        gUniLightPos[i]     = glGetUniformLocation(gProgram, lightName);
        snprintf(lightName, sizeof(lightName), "u_lights[%d].ambient", i);
        gUniLightAmbient[i] = glGetUniformLocation(gProgram, lightName);
        snprintf(lightName, sizeof(lightName), "u_lights[%d].diffuse", i);
        gUniLightDiffuse[i] = glGetUniformLocation(gProgram, lightName);
        snprintf(lightName, sizeof(lightName), "u_lights[%d].specular", i);
        gUniLightSpecular[i]= glGetUniformLocation(gProgram, lightName);
        snprintf(lightName, sizeof(lightName), "u_lights[%d].enabled", i);
        gUniLightEnabled[i] = glGetUniformLocation(gProgram, lightName);
    }

    // Create VAO and streaming VBO/IBO
    glGenVertexArrays(1, &gVAO);
    glBindVertexArray(gVAO);

    glGenBuffers(1, &gStreamVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gStreamVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(BridgeVertex) * IM_MAX_VERTS, NULL, GL_STREAM_DRAW);

    glGenBuffers(1, &gStreamIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gStreamIBO);

    // Set up vertex attribute pointers
    glVertexAttribPointer(gAttribPos,      3, GL_FLOAT, GL_FALSE, sizeof(BridgeVertex), (void*)offsetof(BridgeVertex, x));
    glVertexAttribPointer(gAttribNormal,   3, GL_FLOAT, GL_FALSE, sizeof(BridgeVertex), (void*)offsetof(BridgeVertex, nx));
    glVertexAttribPointer(gAttribTexcoord, 2, GL_FLOAT, GL_FALSE, sizeof(BridgeVertex), (void*)offsetof(BridgeVertex, s));
    glVertexAttribPointer(gAttribColor,    4, GL_FLOAT, GL_FALSE, sizeof(BridgeVertex), (void*)offsetof(BridgeVertex, r));

    glEnableVertexAttribArray(gAttribPos);
    glEnableVertexAttribArray(gAttribNormal);
    glEnableVertexAttribArray(gAttribTexcoord);
    glEnableVertexAttribArray(gAttribColor);

    glBindVertexArray(0);

    glUseProgram(gProgram);
    glUniform1i(gUniTexture0, 0);

    gState.dirty = 1;

    BRIDGE_LOGI("GLESBridge initialized");
}

void GLESBridge_Shutdown(void) {
    if (gProgram)    { glDeleteProgram(gProgram);          gProgram    = 0; }
    if (gStreamVBO)  { glDeleteBuffers(1, &gStreamVBO);    gStreamVBO  = 0; }
    if (gStreamIBO)  { glDeleteBuffers(1, &gStreamIBO);    gStreamIBO  = 0; }
    if (gVAO)        { glDeleteVertexArrays(1, &gVAO);     gVAO        = 0; }
}

// ============================================================================
// Upload uniforms to shader
// ============================================================================

void bridge_FlushState(void) {
    if (!gState.dirty) return;
    gState.dirty = 0;

    glUseProgram(gProgram);

    // Matrices
    Mat4_Copy(gState.modelview,  gMVStack.stack[gMVStack.top]);
    Mat4_Copy(gState.projection, gProjStack.stack[gProjStack.top]);
    Mat4_Copy(gState.texture,    gTexStack.stack[gTexStack.top]);
    ComputeMVP();

    glUniformMatrix4fv(gUniMVP,       1, GL_FALSE, gState.mvp);
    glUniformMatrix4fv(gUniMV,        1, GL_FALSE, gState.modelview);
    glUniformMatrix4fv(gUniTexMatrix, 1, GL_FALSE, gState.texture);

    // Lighting
    glUniform1i(gUniLightingEnabled,      gState.lightingEnabled);
    glUniform1i(gUniNormalizeEnabled,     gState.normalizeEnabled);
    glUniform1i(gUniColorMaterialEnabled, gState.colorMaterialEnabled);
    glUniform1i(gUniTexGenEnabled,        gState.texGenEnabled);
    glUniform4fv(gUniAmbientLight, 1, gState.ambientLight);
    glUniform4fv(gUniMatAmbient,   1, gState.materialAmbient);
    glUniform4fv(gUniMatDiffuse,   1, gState.materialDiffuse);
    glUniform4fv(gUniMatSpecular,  1, gState.materialSpecular);
    glUniform4fv(gUniMatEmission,  1, gState.materialEmission);
    glUniform1f(gUniMatShininess,     gState.materialShininess);
    glUniform4fv(gUniCurrentColor, 1, gState.currentColor);

    for (int i = 0; i < MAX_LIGHTS; i++) {
        glUniform4fv(gUniLightPos[i],      1, gState.lights[i].pos);
        glUniform4fv(gUniLightAmbient[i],  1, gState.lights[i].ambient);
        glUniform4fv(gUniLightDiffuse[i],  1, gState.lights[i].diffuse);
        glUniform4fv(gUniLightSpecular[i], 1, gState.lights[i].specular);
        glUniform1i(gUniLightEnabled[i],      gState.lights[i].enabled);
    }

    // Fog
    glUniform1i(gUniFogEnabled, gState.fogEnabled);
    glUniform1i(gUniFogMode,    gState.fogMode);
    glUniform4fv(gUniFogColor,  1, gState.fogColor);
    glUniform1f(gUniFogStart,   gState.fogStart);
    glUniform1f(gUniFogEnd,     gState.fogEnd);
    glUniform1f(gUniFogDensity, gState.fogDensity);

    // Alpha test
    glUniform1i(gUniAlphaTestEnabled, gState.alphaTestEnabled);
    glUniform1i(gUniAlphaFunc,        (int)gState.alphaFunc);
    glUniform1f(gUniAlphaRef,         gState.alphaRef);

    // Texture
    glUniform1i(gUniTexture2DEnabled, gState.texture2DEnabled);
    glUniform1i(gUniTexEnvMode,       gState.texEnvMode);
}

// ============================================================================
// Matrix operations
// ============================================================================

void bridge_MatrixMode(GLenum mode) {
    gCurrentMatrixMode = mode;
}

void bridge_PushMatrix(void) {
    MatrixStack *s = CurrentStack();
    if (s->top < MATRIX_STACK_DEPTH - 1) {
        Mat4_Copy(s->stack[s->top + 1], s->stack[s->top]);
        s->top++;
    }
    gState.dirty = 1;
}

void bridge_PopMatrix(void) {
    MatrixStack *s = CurrentStack();
    if (s->top > 0) s->top--;
    gState.dirty = 1;
}

void bridge_LoadIdentity(void) {
    Mat4_Identity(*CurrentTop());
    gState.dirty = 1;
}

void bridge_LoadMatrixf(const GLfloat *m) {
    Mat4_Copy(*CurrentTop(), m);
    gState.dirty = 1;
}

void bridge_MultMatrixf(const GLfloat *m) {
    Mat4 tmp;
    Mat4_Multiply(tmp, *CurrentTop(), m);
    Mat4_Copy(*CurrentTop(), tmp);
    gState.dirty = 1;
}

void bridge_Translatef(GLfloat x, GLfloat y, GLfloat z) {
    Mat4 t;
    Mat4_Identity(t);
    t[12] = x; t[13] = y; t[14] = z;
    bridge_MultMatrixf(t);
}

void bridge_Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    float rad = angle * (float)(3.14159265358979323846 / 180.0);
    float c = cosf(rad), s = sinf(rad);
    float len = sqrtf(x*x + y*y + z*z);
    if (len > 0.0f) { x /= len; y /= len; z /= len; }
    Mat4 r;
    r[ 0] = c + x*x*(1-c);   r[ 4] = x*y*(1-c) - z*s; r[ 8] = x*z*(1-c) + y*s; r[12] = 0;
    r[ 1] = y*x*(1-c) + z*s; r[ 5] = c + y*y*(1-c);   r[ 9] = y*z*(1-c) - x*s; r[13] = 0;
    r[ 2] = z*x*(1-c) - y*s; r[ 6] = z*y*(1-c) + x*s; r[10] = c + z*z*(1-c);   r[14] = 0;
    r[ 3] = 0;                r[ 7] = 0;                r[11] = 0;                r[15] = 1;
    bridge_MultMatrixf(r);
}

void bridge_Scalef(GLfloat x, GLfloat y, GLfloat z) {
    Mat4 s;
    Mat4_Identity(s);
    s[0] = x; s[5] = y; s[10] = z;
    bridge_MultMatrixf(s);
}

void bridge_Ortho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) {
    Mat4 m;
    memset(m, 0, sizeof(m));
    m[ 0] = (float)(2.0/(r-l));
    m[ 5] = (float)(2.0/(t-b));
    m[10] = (float)(-2.0/(f-n));
    m[12] = (float)(-(r+l)/(r-l));
    m[13] = (float)(-(t+b)/(t-b));
    m[14] = (float)(-(f+n)/(f-n));
    m[15] = 1.0f;
    bridge_MultMatrixf(m);
}

void bridge_Frustum(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) {
    Mat4 m;
    memset(m, 0, sizeof(m));
    m[ 0] = (float)(2.0*n/(r-l));
    m[ 5] = (float)(2.0*n/(t-b));
    m[ 8] = (float)((r+l)/(r-l));
    m[ 9] = (float)((t+b)/(t-b));
    m[10] = (float)(-(f+n)/(f-n));
    m[11] = -1.0f;
    m[14] = (float)(-2.0*f*n/(f-n));
    bridge_MultMatrixf(m);
}

// ============================================================================
// Immediate mode
// ============================================================================

void bridge_Begin(GLenum mode) {
    gImVertCount = 0;
    gImPrimMode = mode;
}

static void PushVert(float x, float y, float z) {
    if (gImVertCount >= IM_MAX_VERTS) {
        BRIDGE_LOGE("bridge_Begin: vertex buffer overflow");
        return;
    }
    BridgeVertex *v = &gImVerts[gImVertCount++];
    v->x  = x;  v->y  = y;  v->z  = z;
    v->nx = gState.currentNormal[0];
    v->ny = gState.currentNormal[1];
    v->nz = gState.currentNormal[2];
    v->s  = gState.currentTexcoord[0];
    v->t  = gState.currentTexcoord[1];
    v->r  = gState.currentColor[0];
    v->g  = gState.currentColor[1];
    v->b  = gState.currentColor[2];
    v->a  = gState.currentColor[3];
}

void bridge_End(void) {
    if (gImVertCount == 0) return;

    bridge_FlushState();
    glUseProgram(gProgram);
    glBindVertexArray(gVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gStreamVBO);

    GLenum drawMode = gImPrimMode;
    int vertCount = gImVertCount;

    // Convert GL_QUADS (unsupported in GLES3) to triangles
    if (gImPrimMode == GL_QUADS) {
        int quadCount = gImVertCount / 4;
        int triVertCount = quadCount * 6;
        BridgeVertex *triVerts = (BridgeVertex*)malloc(triVertCount * sizeof(BridgeVertex));
        if (!triVerts) return;
        for (int i = 0; i < quadCount; i++) {
            BridgeVertex *q = &gImVerts[i * 4];
            triVerts[i*6+0] = q[0]; triVerts[i*6+1] = q[1]; triVerts[i*6+2] = q[2];
            triVerts[i*6+3] = q[0]; triVerts[i*6+4] = q[2]; triVerts[i*6+5] = q[3];
        }
        glBufferData(GL_ARRAY_BUFFER, triVertCount * sizeof(BridgeVertex), triVerts, GL_STREAM_DRAW);
        free(triVerts);
        drawMode = GL_TRIANGLES;
        vertCount = triVertCount;
    } else {
        glBufferData(GL_ARRAY_BUFFER, gImVertCount * sizeof(BridgeVertex), gImVerts, GL_STREAM_DRAW);
    }

    glVertexAttribPointer(gAttribPos,      3, GL_FLOAT, GL_FALSE, sizeof(BridgeVertex), (void*)offsetof(BridgeVertex, x));
    glVertexAttribPointer(gAttribNormal,   3, GL_FLOAT, GL_FALSE, sizeof(BridgeVertex), (void*)offsetof(BridgeVertex, nx));
    glVertexAttribPointer(gAttribTexcoord, 2, GL_FLOAT, GL_FALSE, sizeof(BridgeVertex), (void*)offsetof(BridgeVertex, s));
    glVertexAttribPointer(gAttribColor,    4, GL_FLOAT, GL_FALSE, sizeof(BridgeVertex), (void*)offsetof(BridgeVertex, r));

    glEnableVertexAttribArray(gAttribPos);
    glEnableVertexAttribArray(gAttribNormal);
    glEnableVertexAttribArray(gAttribTexcoord);
    glEnableVertexAttribArray(gAttribColor);

    glDrawArrays(drawMode, 0, vertCount);
    glBindVertexArray(0);
    gImVertCount = 0;
}

void bridge_Vertex2f(GLfloat x, GLfloat y)              { PushVert(x, y, 0.0f); }
void bridge_Vertex3f(GLfloat x, GLfloat y, GLfloat z)   { PushVert(x, y, z); }
void bridge_Vertex3fv(const GLfloat *v)                  { PushVert(v[0], v[1], v[2]); }

void bridge_Normal3f(GLfloat x, GLfloat y, GLfloat z) {
    gState.currentNormal[0] = x;
    gState.currentNormal[1] = y;
    gState.currentNormal[2] = z;
}

void bridge_Normal3fv(const GLfloat *v) {
    gState.currentNormal[0] = v[0];
    gState.currentNormal[1] = v[1];
    gState.currentNormal[2] = v[2];
}

void bridge_TexCoord2f(GLfloat s, GLfloat t) {
    gState.currentTexcoord[0] = s;
    gState.currentTexcoord[1] = t;
}

void bridge_TexCoord2fv(const GLfloat *v) {
    gState.currentTexcoord[0] = v[0];
    gState.currentTexcoord[1] = v[1];
}

void bridge_Color3f(GLfloat r, GLfloat g, GLfloat b) {
    gState.currentColor[0] = r;
    gState.currentColor[1] = g;
    gState.currentColor[2] = b;
    gState.currentColor[3] = 1.0f;
    gState.dirty = 1;
}

void bridge_Color4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    gState.currentColor[0] = r;
    gState.currentColor[1] = g;
    gState.currentColor[2] = b;
    gState.currentColor[3] = a;
    gState.dirty = 1;
}

void bridge_Color4fv(const GLfloat *v) {
    gState.currentColor[0] = v[0];
    gState.currentColor[1] = v[1];
    gState.currentColor[2] = v[2];
    gState.currentColor[3] = v[3];
    gState.dirty = 1;
}

void bridge_Color4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a) {
    gState.currentColor[0] = r / 255.0f;
    gState.currentColor[1] = g / 255.0f;
    gState.currentColor[2] = b / 255.0f;
    gState.currentColor[3] = a / 255.0f;
    gState.dirty = 1;
}

// ============================================================================
// Lighting
// ============================================================================

void bridge_Lightfv(GLenum light, GLenum pname, const GLfloat *params) {
    int idx = (int)(light - GL_LIGHT0);
    if (idx < 0 || idx >= MAX_LIGHTS) return;
    switch (pname) {
        case GL_POSITION:
            // Transform position to eye space using current modelview
            {
                Mat4 *mv = &gMVStack.stack[gMVStack.top];
                float *p = gState.lights[idx].pos;
                float w = params[3];
                p[0] = (*mv)[0]*params[0] + (*mv)[4]*params[1] + (*mv)[ 8]*params[2] + (*mv)[12]*w;
                p[1] = (*mv)[1]*params[0] + (*mv)[5]*params[1] + (*mv)[ 9]*params[2] + (*mv)[13]*w;
                p[2] = (*mv)[2]*params[0] + (*mv)[6]*params[1] + (*mv)[10]*params[2] + (*mv)[14]*w;
                p[3] = w;
            }
            break;
        case GL_AMBIENT:  memcpy(gState.lights[idx].ambient,  params, 4*sizeof(float)); break;
        case GL_DIFFUSE:  memcpy(gState.lights[idx].diffuse,  params, 4*sizeof(float)); break;
        case GL_SPECULAR: memcpy(gState.lights[idx].specular, params, 4*sizeof(float)); break;
    }
    gState.dirty = 1;
}

void bridge_Lightf(GLenum light, GLenum pname, GLfloat param) {
    GLfloat v[4] = {param, 0, 0, 0};
    bridge_Lightfv(light, pname, v);
}

void bridge_LightModelfv(GLenum pname, const GLfloat *params) {
    if (pname == 0x0B53 /*GL_LIGHT_MODEL_AMBIENT*/) {
        memcpy(gState.ambientLight, params, 4*sizeof(float));
        gState.dirty = 1;
    }
}

void bridge_LightModeli(GLenum pname, GLint param) {
    // GL_LIGHT_MODEL_TWO_SIDE (0x0B52) - not emulated in the bridge, silently ignored
    (void)pname; (void)param;
}

void bridge_Materialfv(GLenum face, GLenum pname, const GLfloat *params) {
    (void)face;
    switch (pname) {
        case GL_AMBIENT:
            memcpy(gState.materialAmbient, params, 4*sizeof(float));
            break;
        case GL_DIFFUSE:
            memcpy(gState.materialDiffuse, params, 4*sizeof(float));
            break;
        case GL_SPECULAR:
            memcpy(gState.materialSpecular, params, 4*sizeof(float));
            break;
        case GL_EMISSION:
            memcpy(gState.materialEmission, params, 4*sizeof(float));
            break;
        case GL_AMBIENT_AND_DIFFUSE:
            memcpy(gState.materialAmbient, params, 4*sizeof(float));
            memcpy(gState.materialDiffuse, params, 4*sizeof(float));
            break;
        case GL_SHININESS:
            gState.materialShininess = params[0];
            break;
    }
    gState.dirty = 1;
}

void bridge_Materialf(GLenum face, GLenum pname, GLfloat param) {
    GLfloat v[4] = {param, 0, 0, 0};
    bridge_Materialfv(face, pname, v);
}

// ============================================================================
// Fog
// ============================================================================

void bridge_Fogfv(GLenum pname, const GLfloat *params) {
    switch (pname) {
        case GL_FOG_COLOR:   memcpy(gState.fogColor, params, 4*sizeof(float)); break;
        case GL_FOG_START:   gState.fogStart   = params[0]; break;
        case GL_FOG_END:     gState.fogEnd     = params[0]; break;
        case GL_FOG_DENSITY: gState.fogDensity = params[0]; break;
    }
    gState.dirty = 1;
}

void bridge_Fogf(GLenum pname, GLfloat param) {
    GLfloat v[4] = {param, 0, 0, 0};
    bridge_Fogfv(pname, v);
}

void bridge_Fogi(GLenum pname, GLint param) {
    if (pname == GL_FOG_MODE) {
        gState.fogMode = (int)param;
        gState.dirty = 1;
    }
}

// ============================================================================
// Alpha test
// ============================================================================

void bridge_AlphaFunc(GLenum func, GLfloat ref) {
    gState.alphaFunc = func;
    gState.alphaRef  = ref;
    gState.dirty = 1;
}

// ============================================================================
// Texture environment
// ============================================================================

void bridge_TexEnvi(GLenum target, GLenum pname, GLint param) {
    (void)target;
    if (pname == GL_TEXTURE_ENV_MODE) {
        gState.texEnvMode = (int)param;
        gState.dirty = 1;
    }
}

void bridge_TexEnvfv(GLenum target, GLenum pname, const GLfloat *params) {
    (void)target; (void)pname; (void)params;
}

// ============================================================================
// Texture generation (sphere map)
// ============================================================================

void bridge_TexGeni(GLenum coord, GLenum pname, GLint param) {
    (void)coord;
    if (pname == GL_TEXTURE_GEN_MODE && param == GL_SPHERE_MAP) {
        gState.texGenEnabled = 1;
        gState.dirty = 1;
    }
}

// ============================================================================
// Enable / Disable state (intercept GLES-unsupported caps)
// ============================================================================

void bridge_Enable(GLenum cap) {
    switch (cap) {
        case GL_LIGHTING:
            gState.lightingEnabled = 1; gState.dirty = 1; return;
        case GL_LIGHT0: case GL_LIGHT1: case GL_LIGHT2: case GL_LIGHT3:
            gState.lights[cap - GL_LIGHT0].enabled = 1; gState.dirty = 1; return;
        case GL_FOG:
            gState.fogEnabled = 1; gState.dirty = 1; return;
        case GL_ALPHA_TEST:
            gState.alphaTestEnabled = 1; gState.dirty = 1; return;
        case GL_NORMALIZE:
            gState.normalizeEnabled = 1; gState.dirty = 1; return;
        case GL_COLOR_MATERIAL:
            gState.colorMaterialEnabled = 1; gState.dirty = 1; return;
        case GL_TEXTURE_GEN_S: case GL_TEXTURE_GEN_T:
            gState.texGenEnabled = 1; gState.dirty = 1; return;
        case GL_TEXTURE_2D:
            gState.texture2DEnabled = 1; gState.dirty = 1; return;
        // Client-state enums are not valid in GLES3 core glEnable — ignore them
        case GL_VERTEX_ARRAY: case GL_NORMAL_ARRAY:
        case GL_COLOR_ARRAY:  case GL_TEXTURE_COORD_ARRAY:
            return;
        default:
            glEnable(cap); return;
    }
}

void bridge_Disable(GLenum cap) {
    switch (cap) {
        case GL_LIGHTING:
            gState.lightingEnabled = 0; gState.dirty = 1; return;
        case GL_LIGHT0: case GL_LIGHT1: case GL_LIGHT2: case GL_LIGHT3:
            gState.lights[cap - GL_LIGHT0].enabled = 0; gState.dirty = 1; return;
        case GL_FOG:
            gState.fogEnabled = 0; gState.dirty = 1; return;
        case GL_ALPHA_TEST:
            gState.alphaTestEnabled = 0; gState.dirty = 1; return;
        case GL_NORMALIZE:
            gState.normalizeEnabled = 0; gState.dirty = 1; return;
        case GL_COLOR_MATERIAL:
            gState.colorMaterialEnabled = 0; gState.dirty = 1; return;
        case GL_TEXTURE_GEN_S: case GL_TEXTURE_GEN_T:
            gState.texGenEnabled = 0; gState.dirty = 1; return;
        case GL_TEXTURE_2D:
            gState.texture2DEnabled = 0; gState.dirty = 1; return;
        case GL_VERTEX_ARRAY: case GL_NORMAL_ARRAY:
        case GL_COLOR_ARRAY:  case GL_TEXTURE_COORD_ARRAY:
            return;
        default:
            glDisable(cap); return;
    }
}

// ============================================================================
// Client state (vertex arrays)
// ============================================================================

void bridge_EnableClientState(GLenum array) {
    switch (array) {
        case GL_VERTEX_ARRAY:        gVertexArray.enabled   = 1; break;
        case GL_NORMAL_ARRAY:        gNormalArray.enabled   = 1; break;
        case GL_TEXTURE_COORD_ARRAY: gTexCoordArray.enabled = 1; break;
        case GL_COLOR_ARRAY:         gColorArray.enabled    = 1; break;
    }
}

void bridge_DisableClientState(GLenum array) {
    switch (array) {
        case GL_VERTEX_ARRAY:        gVertexArray.enabled   = 0; break;
        case GL_NORMAL_ARRAY:        gNormalArray.enabled   = 0; break;
        case GL_TEXTURE_COORD_ARRAY: gTexCoordArray.enabled = 0; break;
        case GL_COLOR_ARRAY:         gColorArray.enabled    = 0; break;
    }
}

void bridge_VertexPointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {
    gVertexArray.size    = size;
    gVertexArray.type    = type;
    gVertexArray.stride  = stride;
    gVertexArray.pointer = pointer;
}

void bridge_NormalPointer(GLenum type, GLsizei stride, const void *pointer) {
    gNormalArray.size    = 3;
    gNormalArray.type    = type;
    gNormalArray.stride  = stride;
    gNormalArray.pointer = pointer;
}

void bridge_TexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {
    gTexCoordArray.size    = size;
    gTexCoordArray.type    = type;
    gTexCoordArray.stride  = stride;
    gTexCoordArray.pointer = pointer;
}

void bridge_ColorPointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {
    gColorArray.size    = size;
    gColorArray.type    = type;
    gColorArray.stride  = stride;
    gColorArray.pointer = pointer;
}

// Helper: read n floats from a generic vertex array at the given vertex index
static void GetVertFloat(const ClientArrayState *arr, int idx, float *out, int n) {
    const uint8_t *base = (const uint8_t*)arr->pointer;
    int byteSize = (arr->type == GL_FLOAT) ? 4 : 1;
    int stride = arr->stride ? arr->stride : (n * byteSize);
    base += (size_t)idx * (size_t)stride;
    if (arr->type == GL_FLOAT) {
        for (int i = 0; i < n; i++) out[i] = ((const float*)base)[i];
    } else if (arr->type == GL_UNSIGNED_BYTE) {
        for (int i = 0; i < n; i++) out[i] = base[i] / 255.0f;
    }
}

// ============================================================================
// Draw calls (with VBO upload)
// ============================================================================

static void DrawPrimitiveFromArrays(GLenum mode, int first, int count, GLenum idxType, const void *indices) {
    bridge_FlushState();
    glUseProgram(gProgram);
    glBindVertexArray(gVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gStreamVBO);

    // Build interleaved vertex buffer from client arrays
    BridgeVertex *verts = (BridgeVertex*)malloc((size_t)count * sizeof(BridgeVertex));
    if (!verts) return;

    for (int i = 0; i < count; i++) {
        int vidx;
        if (indices) {
            if (idxType == GL_UNSIGNED_SHORT)     vidx = ((const uint16_t*)indices)[i];
            else if (idxType == GL_UNSIGNED_INT)  vidx = (int)((const uint32_t*)indices)[i];
            else                                  vidx = ((const uint8_t*)indices)[i];
        } else {
            vidx = first + i;
        }

        BridgeVertex *v = &verts[i];
        memset(v, 0, sizeof(*v));
        v->r = gState.currentColor[0]; v->g = gState.currentColor[1];
        v->b = gState.currentColor[2]; v->a = gState.currentColor[3];

        if (gVertexArray.enabled && gVertexArray.pointer) {
            float pos[4] = {0, 0, 0, 1};
            GetVertFloat(&gVertexArray, vidx, pos, gVertexArray.size);
            v->x = pos[0]; v->y = pos[1]; v->z = pos[2];
        }
        if (gNormalArray.enabled && gNormalArray.pointer) {
            float n[3] = {0, 0, 1};
            GetVertFloat(&gNormalArray, vidx, n, 3);
            v->nx = n[0]; v->ny = n[1]; v->nz = n[2];
        }
        if (gTexCoordArray.enabled && gTexCoordArray.pointer) {
            float tc[2] = {0, 0};
            int tcSize = gTexCoordArray.size >= 2 ? 2 : gTexCoordArray.size;
            GetVertFloat(&gTexCoordArray, vidx, tc, tcSize);
            v->s = tc[0]; v->t = tc[1];
        }
        if (gColorArray.enabled && gColorArray.pointer) {
            float c[4] = {1, 1, 1, 1};
            GetVertFloat(&gColorArray, vidx, c, gColorArray.size);
            v->r = c[0]; v->g = c[1]; v->b = c[2]; v->a = c[3];
        }
    }

    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)((size_t)count * sizeof(BridgeVertex)), verts, GL_STREAM_DRAW);
    free(verts);

    glVertexAttribPointer(gAttribPos,      3, GL_FLOAT, GL_FALSE, sizeof(BridgeVertex), (void*)offsetof(BridgeVertex, x));
    glVertexAttribPointer(gAttribNormal,   3, GL_FLOAT, GL_FALSE, sizeof(BridgeVertex), (void*)offsetof(BridgeVertex, nx));
    glVertexAttribPointer(gAttribTexcoord, 2, GL_FLOAT, GL_FALSE, sizeof(BridgeVertex), (void*)offsetof(BridgeVertex, s));
    glVertexAttribPointer(gAttribColor,    4, GL_FLOAT, GL_FALSE, sizeof(BridgeVertex), (void*)offsetof(BridgeVertex, r));

    glEnableVertexAttribArray(gAttribPos);
    glEnableVertexAttribArray(gAttribNormal);
    glEnableVertexAttribArray(gAttribTexcoord);
    glEnableVertexAttribArray(gAttribColor);

    if (mode == GL_QUADS) {
        // Re-index quad vertices as triangle pairs
        int quadCount = count / 4;
        int triCount = quadCount * 6;
        uint16_t *idx = (uint16_t*)malloc((size_t)triCount * sizeof(uint16_t));
        if (!idx) return;
        for (int i = 0; i < quadCount; i++) {
            idx[i*6+0] = (uint16_t)(i*4+0); idx[i*6+1] = (uint16_t)(i*4+1); idx[i*6+2] = (uint16_t)(i*4+2);
            idx[i*6+3] = (uint16_t)(i*4+0); idx[i*6+4] = (uint16_t)(i*4+2); idx[i*6+5] = (uint16_t)(i*4+3);
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gStreamIBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)((size_t)triCount * sizeof(uint16_t)), idx, GL_STREAM_DRAW);
        free(idx);
        glDrawElements(GL_TRIANGLES, triCount, GL_UNSIGNED_SHORT, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    } else {
        glDrawArrays(mode, 0, count);
    }

    glBindVertexArray(0);
}

void bridge_DrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices) {
    DrawPrimitiveFromArrays(mode, 0, (int)count, type, indices);
}

void bridge_DrawArrays(GLenum mode, GLint first, GLsizei count) {
    DrawPrimitiveFromArrays(mode, (int)first, (int)count, 0, NULL);
}

// ============================================================================
// Color material
// ============================================================================

void bridge_ColorMaterial(GLenum face, GLenum mode) {
    (void)face; (void)mode;
    gState.colorMaterialEnabled = 1;
    gState.dirty = 1;
}

// ============================================================================
// Polygon mode (stub - not supported in GLES3)
// ============================================================================

void bridge_PolygonMode(GLenum face, GLenum mode) {
    (void)face; (void)mode;
    // Silently ignored — GLES3 has no polygon mode support
}

void bridge_GetFloatv(GLenum pname, GLfloat *params) {
    switch (pname) {
        case GL_PROJECTION_MATRIX: // 0x0BA7 — return from software stack
            memcpy(params, &gProjStack.stack[gProjStack.top], 16 * sizeof(GLfloat));
            break;
        case GL_MODELVIEW_MATRIX:  // 0x0BA6 — return from software stack
            memcpy(params, &gMVStack.stack[gMVStack.top], 16 * sizeof(GLfloat));
            break;
        case GL_CURRENT_COLOR:     // 0x0B00 — return from bridge state
            memcpy(params, gState.currentColor, 4 * sizeof(GLfloat));
            break;
        default:
            glGetFloatv(pname, params);
            break;
    }
}

// ============================================================================
// Texture format conversion
// ============================================================================

void bridge_TexImage2D(GLenum target, GLint level, GLint internalformat,
                       GLsizei width, GLsizei height, GLint border,
                       GLenum format, GLenum type, const void *pixels) {
    // Map legacy luminance formats to GLES3 equivalents
    if (internalformat == GL_LUMINANCE) {
        internalformat = GL_R8;
        format = GL_RED;
        type = GL_UNSIGNED_BYTE;
    } else if (internalformat == GL_LUMINANCE_ALPHA) {
        internalformat = GL_RG8;
        format = GL_RG;
        type = GL_UNSIGNED_BYTE;
    }

    // Convert BGRA → RGBA (GLES3 has no GL_BGRA as a base format)
    if (pixels && (format == GL_BGRA) &&
        (type == GL_UNSIGNED_INT_8_8_8_8_REV || type == GL_UNSIGNED_BYTE)) {
        uint8_t *dst = (uint8_t*)malloc((size_t)(width * height * 4));
        if (!dst) return;
        const uint8_t *src = (const uint8_t*)pixels;
        for (int i = 0; i < width * height; i++) {
            dst[i*4+0] = src[i*4+2]; // R ← B
            dst[i*4+1] = src[i*4+1]; // G
            dst[i*4+2] = src[i*4+0]; // B ← R
            dst[i*4+3] = src[i*4+3]; // A
        }
        glTexImage2D(target, level, GL_RGBA, width, height, border, GL_RGBA, GL_UNSIGNED_BYTE, dst);
        free(dst);
        return;
    }

    // Convert BGR → RGB
    if (pixels && format == GL_BGR) {
        uint8_t *dst = (uint8_t*)malloc((size_t)(width * height * 3));
        if (!dst) return;
        const uint8_t *src = (const uint8_t*)pixels;
        for (int i = 0; i < width * height; i++) {
            dst[i*3+0] = src[i*3+2];
            dst[i*3+1] = src[i*3+1];
            dst[i*3+2] = src[i*3+0];
        }
        glTexImage2D(target, level, GL_RGB, width, height, border, GL_RGB, GL_UNSIGNED_BYTE, dst);
        free(dst);
        return;
    }

    glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

void bridge_TexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                          GLsizei width, GLsizei height, GLenum format, GLenum type,
                          const void *pixels) {
    if (pixels && (format == GL_BGRA) &&
        (type == GL_UNSIGNED_INT_8_8_8_8_REV || type == GL_UNSIGNED_BYTE)) {
        GLint rowLength = 0;
        glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowLength);

        uint8_t *dst = (uint8_t*)malloc((size_t)(width * height * 4));
        if (!dst) return;
        const uint8_t *src = (const uint8_t*)pixels;
        int srcStride = rowLength > 0 ? rowLength * 4 : width * 4;
        for (int y = 0; y < height; y++) {
            const uint8_t *row = src + y * srcStride;
            uint8_t *dstRow = dst + y * width * 4;
            for (int x = 0; x < width; x++) {
                dstRow[x*4+0] = row[x*4+2];
                dstRow[x*4+1] = row[x*4+1];
                dstRow[x*4+2] = row[x*4+0];
                dstRow[x*4+3] = row[x*4+3];
            }
        }
        // Temporarily reset row length for our tightly-packed converted buffer
        if (rowLength > 0) glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexSubImage2D(target, level, xoffset, yoffset, width, height, GL_RGBA, GL_UNSIGNED_BYTE, dst);
        if (rowLength > 0) glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
        free(dst);
        return;
    }

    glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

#endif // __ANDROID__
