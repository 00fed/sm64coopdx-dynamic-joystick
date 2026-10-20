//Feel free to use it in your port too, but please keep authorship!
//Touch Controls made by: VDavid003
/* Heavily modified by xLuigiGamerx and ManIsCat2 to add new features, like:

- DJUI Based touch controls editor
- Different texture per button and joystick (joystick base as well)

*/

#ifdef TOUCH_CONTROLS
#include <ultra64.h>
#include <PR/ultratypes.h>
#include <PR/gbi.h>
#include <math.h>

#include "config.h"
#include "sm64.h"
#include "game/game_init.h"
#include "game/memory.h"
#include "game/segment2.h"
#include "game/object_helpers.h"
#include "gfx_dimensions.h"
#include "pc/pc_main.h"
#include "pc/gfx/gfx_pc.h"
#include "pc/djui/djui_panel.h"
#include "pc/djui/djui_panel_pause.h"
#include "pc/djui/djui_panel_main.h"
#include "pc/djui/djui_panel_touch_controls_editor.h"
#include "pc/djui/djui_console.h"
#include "pc/network/network.h"

#include "controller_api.h"
#include "controller_touchscreen.h"
#include "controller_touchscreen_textures.h"

#include "pc/configfile.h"

// Mouselook
s16 before_x = 0;
s16 before_y = 0;
s16 touch_x = 0;
s16 touch_y = 0;
s16 touch_cam_last_x = 0;
s16 touch_cam_last_y = 0;

// Config
bool gInTouchConfig = false, gGamepadActive = false;
enum ConfigControlElementIndex gSelectedTouchElement = TOUCH_MOUSE;

// Dynamic Joystick
static struct Position sDynamicJoystickBase = { 0, 0 };
static bool sDynamicJoystickActive = false;
static SDL_TouchID sJoystickTouchID = 0;

ConfigControlElement configControlElementsDefault[TOUCH_COUNT] = {
#include "controller_touchscreen_layout.inc"
};

ConfigControlElement configControlElements[TOUCH_COUNT] = {
#include "controller_touchscreen_layout.inc"
};

ConfigControlElement configControlElementsLast[TOUCH_COUNT] = {
#include "controller_touchscreen_layout.inc"
};

// This order must match configControlElements and ConfigControlElementIndex
static struct ControlElement controlElements[TOUCH_COUNT] = {
    [TOUCH_STICK] =      {.type = Joystick},
    [TOUCH_MOUSE] =      {.type = Mouse},
    [TOUCH_A] =          {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_A,          .buttonDown = TEXTURE_TOUCH_A_PRESSED },          .buttonID = A_BUTTON},
    [TOUCH_B] =          {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_B,          .buttonDown = TEXTURE_TOUCH_B_PRESSED },          .buttonID = B_BUTTON},
    [TOUCH_X] =          {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_X,          .buttonDown = TEXTURE_TOUCH_X_PRESSED },          .buttonID = X_BUTTON},
    [TOUCH_Y] =          {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_Y,          .buttonDown = TEXTURE_TOUCH_Y_PRESSED },          .buttonID = Y_BUTTON},
    [TOUCH_START] =      {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_START,      .buttonDown = TEXTURE_TOUCH_START_PRESSED },      .buttonID = START_BUTTON},
    [TOUCH_L] =          {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_L,          .buttonDown = TEXTURE_TOUCH_L_PRESSED },          .buttonID = L_TRIG},
    [TOUCH_R] =          {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_R,          .buttonDown = TEXTURE_TOUCH_R_PRESSED },          .buttonID = R_TRIG},
    [TOUCH_Z] =          {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_Z,          .buttonDown = TEXTURE_TOUCH_Z_PRESSED },          .buttonID = Z_TRIG},
    [TOUCH_CUP] =        {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_C_UP,       .buttonDown = TEXTURE_TOUCH_C_UP_PRESSED },       .buttonID = U_CBUTTONS},
    [TOUCH_CDOWN] =      {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_C_DOWN,     .buttonDown = TEXTURE_TOUCH_C_DOWN_PRESSED },     .buttonID = D_CBUTTONS},
    [TOUCH_CLEFT] =      {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_C_LEFT,     .buttonDown = TEXTURE_TOUCH_C_LEFT_PRESSED },     .buttonID = L_CBUTTONS},
    [TOUCH_CRIGHT] =     {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_C_RIGHT,    .buttonDown = TEXTURE_TOUCH_C_RIGHT_PRESSED },    .buttonID = R_CBUTTONS},
    [TOUCH_CHAT] =       {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_CHAT,       .buttonDown = TEXTURE_TOUCH_CHAT_PRESSED },       .buttonID = CHAT_BUTTON},
    [TOUCH_PLAYERLIST] = {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_PLAYERLIST, .buttonDown = TEXTURE_TOUCH_PLAYERLIST_PRESSED }, .buttonID = PLAYERLIST_BUTTON},
    [TOUCH_DUP] =        {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_DPAD_UP,    .buttonDown = TEXTURE_TOUCH_DPAD_UP_PRESSED },    .buttonID = U_JPAD},
    [TOUCH_DDOWN] =      {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_DPAD_DOWN,  .buttonDown = TEXTURE_TOUCH_DPAD_DOWN_PRESSED },  .buttonID = D_JPAD},
    [TOUCH_DLEFT] =      {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_DPAD_LEFT,  .buttonDown = TEXTURE_TOUCH_DPAD_LEFT_PRESSED },  .buttonID = L_JPAD},
    [TOUCH_DRIGHT] =     {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_DPAD_RIGHT, .buttonDown = TEXTURE_TOUCH_DPAD_RIGHT_PRESSED }, .buttonID = R_JPAD},
    [TOUCH_CONSOLE] =    {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_CONSOLE,    .buttonDown = TEXTURE_TOUCH_CONSOLE_PRESSED },    .buttonID = CONSOLE_BUTTON},
};

