// TouchControls.c
// Touch control overlay for Billy Frontier Android port

#ifdef __ANDROID__

#include "game.h"
#include "touch_controls.h"
#include "main.h"
#include <GLES3/gl3.h>
#include <math.h>
#include <string.h>

#define TC_PI 3.14159265f

// ============================================================================
// Layout constants (in normalized 0..1 coordinates, then scaled to window)
// ============================================================================

// Joystick (left side)
#define JS_CX       0.12f   // center X (fraction of width)
#define JS_CY       0.72f   // center Y (fraction of height)
#define JS_RADIUS   0.10f   // radius (fraction of width)
#define JS_DEADZONE 0.15f   // dead zone fraction

// Right-side buttons
#define BTN_RADIUS  0.055f  // button radius (fraction of width)
#define BTN_SHOOT_X 0.90f
#define BTN_SHOOT_Y 0.60f
#define BTN_DUCK_X  0.82f
#define BTN_DUCK_Y  0.78f
#define BTN_JUMP_X  0.90f
#define BTN_JUMP_Y  0.60f

// D-pad (left side, used in menu mode)
#define DPAD_CX     0.12f
#define DPAD_CY     0.72f
#define DPAD_BTN_R  0.06f
#define DPAD_OFFSET 0.10f  // distance from center to each arrow

// Duel arrows (left/right sides)
#define DUEL_LEFT_X  0.12f
#define DUEL_LEFT_Y  0.75f
#define DUEL_RIGHT_X 0.88f
#define DUEL_RIGHT_Y 0.75f
#define DUEL_BTN_R   0.09f

// Pause button
#define PAUSE_X     0.50f
#define PAUSE_Y     0.05f
#define PAUSE_R     0.04f

// ============================================================================
// State
// ============================================================================

typedef struct {
    float cx, cy;       // center in pixels
    float radius;       // hit radius in pixels
    TouchButtonID btnId;
    bool  visible;
} TouchButton;

typedef struct {
    bool  pressed;
    bool  justPressed;
    bool  justReleased;
} BtnState;

static TouchControlScheme gScheme = TOUCH_SCHEME_MENU;
static BtnState   gBtnState[TOUCH_BTN_COUNT];

// Virtual joystick
static float gJsBaseX = 0, gJsBaseY = 0;   // base position (pixels)
static float gJsThumbX = 0, gJsThumbY = 0; // thumb position (pixels)
static float gJsRadius = 0;
static SDL_FingerID gJsFingerID = 0;
static bool  gJsActive = false;
static float gJoystickX = 0.0f, gJoystickY = 0.0f;

// Touch-to-aim
static float gAimX = 0, gAimY = 0;
static bool  gHasAimTouch = false;
static SDL_FingerID gAimFingerID = 0;
// Whether the aim touch also triggered a shoot press (rail-shooter tap-to-shoot)
static bool  gAimTriggeredShoot = false;

// Window size (set during draw or event processing)
static int gWinW = 640, gWinH = 480;

// Button layout (rebuilt on scheme change or window resize)
#define MAX_BUTTONS 12
static TouchButton gButtons[MAX_BUTTONS];
static int gButtonCount = 0;

// ============================================================================
// Helpers
// ============================================================================

static float ScaleX(float f) { return f * gWinW; }
static float ScaleY(float f) { return f * gWinH; }

static bool IsRightSideTouch(float x) {
    return x > gWinW * 0.5f;
}

static float Dist(float ax, float ay, float bx, float by) {
    float dx = ax - bx, dy = ay - by;
    return sqrtf(dx*dx + dy*dy);
}

static void SetPressed(TouchButtonID id, bool pressed) {
    if (!gBtnState[id].pressed && pressed)
        gBtnState[id].justPressed = true;
    if (gBtnState[id].pressed && !pressed)
        gBtnState[id].justReleased = true;
    gBtnState[id].pressed = pressed;
}

