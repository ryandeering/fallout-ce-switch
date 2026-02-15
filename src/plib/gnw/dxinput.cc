#include "plib/gnw/dxinput.h"
#ifdef __SWITCH__
#include "plib/gnw/mouse.h"
#endif

namespace fallout {

static bool dxinput_mouse_init();
static void dxinput_mouse_exit();
static bool dxinput_keyboard_init();
static void dxinput_keyboard_exit();

static int gMouseWheelDeltaX = 0;
static int gMouseWheelDeltaY = 0;

#ifdef __SWITCH__
static const int JOYSTICK_DEAD_ZONE = 8000;
static PadState pad;
double cursorSpeedup = 1.0;

void dxinput_reinitialize_switch_pad()
{
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);
}
#endif

// 0x4E0400
bool dxinput_init()
{
    if (SDL_InitSubSystem(SDL_INIT_EVENTS) != 0) {
        return false;
    }

    if (!dxinput_mouse_init()) {
        goto err;
    }

    if (!dxinput_keyboard_init()) {
        goto err;
    }

#ifdef __SWITCH__
    dxinput_reinitialize_switch_pad();
#endif
    return true;

err:

    dxinput_mouse_exit();

    return false;
}

// 0x4E0478
void dxinput_exit()
{
    SDL_QuitSubSystem(SDL_INIT_EVENTS);
}

// 0x4E04E8
bool dxinput_acquire_mouse()
{
    return true;
}

// 0x4E0514
bool dxinput_unacquire_mouse()
{
    return true;
}

// 0x4E053C
bool dxinput_get_mouse_state(MouseData* mouseState)
{
    // CE: This function is sometimes called outside loops calling `get_input`
    // and subsequently `GNW95_process_message`, so mouse events might not be
    // handled by SDL yet.
    //
    // TODO: Move mouse events processing into `GNW95_process_message` and
    // update mouse position manually.
    SDL_PumpEvents();

    Uint32 buttons = SDL_GetRelativeMouseState(&(mouseState->x), &(mouseState->y));
    mouseState->buttons[0] = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    mouseState->buttons[1] = (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    mouseState->wheelX = gMouseWheelDeltaX;
    mouseState->wheelY = gMouseWheelDeltaY;

    gMouseWheelDeltaX = 0;
    gMouseWheelDeltaY = 0;

     #ifdef __SWITCH__
    padUpdate(&pad);
    
    handleLeftStickMovement(mouseState);
    handleControllerButtons(mouseState);
    #endif

    return true;
}

// 0x4E05A8
bool dxinput_acquire_keyboard()
{
    return true;
}

// 0x4E05D4
bool dxinput_unacquire_keyboard()
{
    return true;
}

// 0x4E05FC
bool dxinput_flush_keyboard_buffer()
{
    SDL_FlushEvents(SDL_KEYDOWN, SDL_TEXTINPUT);
    return true;
}

// 0x4E0650
bool dxinput_read_keyboard_buffer(KeyboardData* keyboardData)
{
    return true;
}

// 0x4E070C
bool dxinput_mouse_init()
{
    return SDL_SetRelativeMouseMode(SDL_TRUE) == 0;
}

// 0x4E078C
void dxinput_mouse_exit()
{
}

// 0x4E07B8
bool dxinput_keyboard_init()
{
    return true;
}

// 0x4E0874
void dxinput_keyboard_exit()
{
}

void handleMouseEvent(SDL_Event* event)
{
    // Mouse movement and buttons are accumulated in SDL itself and will be
    // processed later in `mouseDeviceGetData` via `SDL_GetRelativeMouseState`.

    if (event->type == SDL_MOUSEWHEEL) {
        gMouseWheelDeltaX += event->wheel.x;
        gMouseWheelDeltaY += event->wheel.y;
    }
}

#ifdef __SWITCH__
void handleLeftStickMovement(MouseData* mouseState)
{
    HidAnalogStickState leftStick = padGetStickPos(&pad, 0);
    if (abs(leftStick.x) > JOYSTICK_DEAD_ZONE || abs(leftStick.y) > JOYSTICK_DEAD_ZONE) {
        mouseState->x += static_cast<int>((leftStick.x / 10000) * cursorSpeedup * (mouse_sensitivity * 1.5));
        mouseState->y -= static_cast<int>((leftStick.y / 10000) * cursorSpeedup * (mouse_sensitivity * 1.5));
    }

    // Clamp mouse coordinates to screen boundaries
    if (mouseState->x >= 1708) mouseState->x = 1707;
    if (mouseState->y >= 960) mouseState->y = 959;
}

void handleControllerButtons(MouseData* mouseState)
{
    u64 buttons = padGetButtons(&pad);
    mouseState->buttons[0] |= (buttons & HidNpadButton_ZL) != 0;
    mouseState->buttons[1] |= (buttons & HidNpadButton_ZR) != 0;
}
#endif

} // namespace fallout