static u32 controlElementsLength = sizeof(controlElements)/sizeof(struct ControlElement);

struct Position get_pos(ConfigControlElement *config) {
    struct Position ret;
    switch (config->anchor) {
        case CONTROL_ELEMENT_LEFT:
            ret.x = RECT_FROM_LEFT_EDGE(config->x << 2);
            ret.y = config->y;
            break;
        case CONTROL_ELEMENT_RIGHT:
            // Correctly project inward from RIGHT_EDGE
            ret.x = RECT_FROM_RIGHT_EDGE(config->x << 2);
            ret.y = config->y;
            break;
        case CONTROL_ELEMENT_CENTER:
            ret.x = SCREEN_WIDTH_API / 2;
            ret.y = config->y;
            break;
    }
    if (config->hidden) {
        if (!gInTouchConfig) {
            ret.x = HIDE_POS;
            ret.y = HIDE_POS;
        }
    }
    if (configSnapTouch) {
        ret.x = 50 * ((ret.x + 49) / 50) - 25;
        ret.y = 50 * ((ret.y + 49) / 50) - 25;
    }
    if (!gInTouchConfig && (gDjuiInMainMenu && !gDjuiDisabled)) {
        ret.x = HIDE_POS;
        ret.y = HIDE_POS;
    }
    return ret;
}

Colors get_color(ConfigControlElement *config) {
    Colors ret;
    
    ret.r = config->r;
    ret.g = config->g;
    ret.b = config->b;
    ret.a = config->a;

    return ret;
}

// File: src/pc/controller/controller_touchscreen.c

void move_touch_element(struct TouchEvent *event, enum ConfigControlElementIndex i) {
    ConfigControlElement *config = &configControlElements[i];

    s32 x_raw = CORRECT_TOUCH_X(event->x);
    s32 y_raw = CORRECT_TOUCH_Y(event->y);

    s32 x = 0;
    enum ConfigControlElementAnchor anchor;

    // Clamp coordinates to the active screen area so edge buttons remain grab-able
    if (x_raw < LEFT_EDGE) x_raw = LEFT_EDGE;
    if (x_raw > RIGHT_EDGE) x_raw = RIGHT_EDGE;
    if (y_raw < 0) y_raw = 0;
    if (y_raw > SCREEN_HEIGHT_API) y_raw = SCREEN_HEIGHT_API;

    if (x_raw < (SCREEN_WIDTH_API / 2) - 30) {
        // Left anchor: store positive offset from LEFT_EDGE
        s32 diff = x_raw - RECT_FROM_LEFT_EDGE(0);
        x = (diff > 0) ? (diff >> 2) : 0;
        anchor = CONTROL_ELEMENT_LEFT;
    } else if (x_raw > (SCREEN_WIDTH_API / 2) + 30) {
        // Right anchor: compute positive inward distance from RIGHT_EDGE
        s32 diff = RECT_FROM_RIGHT_EDGE(0) - x_raw;
        x = (diff > 0) ? (diff >> 2) : 0;
        anchor = CONTROL_ELEMENT_RIGHT;
    } else {
        x = SCREEN_WIDTH_API / 2;
        anchor = CONTROL_ELEMENT_CENTER;
    }

    config->x = x;
    config->y = y_raw;
    config->anchor = anchor;
}