// ============================================================================
// Button layout rebuild
// ============================================================================

static void RebuildLayout(void) {
    gButtonCount = 0;

    // Always add pause button
    gButtons[gButtonCount++] = (TouchButton){
        .cx = ScaleX(PAUSE_X), .cy = ScaleY(PAUSE_Y),
        .radius = ScaleX(PAUSE_R), .btnId = TOUCH_BTN_PAUSE, .visible = true
    };

    switch (gScheme) {
        case TOUCH_SCHEME_MENU:
            // D-pad up/down/left/right + confirm + back
            gButtons[gButtonCount++] = (TouchButton){
                .cx = ScaleX(DPAD_CX), .cy = ScaleY(DPAD_CY - DPAD_OFFSET),
                .radius = ScaleX(DPAD_BTN_R), .btnId = TOUCH_BTN_DPAD_UP, .visible = true
            };
            gButtons[gButtonCount++] = (TouchButton){
                .cx = ScaleX(DPAD_CX), .cy = ScaleY(DPAD_CY + DPAD_OFFSET),
                .radius = ScaleX(DPAD_BTN_R), .btnId = TOUCH_BTN_DPAD_DOWN, .visible = true
            };
            gButtons[gButtonCount++] = (TouchButton){
                .cx = ScaleX(DPAD_CX - DPAD_OFFSET), .cy = ScaleY(DPAD_CY),
                .radius = ScaleX(DPAD_BTN_R), .btnId = TOUCH_BTN_DPAD_LEFT, .visible = true
            };
            gButtons[gButtonCount++] = (TouchButton){
                .cx = ScaleX(DPAD_CX + DPAD_OFFSET), .cy = ScaleY(DPAD_CY),
                .radius = ScaleX(DPAD_BTN_R), .btnId = TOUCH_BTN_DPAD_RIGHT, .visible = true
            };
            // Confirm (right side)
            gButtons[gButtonCount++] = (TouchButton){
                .cx = ScaleX(BTN_SHOOT_X), .cy = ScaleY(BTN_SHOOT_Y),
                .radius = ScaleX(BTN_RADIUS), .btnId = TOUCH_BTN_CONFIRM, .visible = true
            };
            // Back
            gButtons[gButtonCount++] = (TouchButton){
                .cx = ScaleX(BTN_DUCK_X), .cy = ScaleY(BTN_DUCK_Y),
                .radius = ScaleX(BTN_RADIUS), .btnId = TOUCH_BTN_BACK, .visible = true
            };
            break;

        case TOUCH_SCHEME_DUEL:
            // Full D-pad for the key sequence (up/down/left/right arrows)
            gButtons[gButtonCount++] = (TouchButton){
                .cx = ScaleX(DPAD_CX), .cy = ScaleY(DPAD_CY - DPAD_OFFSET),
                .radius = ScaleX(DPAD_BTN_R), .btnId = TOUCH_BTN_DPAD_UP, .visible = true
            };
            gButtons[gButtonCount++] = (TouchButton){
                .cx = ScaleX(DPAD_CX), .cy = ScaleY(DPAD_CY + DPAD_OFFSET),
                .radius = ScaleX(DPAD_BTN_R), .btnId = TOUCH_BTN_DPAD_DOWN, .visible = true
            };
            gButtons[gButtonCount++] = (TouchButton){
                .cx = ScaleX(DPAD_CX - DPAD_OFFSET), .cy = ScaleY(DPAD_CY),
                .radius = ScaleX(DPAD_BTN_R), .btnId = TOUCH_BTN_DPAD_LEFT, .visible = true
            };
            gButtons[gButtonCount++] = (TouchButton){
                .cx = ScaleX(DPAD_CX + DPAD_OFFSET), .cy = ScaleY(DPAD_CY),
                .radius = ScaleX(DPAD_BTN_R), .btnId = TOUCH_BTN_DPAD_RIGHT, .visible = true
            };
            // Shoot button (right side) for quick-draw
            gButtons[gButtonCount++] = (TouchButton){
                .cx = ScaleX(BTN_SHOOT_X), .cy = ScaleY(BTN_SHOOT_Y),
                .radius = ScaleX(BTN_RADIUS), .btnId = TOUCH_BTN_SHOOT, .visible = true
            };
            break;

        case TOUCH_SCHEME_SHOOTOUT:
            // Shoot button (right side)
            gButtons[gButtonCount++] = (TouchButton){
                .cx = ScaleX(BTN_SHOOT_X), .cy = ScaleY(BTN_SHOOT_Y),
                .radius = ScaleX(BTN_RADIUS), .btnId = TOUCH_BTN_SHOOT, .visible = true
            };
            // Duck button
            gButtons[gButtonCount++] = (TouchButton){
                .cx = ScaleX(BTN_DUCK_X), .cy = ScaleY(BTN_DUCK_Y),
                .radius = ScaleX(BTN_RADIUS), .btnId = TOUCH_BTN_DUCK, .visible = true
            };
            // Left/right for strafing
            gButtons[gButtonCount++] = (TouchButton){
                .cx = ScaleX(DUEL_LEFT_X), .cy = ScaleY(DUEL_LEFT_Y),
                .radius = ScaleX(DUEL_BTN_R * 0.7f), .btnId = TOUCH_BTN_DPAD_LEFT, .visible = true
            };
            gButtons[gButtonCount++] = (TouchButton){
                .cx = ScaleX(0.28f), .cy = ScaleY(DUEL_LEFT_Y),
                .radius = ScaleX(DUEL_BTN_R * 0.7f), .btnId = TOUCH_BTN_DPAD_RIGHT, .visible = true
            };
            break;

        case TOUCH_SCHEME_STAMPEDE:
            // Jump button (right side) - virtual joystick on left
            gButtons[gButtonCount++] = (TouchButton){
                .cx = ScaleX(BTN_JUMP_X), .cy = ScaleY(BTN_JUMP_Y),
                .radius = ScaleX(BTN_RADIUS), .btnId = TOUCH_BTN_JUMP, .visible = true
            };
            break;

        case TOUCH_SCHEME_TARGET:
            // Shoot button (right side) - touch-to-aim for aiming
            gButtons[gButtonCount++] = (TouchButton){
                .cx = ScaleX(BTN_SHOOT_X), .cy = ScaleY(BTN_SHOOT_Y),
                .radius = ScaleX(BTN_RADIUS), .btnId = TOUCH_BTN_SHOOT, .visible = true
            };
            break;
    }

    // Set joystick default position
    gJsBaseX = ScaleX(JS_CX);
    gJsBaseY = ScaleY(JS_CY);
    gJsRadius = ScaleX(JS_RADIUS);
    if (!gJsActive) {
        gJsThumbX = gJsBaseX;
        gJsThumbY = gJsBaseY;
    }
}

