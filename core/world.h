// author: rozgo

#ifndef __WORLD_H__
#define __WORLD_H__

#include "qb.h"

extern const uint8_t QB_CTX_TYPE_WORLD;

struct spline_t;

typedef struct world_context_t {
    
    context_t ctx;
    
    struct spline_t* spline;
    
    GLuint skybox_gl_id[3];
    GLuint ground_gl_id;
    
} world_context_t;

world_context_t* qb_world_context_create ( void );
void qb_world_setup ( context_t* ctx );
void qb_world_render ( context_t* ctx );
void qb_world_zoom_end ( context_t* ctx );

#endif