void touch_down(struct TouchEvent* event) {
    gGamepadActive = false;
    struct Position pos;
    s32 touchX = CORRECT_TOUCH_X(event->x);
    s32 touchY = CORRECT_TOUCH_Y(event->y);
    if (gInTouchConfig) {
        // Iterate in reverse so topmost rendered buttons receive grab priority
        for (s32 i = (s32)controlElementsLength - 1; i >= 0; i--) {
            if (controlElements[i].touchID == 0) {
                pos = get_pos(&configControlElements[i]);
                if (pos.y == HIDE_POS) continue;
                // Use the visual rendered texture radius (64 * size for buttons, 128 * size for stick)
                // to eliminate deadzones at screen edges and corner boundaries
                f32 scale = configControlElements[i].size;
                if (scale <= 0.01f) scale = 0.01f;
                f32 halfW = (controlElements[i].type == Joystick ? 128.0f : 64.0f) * scale;
                f32 halfH = halfW;
                if (touchX >= (pos.x - halfW) && touchX <= (pos.x + halfW) &&
                    touchY >= (pos.y - halfH) && touchY <= (pos.y + halfH)) {
                    controlElements[i].touchID = event->touchID;
                    gSelectedTouchElement = (enum ConfigControlElementIndex)i;
                    break;
                }
            }
        }
        return;
    }
    // Pass 1: Prioritize Buttons first (A, B, X, Y, Z, L, R, D-pad, Start, Chat, etc.)
    for (u32 i = 0; i < controlElementsLength; i++) {
        if (controlElements[i].type == Button && controlElements[i].touchID == 0) {
            pos = get_pos(&configControlElements[i]);
            if (pos.y == HIDE_POS) continue;
            f32 halfW = 64.0f * configControlElements[i].size;
            if (touchX >= (pos.x - halfW) && touchX <= (pos.x + halfW) &&
                touchY >= (pos.y - halfW) && touchY <= (pos.y + halfW)) {
                controlElements[i].touchID = event->touchID;
                gSelectedTouchElement = i;
                if (controlElements[i].buttonID == CHAT_BUTTON)
                    djui_interactable_on_key_down(configKeyChat[0]);
                if (controlElements[i].buttonID == PLAYERLIST_BUTTON)
                    djui_interactable_on_key_down(configKeyPlayerList[0]);
                return;
            }
        }
    }
    // Pass 2: Restrict Dynamic Joystick strictly to the bottom-left quadrant
    pos = get_pos(&configControlElements[TOUCH_STICK]);
    
    // 1. Tetikleme alanını ekranın sol 1/3'lük dilimine çekiyoruz
    // Böylece orta bölgedeki dokunmalar doğrudan kameraya (TOUCH_MOUSE) kalır
    if (pos.y != HIDE_POS && sJoystickTouchID == 0 &&
        touchX < (SCREEN_WIDTH_API / 3) && touchY > (SCREEN_HEIGHT_API / 2)) {
        
        f32 scale = configControlElements[TOUCH_STICK].size;
        if (scale <= 0.01f) scale = 0.01f;
        s32 pad = (s32)(128.0f * scale); 

        s32 baseX = touchX;
        s32 baseY = touchY;

        // 2. Ekranın sol kenarından taşmayı engelle
        if (baseX - pad < LEFT_EDGE) baseX = LEFT_EDGE + pad;
        
        // 3. Sağ sınırı yeni 1/3'lük çizgiye kelepçele
        if (baseX + pad > (SCREEN_WIDTH_API / 3)) baseX = (SCREEN_WIDTH_API / 3) - pad;

        // Dikey (Y) Eksenini Kelepçele
        if (baseY - pad < (SCREEN_HEIGHT_API / 2)) baseY = (SCREEN_HEIGHT_API / 2) + pad;
        if (baseY + pad > SCREEN_HEIGHT_API) baseY = SCREEN_HEIGHT_API - pad;

        sJoystickTouchID = event->touchID;
        controlElements[TOUCH_STICK].touchID = event->touchID;
        gSelectedTouchElement = TOUCH_STICK;
        
        sDynamicJoystickBase.x = baseX;
        sDynamicJoystickBase.y = baseY;
        sDynamicJoystickActive = true;
        controlElements[TOUCH_STICK].joyX = 0;
        controlElements[TOUCH_STICK].joyY = 0;
        return;
    }
    // Pass 3: Global Modern Camera/Mouse Area (Fallback)
    if (controlElements[TOUCH_MOUSE].touchID == 0) {
        controlElements[TOUCH_MOUSE].touchID = event->touchID;
        before_x = touchX;
        before_y = touchY;
        touch_x = 0;
        touch_y = 0;
        return;
    }
}
void touch_motion(struct TouchEvent* event) {
    struct Position pos;
    s32 size;
    for(u32 i = 0; i < controlElementsLength; i++) {
        pos = get_pos(&configControlElements[i]);
        if (pos.y == HIDE_POS) continue;
        size = (s32)roundf(configControlElements[i].size * 100.0f);
        if (gInTouchConfig) {
            if (controlElements[i].touchID == event->touchID && controlElements[i].type != Mouse && gSelectedTouchElement == i) {
                move_touch_element(event, gSelectedTouchElement);
            }
        } else {
            if (!gDjuiPanelPauseCreated) {
                if (controlElements[i].touchID == event->touchID) {
                    s32 x, y;
                    switch (controlElements[i].type) {
                        case Joystick: {
                            // Touch ID protection: only the locked touch ID can move the joystick
                            if (event->touchID != sJoystickTouchID || !sDynamicJoystickActive) {
                                break;
                            }
                            struct Position center = sDynamicJoystickBase;
                            if (configPhantomTouch) {
                                s32 touchX = CORRECT_TOUCH_X(event->x);
                                s32 touchY = CORRECT_TOUCH_Y(event->y);
                                if (abs(touchX - center.x) > (size * 6) / 2 || abs(touchY - center.y) > (size * 6) / 2) {
                                    controlElements[i].joyX = 0;
                                    controlElements[i].joyY = 0;
                                    controlElements[i].touchID = 0;
                                    sDynamicJoystickActive = false;
                                    sJoystickTouchID = 0;
                                    break;
                                }
                            }
                            x = CORRECT_TOUCH_X(event->x) - center.x;
                            y = CORRECT_TOUCH_Y(event->y) - center.y;
                            if (x > size / 2)
                                x = size / 2;
                            if (x < -size / 2)
                                x = - size / 2;
                            if (y > size / 2)
                                y = size / 2;
                            if (y < -size / 2)
                                y = - size / 2;
                            controlElements[i].joyX = x;
                            controlElements[i].joyY = y;
                            break;
                        }
                        case Mouse:
                            if (configPhantomTouch && !TRIGGER_DETECT(size)) {
                                touch_x = before_x = 0;
                                touch_y = before_y = 0;
                                controlElements[i].touchID = 0;
                                break;
                            }
                            if (before_x > 0)
                                touch_x = CORRECT_TOUCH_X(event->x) - before_x;
                            if (before_y > 0)
                                touch_y = CORRECT_TOUCH_Y(event->y) - before_y;
                            before_x = CORRECT_TOUCH_X(event->x);
                            before_y = CORRECT_TOUCH_Y(event->y);
                            if ((u16)abs(touch_x) < configStickDeadzone / 4)
                                touch_x = 0;
                            if ((u16)abs(touch_y) < configStickDeadzone / 4)
                                touch_y = 0;
                            break;
                        case Button:
                            if ((controlElements[i].slideTouch && !TRIGGER_DETECT(size)) || (configPhantomTouch && !controlElements[i].slideTouch && !TRIGGER_DETECT(size * 3))) {
                                controlElements[i].slideTouch = 0;
                                controlElements[i].touchID = 0;
                            }
                            break;
                    }
                } else if (controlElements[i].type == Button && TRIGGER_DETECT(size) && (controlElements[TOUCH_MOUSE].touchID != event->touchID || !configFreeCameraMouse) && configSlideTouch) {
                    controlElements[i].slideTouch = 1;
                    controlElements[i].touchID = event->touchID;
                    if (controlElements[i].buttonID == CHAT_BUTTON)
                        djui_interactable_on_key_down(configKeyChat[0]);
                    if (controlElements[i].buttonID == PLAYERLIST_BUTTON)
                        djui_interactable_on_key_down(configKeyPlayerList[0]);
                }
            }
        }
    }
}