// ============================================================================
// Touch button hit test
// ============================================================================

static TouchButton* HitTestButton(float x, float y) {
    for (int i = 0; i < gButtonCount; i++) {
        TouchButton *b = &gButtons[i];
        if (!b->visible) continue;
        if (Dist(x, y, b->cx, b->cy) <= b->radius * 1.3f)  // 30% larger hit area
            return b;
    }
    return NULL;
}

// Per-touch tracking (finger → button)
#define MAX_TOUCH_FINGERS 10
static struct {
    SDL_FingerID fingerID;
    TouchButtonID btnId;
    bool active;
} gTouchMap[MAX_TOUCH_FINGERS];

static void TouchMap_AssignButton(SDL_FingerID fid, TouchButtonID btnId) {
    for (int i = 0; i < MAX_TOUCH_FINGERS; i++) {
        if (!gTouchMap[i].active) {
            gTouchMap[i].fingerID = fid;
            gTouchMap[i].btnId = btnId;
            gTouchMap[i].active = true;
            return;
        }
    }
}

static TouchButtonID TouchMap_GetButton(SDL_FingerID fid) {
    for (int i = 0; i < MAX_TOUCH_FINGERS; i++) {
        if (gTouchMap[i].active && gTouchMap[i].fingerID == fid)
            return gTouchMap[i].btnId;
    }
    return TOUCH_BTN_COUNT; // invalid
}

