#ifndef __SPLINE_H__
#define __SPLINE_H__

#include "qb.h"

typedef struct cubic_bezier_t
{
    vec3_t points[3];
    
} cubic_bezier_t;

typedef struct spline_t
{    
    vec3_t position;
    vec3_t scale;
    vec3_t rotation;
    
    uint8_t num_points;
    vec3_t* points;
//    union
//    {
//        cubic_bezier_t* curves;
//        vec3_t* points;
//    };
    
} spline_t;

spline_t* qb_spline_from_random_elipse ( vec3_t extents, uint8_t num_points, vec_t min_dist );
void qb_spline_render ( context_t* ctx, spline_t* spline );
void qb_spline_interpolate ( spline_t* spline, vec3_t out, float t );

#endif