static void handle_touch_up(u32 i) { 
    controlElements[i].touchID = 0;
    if (controlElements[i].type == Joystick) {
        controlElements[i].joyX = 0;
        controlElements[i].joyY = 0;
        sDynamicJoystickActive = false;
        sJoystickTouchID = 0;
    }
    struct Position pos = get_pos(&configControlElements[i]);
    if (pos.y == HIDE_POS) { return; }
    switch (controlElements[i].type) {
        case Joystick:
            break;
        case Mouse:
            touch_x = before_x = 0;
            touch_y = before_y = 0;
            break;
        case Button:
            if (controlElements[i].buttonID == CHAT_BUTTON && !gInTouchConfig)
                djui_interactable_on_key_up(configKeyChat[0]);
            if (controlElements[i].buttonID == PLAYERLIST_BUTTON && !gInTouchConfig)
                djui_interactable_on_key_up(configKeyPlayerList[0]);
            if (controlElements[i].buttonID == CONSOLE_BUTTON && !gInTouchConfig)
                djui_console_toggle();
            break;
    }
}

void touch_up(struct TouchEvent* event) {
    for (u32 i = 0; i < controlElementsLength; i++) {
        if (controlElements[i].touchID == event->touchID) {
            if (!gInTouchConfig && controlElements[i].type == Joystick && sJoystickTouchID != 0 && event->touchID != sJoystickTouchID) {
                continue;
            }
            handle_touch_up(i);
        }
    }
}