static void TouchMap_Remove(SDL_FingerID fid) {
    for (int i = 0; i < MAX_TOUCH_FINGERS; i++) {
        if (gTouchMap[i].active && gTouchMap[i].fingerID == fid) {
            gTouchMap[i].active = false;
            return;
        }
    }
}

// Returns true if any mapped finger is holding this button
static bool TouchMap_HasButton(TouchButtonID btnId) {
    for (int i = 0; i < MAX_TOUCH_FINGERS; i++) {
        if (gTouchMap[i].active && gTouchMap[i].btnId == btnId)
            return true;
    }
    return false;
}

// ============================================================================
// Public API
// ============================================================================

void TouchControls_Init(void) {
    memset(gBtnState, 0, sizeof(gBtnState));
    memset(gTouchMap, 0, sizeof(gTouchMap));
    gJsActive = false;
    gJoystickX = gJoystickY = 0.0f;
    gHasAimTouch = false;
    gAimTriggeredShoot = false;
    gScheme = TOUCH_SCHEME_MENU;
    RebuildLayout();
}

void TouchControls_Shutdown(void) {
    // nothing to free
}

void TouchControls_SetScheme(TouchControlScheme scheme) {
    if (gScheme == scheme) return;
    gScheme = scheme;
    // Reset all input state when switching scheme
    memset(gBtnState, 0, sizeof(gBtnState));
    memset(gTouchMap, 0, sizeof(gTouchMap));
    gJsActive = false;
    gJoystickX = gJoystickY = 0.0f;
    gHasAimTouch = false;
    gAimTriggeredShoot = false;
    RebuildLayout();
}

TouchControlScheme TouchControls_GetScheme(void) {
    return gScheme;
}

void TouchControls_SetSchemeForArea(int area) {
    switch (area) {
        case AREA_TOWN_DUEL1:
        case AREA_TOWN_DUEL2:
        case AREA_TOWN_DUEL3:
        case AREA_SWAMP_DUEL1:
        case AREA_SWAMP_DUEL2:
        case AREA_SWAMP_DUEL3:
            TouchControls_SetScheme(TOUCH_SCHEME_DUEL);
            break;

        case AREA_TOWN_SHOOTOUT:
        case AREA_SWAMP_SHOOTOUT:
            TouchControls_SetScheme(TOUCH_SCHEME_SHOOTOUT);
            break;

        case AREA_TOWN_STAMPEDE:
        case AREA_SWAMP_STAMPEDE:
            TouchControls_SetScheme(TOUCH_SCHEME_STAMPEDE);
            break;

        case AREA_TARGETPRACTICE1:
        case AREA_TARGETPRACTICE2:
            TouchControls_SetScheme(TOUCH_SCHEME_TARGET);
            break;

        default:
            TouchControls_SetScheme(TOUCH_SCHEME_MENU);
            break;
    }
}

void TouchControls_Update(void) {
    // Clear just-pressed/released flags at end of frame
    for (int i = 0; i < TOUCH_BTN_COUNT; i++) {
        gBtnState[i].justPressed = false;
        gBtnState[i].justReleased = false;
    }
}

