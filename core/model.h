// author: rozgo

#ifndef __MODEL_H__
#define __MODEL_H__

#include "qb.h"

extern const uint8_t QB_CTX_TYPE_MODEL;

typedef struct model_context_t {
    
    context_t ctx;
    
    octant_t* model;
    cuboid_t* limits;
    
    float timer;
    
} model_context_t;

model_context_t* qb_model_context_create ( void );
void qb_model_setup ( context_t* ctx );
void qb_model_render ( context_t* ctx );
void qb_model_set ( context_t* ctx, octant_t* octant );
void qb_model_zoom_end ( context_t* ctx );

#endif
