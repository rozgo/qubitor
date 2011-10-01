// author: rozgo

#ifndef __TOOLS_H__
#define __TOOLS_H__

#include "qb.h"

enum {
    TOOLS_MODE_SEL,
    TOOLS_MODE_ADD,
    TOOLS_MODE_DEL,
    TOOLS_MODE_ROT,
};

enum {
    TOOLS_BTN_SEL,
    TOOLS_BTN_ADD,
    TOOLS_BTN_DEL,
    TOOLS_BTN_ROT,
    TOOLS_BTN_SLI,
    TOOLS_BTN_SP0,
    TOOLS_BTN_LIB,
    TOOLS_BTN_PAL,
    TOOLS_BTN_SP1,
    TOOLS_BTN_SAV,
    NUM_TOOLS_BUTTONS
};

extern const uint8_t QB_CTX_TYPE_TOOLS;

typedef struct tools_context_t {
    
    context_t ctx;
    context_t* world_ctx;
    context_t* model_ctx;
    context_t* gamut_ctx;
    
    qube_t* buttons[NUM_TOOLS_BUTTONS * 2];
    octant_t* octants[NUM_TOOLS_BUTTONS];
    
    u_int8_t mode;
    
    int touch_moved;
    octant_t* touch_picked;
    vec3_t touch_plane;
    vec3_t touch_start;
    int touch_ticks;
    
//    octant_t* world_selected;
//    cuboid_t* world_selected_glow;
//    vec3_t world_selected_plane;
//    
//    octant_t* model_selected;
//    cuboid_t* model_selected_glow;
//    vec3_t model_selected_plane;
    
    uint8_t palette_on;
    uint8_t library_on;
    
} tools_context_t;

tools_context_t* qb_tools_context_create ( void );
void qb_tools_setup ( context_t* ctx );
void qb_tools_render ( context_t* ctx );

//void qb_tools_world_select ( context_t* ctx, octant_t* octant );
//void qb_tools_model_select ( context_t* ctx, octant_t* octant );
//void qb_tools_world_deselect ( context_t* ctx, octant_t* octant );
//void qb_tools_model_deselect ( context_t* ctx, octant_t* octant );
void qb_tools_pick ( context_t* ctx, octant_t* octant );

void qb_tools_mode_sel ( context_t* ctx );
void qb_tools_mode_add ( context_t* ctx );
void qb_tools_mode_del ( context_t* ctx );
void qb_tools_mode_rot ( context_t* ctx );

void qb_tools_library ( context_t* ctx, uint8_t show );
void qb_tools_palette ( context_t* ctx, uint8_t show );

void qb_tools_touch_began ( context_t* ctx, vec3_t screen_pos );
void qb_tools_touch_moved ( context_t* ctx, vec3_t screen_pos );
void qb_tools_touch_ended ( context_t* ctx, vec3_t screen_pos );

void qb_tools_on_gamut ( context_t* ctx, vec3_t screen_pos );

/// TODO: on/off wire


#endif
