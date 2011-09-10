#ifndef __GAMUT_H__
#define __GAMUT_H__

#include "qb.h"

extern const uint8_t QB_CTX_TYPE_GAMUT;

typedef struct gamut_context_t {
    
    context_t ctx;
    octant_t* gamut_octant;
    
} gamut_context_t;

gamut_context_t* qb_gamut_context_create ( void );
void qb_gamut_setup ( context_t* ctx );
void qb_gamut_render ( context_t* ctx );
void qb_gamut_pick ( context_t* ctx, vec3_t screen_pos, color_t color );

#endif