void TouchControls_ProcessEvent(const SDL_Event* event) {
    if (!event) return;

    float touchX, touchY;

    switch (event->type) {
        case SDL_EVENT_FINGER_DOWN:
        {
            touchX = event->tfinger.x * gWinW;
            touchY = event->tfinger.y * gWinH;

            // For shootout/target: tap ANYWHERE that isn't a button aims the crosshair there
            // and immediately fires a shot (rail-shooter style controls).
            if ((gScheme == TOUCH_SCHEME_SHOOTOUT || gScheme == TOUCH_SCHEME_TARGET)
                && !gHasAimTouch
                && HitTestButton(touchX, touchY) == NULL)
            {
                gAimX = touchX;
                gAimY = touchY;
                gHasAimTouch = true;
                gAimFingerID = event->tfinger.fingerID;
                // Immediately count as a shoot at this point
                gAimTriggeredShoot = true;
                SetPressed(TOUCH_BTN_SHOOT, true);
                break;
            }

            // For stampede: virtual joystick on left side
            if (gScheme == TOUCH_SCHEME_STAMPEDE && !IsRightSideTouch(touchX)
                && HitTestButton(touchX, touchY) == NULL && !gJsActive)
            {
                gJsActive = true;
                gJsBaseX = touchX;
                gJsBaseY = touchY;
                gJsThumbX = touchX;
                gJsThumbY = touchY;
                gJsFingerID = event->tfinger.fingerID;
                gJoystickX = gJoystickY = 0.0f;
                break;
            }

            // Hit-test buttons
            TouchButton *btn = HitTestButton(touchX, touchY);
            if (btn) {
                TouchMap_AssignButton(event->tfinger.fingerID, btn->btnId);
                SetPressed(btn->btnId, true);
            }
            break;
        }

        case SDL_EVENT_FINGER_MOTION:
        {
            touchX = event->tfinger.x * gWinW;
            touchY = event->tfinger.y * gWinH;

            // Update aim
            if (gHasAimTouch && event->tfinger.fingerID == gAimFingerID) {
                gAimX = touchX;
                gAimY = touchY;
                break;
            }

            // Update joystick
            if (gJsActive && event->tfinger.fingerID == gJsFingerID) {
                float dx = touchX - gJsBaseX;
                float dy = touchY - gJsBaseY;
                float dist = sqrtf(dx*dx + dy*dy);
                float maxDist = gJsRadius;
                if (dist > maxDist) {
                    dx = dx / dist * maxDist;
                    dy = dy / dist * maxDist;
                    dist = maxDist;
                }
                gJsThumbX = gJsBaseX + dx;
                gJsThumbY = gJsBaseY + dy;

                // Compute normalized joystick value with dead zone
                float nx = dx / maxDist;
                float ny = dy / maxDist;
                float len = sqrtf(nx*nx + ny*ny);
                if (len < JS_DEADZONE) {
                    gJoystickX = gJoystickY = 0.0f;
                } else {
                    float scale = (len - JS_DEADZONE) / (1.0f - JS_DEADZONE);
                    gJoystickX = (nx / len) * scale;
                    gJoystickY = (ny / len) * scale;
                }
                break;
            }
            break;
        }

        case SDL_EVENT_FINGER_UP:
        {
            // Release aim
            if (gHasAimTouch && event->tfinger.fingerID == gAimFingerID) {
                gHasAimTouch = false;
                // Release shoot if the aim tap triggered it (and no dedicated shoot button finger)
                if (gAimTriggeredShoot) {
                    gAimTriggeredShoot = false;
                    if (!TouchMap_HasButton(TOUCH_BTN_SHOOT))
                        SetPressed(TOUCH_BTN_SHOOT, false);
                }
                break;
            }

            // Release joystick
            if (gJsActive && event->tfinger.fingerID == gJsFingerID) {
                gJsActive = false;
                gJsThumbX = gJsBaseX;
                gJsThumbY = gJsBaseY;
                gJoystickX = gJoystickY = 0.0f;
                break;
            }

            // Release button
            TouchButtonID btnId = TouchMap_GetButton(event->tfinger.fingerID);
            if (btnId < TOUCH_BTN_COUNT) {
                TouchMap_Remove(event->tfinger.fingerID);
                // Check no other finger is still on this button
                bool stillPressed = false;
                for (int i = 0; i < MAX_TOUCH_FINGERS; i++) {
                    if (gTouchMap[i].active && gTouchMap[i].btnId == btnId)
                        stillPressed = true;
                }
                if (!stillPressed)
                    SetPressed(btnId, false);
            }
            break;
        }

        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            gWinW = event->window.data1;
            gWinH = event->window.data2;
            RebuildLayout();
            break;

        default:
            break;
    }
}