static void render_texture(const Texture *texture, s32 x, s32 y, u32 w, u32 h, f32 scale, u8 r, u8 g, u8 b, u8 a) {
    gSPClearGeometryMode(gDisplayListHead++, G_LIGHTING);
    gDPSetCombineMode(gDisplayListHead++, G_CC_FADEA, G_CC_FADEA);
    gDPSetRenderMode(gDisplayListHead++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetTextureFilter(gDisplayListHead++, G_TF_POINT);
    gSPTexture(gDisplayListHead++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPLoadTextureBlock(gDisplayListHead++, texture, G_IM_FMT_RGBA, G_IM_SIZ_16b, w, h, 0, G_TX_CLAMP, G_TX_CLAMP, 0, 0, 0, 0);
    gDPSetEnvColor(gDisplayListHead++, r, g, b, a);

    if (scale <= 0.01f) scale = 0.01f;
    s32 half_w = (s32)roundf(w * 4.0f * scale);
    s32 half_h = (s32)roundf(h * 4.0f * scale);
    s32 dsdx = (s32)roundf(512.0f / scale);
    s32 dtdy = dsdx;
    gSPTextureRectangle(gDisplayListHead++, x - half_w, y - half_h, x + half_w, y + half_h, G_TX_RENDERTILE, 0, 0, dsdx, dtdy);
    gSPTexture(gDisplayListHead++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetCombineMode(gDisplayListHead++, G_CC_SHADE, G_CC_SHADE);
}


void render_touch_controls(void) {
    if ((gGamepadActive && configAutohideTouch) || (!gDjuiInMainMenu && gDjuiDisabled)) { return; }

    struct Position pos;
    struct Position stick;
    Colors color;
    f32 size;
    f32 normalizedVectorMultiplier;
    
    create_dl_ortho_matrix();

    for (u32 i = 0; i < controlElementsLength; i++) {
        pos = get_pos(&configControlElements[i]);
        color = get_color(&configControlElements[i]);
        size = configControlElements[i].size;
        if (pos.y == HIDE_POS) continue;
        switch (controlElements[i].type) {
            case Joystick:
                if (absi(controlElements[i].joyX) + absi(controlElements[i].joyY) != 0) {
                    normalizedVectorMultiplier = sqrt((controlElements[i].joyX * controlElements[i].joyX) + (controlElements[i].joyY * controlElements[i].joyY))/(absi(controlElements[i].joyX) + absi(controlElements[i].joyY));
                } else {
                    normalizedVectorMultiplier = 0;
                }
                if (gInTouchConfig || gDjuiPanelPauseCreated) {
                    stick.x = 0;
                    stick.y = 0;
                } else {
                    stick.x = (controlElements[i].joyX * normalizedVectorMultiplier * 2);
                    stick.y = (controlElements[i].joyY * normalizedVectorMultiplier * 2);
                }
                struct Position basePos = (sDynamicJoystickActive && !gInTouchConfig) ? sDynamicJoystickBase : pos;
                render_texture(touch_textures[TEXTURE_TOUCH_JOYSTICK_BASE], basePos.x, basePos.y, 32, 32, size, color.r, color.g, color.b, color.a);
                render_texture(touch_textures[TEXTURE_TOUCH_JOYSTICK], basePos.x + stick.x, basePos.y + stick.y, 16, 16, size, color.r, color.g, color.b, color.a);
                break;
            case Mouse:
                break;
            case Button:
                if (!controlElements[i].touchID || gInTouchConfig || gDjuiPanelPauseCreated) {
                    render_texture(touch_textures[controlElements[i].buttonTexture.buttonUp], pos.x, pos.y, 16, 16, size, color.r, color.g, color.b, color.a);
                } else {
                    render_texture(touch_textures[controlElements[i].buttonTexture.buttonDown], pos.x, pos.y, 16, 16, size, color.r, color.g, color.b, color.a);
                }
                break;
        }
    }
}

static void touchscreen_init(void) {
    for (u32 i = 0; i < controlElementsLength; i++) {
        controlElements[i].touchID = 0;
        controlElements[i].joyX = 0;
        controlElements[i].joyY = 0;
        controlElements[i].slideTouch = 0;
    }
    sDynamicJoystickActive = false;
    sJoystickTouchID = 0;
}

static void touchscreen_read(OSContPad *pad) {
    struct Position pos;
    s32 size;
    if (!gInTouchConfig && !gDjuiPanelPauseCreated) {
        for(u32 i = 0; i < controlElementsLength; i++) {
            pos = get_pos(&configControlElements[i]);
            size = (s32)roundf(configControlElements[i].size * 100.0f);
            if (pos.y == HIDE_POS) continue;
            switch (controlElements[i].type) {
                case Joystick:
                    if (controlElements[i].joyX || controlElements[i].joyY) {
                        pad->stick_x = (controlElements[i].joyX + size / 2) * 255 / size - 128;
                        pad->stick_y = (-controlElements[i].joyY + size / 2) * 255 / size - 128; //inverted for some reason
                    }
                    break;
                case Mouse:
                    break;
                case Button:
                    if (controlElements[i].touchID && controlElements[i].buttonID != CHAT_BUTTON && controlElements[i].buttonID != PLAYERLIST_BUTTON && controlElements[i].buttonID != CONSOLE_BUTTON) {
                        pad->button |= controlElements[i].buttonID;
                    }
                    break;
            }
        }
    }
}

// Used by other controller types for setting keybinds
// Doesn't make a huge amount of sense for a touchscreen,
// So instead I allow customizing all button positions in
// an entirely separate construction, which is fine for now
// until someone wants multiple copies of the same button,
// at which point I will have to decide how to do that
static u32 touchscreen_rawkey(void) { 
    return VK_INVALID;
}

struct ControllerAPI controller_touchscreen = {
    0,
    touchscreen_init,
    touchscreen_read,
    touchscreen_rawkey,
    NULL,
    NULL,
    NULL,
    NULL
};
#endif
