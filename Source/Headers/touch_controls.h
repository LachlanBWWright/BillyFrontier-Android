// touch_controls.h
// Touch control overlay for Billy Frontier Android port

#pragma once

#ifdef __ANDROID__

#include <stdbool.h>
#include <SDL3/SDL.h>

// Touch control schemes for different game modes
typedef enum {
    TOUCH_SCHEME_MENU,          // D-pad + confirm/back for menus
    TOUCH_SCHEME_DUEL,          // Left/right arrows + shoot button
    TOUCH_SCHEME_SHOOTOUT,      // Touch-to-aim + shoot/duck buttons
    TOUCH_SCHEME_STAMPEDE,      // Virtual joystick + jump button
    TOUCH_SCHEME_TARGET,        // Touch-to-aim + shoot button
} TouchControlScheme;

// Virtual button IDs
typedef enum {
    TOUCH_BTN_SHOOT     = 0,
    TOUCH_BTN_DUCK      = 1,
    TOUCH_BTN_JUMP      = 2,
    TOUCH_BTN_DPAD_LEFT = 3,
    TOUCH_BTN_DPAD_RIGHT= 4,
    TOUCH_BTN_DPAD_UP   = 5,
    TOUCH_BTN_DPAD_DOWN = 6,
    TOUCH_BTN_CONFIRM   = 7,
    TOUCH_BTN_BACK      = 8,
    TOUCH_BTN_PAUSE     = 9,
    TOUCH_BTN_COUNT     = 10,
} TouchButtonID;

// Initialize the touch control system
void TouchControls_Init(void);
void TouchControls_Shutdown(void);

// Set the active control scheme based on game mode
void TouchControls_SetScheme(TouchControlScheme scheme);
TouchControlScheme TouchControls_GetScheme(void);

// Set scheme from current area (call when entering a new level)
void TouchControls_SetSchemeForArea(int area);

// Process an SDL touch event
void TouchControls_ProcessEvent(const SDL_Event* event);

// Query button state
bool TouchControls_IsPressed(TouchButtonID btn);
bool TouchControls_WasJustPressed(TouchButtonID btn);
bool TouchControls_WasJustReleased(TouchButtonID btn);

// Update (call once per frame - clears just-pressed/released flags)
void TouchControls_Update(void);

// Get virtual joystick analog values (-1.0 to 1.0)
float TouchControls_GetJoystickX(void);
float TouchControls_GetJoystickY(void);

// Get touch position for touch-to-aim (in window pixels)
float TouchControls_GetAimX(void);
float TouchControls_GetAimY(void);
bool  TouchControls_HasAimTouch(void);

// Draw the touch overlay (call after main 3D scene)
void TouchControls_Draw(int windowW, int windowH);

#endif // __ANDROID__