bool TouchControls_IsPressed(TouchButtonID btn) {
    if (btn >= TOUCH_BTN_COUNT) return false;
    return gBtnState[btn].pressed;
}

bool TouchControls_WasJustPressed(TouchButtonID btn) {
    if (btn >= TOUCH_BTN_COUNT) return false;
    return gBtnState[btn].justPressed;
}

bool TouchControls_WasJustReleased(TouchButtonID btn) {
    if (btn >= TOUCH_BTN_COUNT) return false;
    return gBtnState[btn].justReleased;
}

float TouchControls_GetJoystickX(void) { return gJoystickX; }
float TouchControls_GetJoystickY(void) { return gJoystickY; }
float TouchControls_GetAimX(void)      { return gAimX; }
float TouchControls_GetAimY(void)      { return gAimY; }
bool  TouchControls_HasAimTouch(void)  { return gHasAimTouch; }

// ============================================================================
// Drawing helpers
// ============================================================================

static void DrawFilledCircle(float cx, float cy, float r, int segs) {
    bridge_Begin(GL_TRIANGLE_FAN);
    bridge_Vertex2f(cx, cy);
    for (int i = 0; i <= segs; i++) {
        float a = (float)i / segs * 2.0f * TC_PI;
        bridge_Vertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
    }
    bridge_End();
}

static void DrawCircleOutline(float cx, float cy, float r, int segs) {
    bridge_Begin(GL_LINE_LOOP);
    for (int i = 0; i < segs; i++) {
        float a = (float)i / segs * 2.0f * TC_PI;
        bridge_Vertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
    }
    bridge_End();
}

static void DrawArrow(float cx, float cy, float size, int dir) {
    // dir: 0=left, 1=right, 2=up, 3=down
    float dx = 0, dy = 0;
    if (dir == 0) dx = -1; else if (dir == 1) dx = 1;
    if (dir == 2) dy = -1; else if (dir == 3) dy = 1;

    bridge_Begin(GL_TRIANGLES);
    if (dir == 0 || dir == 1) {
        bridge_Vertex2f(cx + dx*size,        cy);
        bridge_Vertex2f(cx - dx*size*0.5f,   cy - size*0.7f);
        bridge_Vertex2f(cx - dx*size*0.5f,   cy + size*0.7f);
    } else {
        bridge_Vertex2f(cx,              cy + dy*size);
        bridge_Vertex2f(cx - size*0.7f,  cy - dy*size*0.5f);
        bridge_Vertex2f(cx + size*0.7f,  cy - dy*size*0.5f);
    }
    bridge_End();
}

// ============================================================================
// Draw overlay
// ============================================================================

