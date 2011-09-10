#include "qb.h"

vec_t* color_normalize ( const color_t in_color, vec4_t out_color )
{
    out_color[0] = in_color[0] / 255.0f;
    out_color[1] = in_color[1] / 255.0f;
    out_color[2] = in_color[2] / 255.0f;
    out_color[3] = in_color[3] / 255.0f;
    return out_color;
}