void TouchControls_Draw(int windowW, int windowH) {
    if (windowW != gWinW || windowH != gWinH) {
        gWinW = windowW;
        gWinH = windowH;
        RebuildLayout();
    }

    // Set up 2D ortho projection
    bridge_MatrixMode(GL_PROJECTION);
    bridge_PushMatrix();
    bridge_LoadIdentity();
    bridge_Ortho(0, windowW, windowH, 0, -1, 1);

    bridge_MatrixMode(GL_MODELVIEW);
    bridge_PushMatrix();
    bridge_LoadIdentity();

    // Disable depth, lighting, textures for 2D overlay
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    bridge_Disable(GL_LIGHTING);
    bridge_Disable(GL_TEXTURE_2D);
    bridge_Disable(GL_FOG);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bridge_FlushState();

    // Draw virtual joystick (stampede mode only)
    if (gScheme == TOUCH_SCHEME_STAMPEDE) {
        float bx = gJsActive ? gJsBaseX : ScaleX(JS_CX);
        float by = gJsActive ? gJsBaseY : ScaleY(JS_CY);
        float r = ScaleX(JS_RADIUS);

        // Background ring
        bridge_Color4f(0.3f, 0.3f, 0.3f, 0.2f);
        DrawFilledCircle(bx, by, r, 32);
        bridge_Color4f(0.7f, 0.7f, 0.7f, 0.4f);
        DrawCircleOutline(bx, by, r, 32);

        // Thumb
        float tx = gJsActive ? gJsThumbX : bx;
        float ty = gJsActive ? gJsThumbY : by;
        bridge_Color4f(0.6f, 0.6f, 0.6f, 0.45f);
        DrawFilledCircle(tx, ty, r * 0.35f, 20);
        bridge_Color4f(0.9f, 0.9f, 0.9f, 0.5f);
        DrawCircleOutline(tx, ty, r * 0.35f, 20);
    }

    // Draw buttons
    for (int i = 0; i < gButtonCount; i++) {
        TouchButton *b = &gButtons[i];
        if (!b->visible) continue;

        bool pressed = gBtnState[b->btnId].pressed;
        float alpha = pressed ? 0.55f : 0.30f;
        float r = b->radius;

        // Button fill colour by function
        switch (b->btnId) {
            case TOUCH_BTN_SHOOT:
                bridge_Color4f(0.8f, 0.2f, 0.2f, alpha);
                break;
            case TOUCH_BTN_DUCK:
                bridge_Color4f(0.2f, 0.5f, 0.8f, alpha);
                break;
            case TOUCH_BTN_JUMP:
                bridge_Color4f(0.2f, 0.8f, 0.2f, alpha);
                break;
            case TOUCH_BTN_CONFIRM:
                bridge_Color4f(0.2f, 0.8f, 0.2f, alpha);
                break;
            case TOUCH_BTN_BACK:
                bridge_Color4f(0.8f, 0.5f, 0.1f, alpha);
                break;
            case TOUCH_BTN_PAUSE:
                bridge_Color4f(0.5f, 0.5f, 0.5f, alpha * 0.6f);
                break;
            default:
                bridge_Color4f(0.5f, 0.5f, 0.5f, alpha);
                break;
        }
        DrawFilledCircle(b->cx, b->cy, r, 24);

        // Button outline
        bridge_Color4f(1.0f, 1.0f, 1.0f, alpha + 0.1f);
        DrawCircleOutline(b->cx, b->cy, r, 24);

        // Arrow glyph for d-pad buttons
        float arrowAlpha = pressed ? 0.9f : 0.7f;
        bridge_Color4f(1.0f, 1.0f, 1.0f, arrowAlpha);
        switch (b->btnId) {
            case TOUCH_BTN_DPAD_LEFT:  DrawArrow(b->cx, b->cy, r*0.45f, 0); break;
            case TOUCH_BTN_DPAD_RIGHT: DrawArrow(b->cx, b->cy, r*0.45f, 1); break;
            case TOUCH_BTN_DPAD_UP:    DrawArrow(b->cx, b->cy, r*0.45f, 2); break;
            case TOUCH_BTN_DPAD_DOWN:  DrawArrow(b->cx, b->cy, r*0.45f, 3); break;
            default: break;
        }
    }

    // Restore matrices
    bridge_MatrixMode(GL_PROJECTION);
    bridge_PopMatrix();
    bridge_MatrixMode(GL_MODELVIEW);
    bridge_PopMatrix();

    // Restore GL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    bridge_Enable(GL_LIGHTING);
    bridge_FlushState();
}

#endif // __ANDROID__
