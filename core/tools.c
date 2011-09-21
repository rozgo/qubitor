// author: rozgo

#include "tools.h"
#include "model.h"
#include "gamut.h"

const uint8_t QB_CTX_TYPE_TOOLS = 87;

tools_context_t* qb_tools_context_create ( void )
{
    tools_context_t* ttx = malloc ( sizeof ( tools_context_t ) );
    assert ( ttx );
    memset ( ttx, 0, sizeof ( tools_context_t ) );
    qb_context_init ( &ttx->ctx );
    ttx->ctx.ctx_type = QB_CTX_TYPE_TOOLS;
    return ttx;
}

static void copy_grayscaled ( GLubyte src[16][16][16][4], GLubyte dest[16][16][16][4] )
{
    for ( int y = 0; y < 16; ++y )
    {
        for ( int z = 0; z < 16; ++z )
        {
            for ( int x = 0; x < 16; ++x )
            {
                vec_t lum =
                ( vec_t )src[y][z][x][0] * 0.299f +
                ( vec_t )src[y][z][x][1] * 0.587f + 
                ( vec_t )src[y][z][x][2] * 0.114f;
                dest[y][z][x][0] = lum;
                dest[y][z][x][1] = lum;
                dest[y][z][x][2] = lum;
                dest[y][z][x][3] = src[y][z][x][3];
            }
        }
    }
}

void qb_tools_setup ( context_t* ctx )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_TOOLS );
    tools_context_t* ttx = ( tools_context_t* )ctx;
    
    float view_ratio = ( float )( ctx->viewport[3] - ctx->viewport[1] ) /
    ( float )( ctx->viewport[2] - ctx->viewport[0] );
    
    ctx->ortho_aabb.extents[0] = 8;
    ctx->ortho_aabb.extents[1] = ctx->ortho_aabb.extents[0] * view_ratio;
    ctx->ortho_aabb.extents[2] = 10;
    ctx->ortho_aabb.origin[0] = ctx->ortho_aabb.extents[0];
    ctx->ortho_aabb.origin[1] = ctx->ortho_aabb.extents[1];
    ctx->ortho_aabb.origin[2] = -ctx->ortho_aabb.extents[2];
    
    vec3_t pos;
    qube_t* clone;
    
    ttx->buttons[0] = qb_qube_from_image ( "assets/tools" );
    VectorSet ( pos, 0.5f, 0.5f, 0.5f );
    qb_octant_expand ( ctx->octree_root, pos, &ttx->octants[0] );
    assert ( ttx->octants[0] );
    ttx->octants[0]->qube = ttx->buttons[0];
    clone = qb_qubes_get_free ();
    assert ( clone );
    copy_grayscaled ( ttx->buttons[0]->qbits, clone->qbits );        
    ttx->buttons[NUM_TOOLS_BUTTONS + 0] = clone;
    
    for ( int i = 1; i < NUM_TOOLS_BUTTONS; ++i )
    {
        VectorSet ( pos, 0.5f + i, 0.5f, 0.5f );
        ttx->buttons[i] = ttx->buttons[i-1]->next_frame;
        qb_octant_expand ( ctx->octree_root, pos, &ttx->octants[i] );
        assert ( ttx->octants[i] );
        ttx->octants[i]->qube = ttx->buttons[i];
        clone = qb_qubes_get_free ();
        assert ( clone );
        copy_grayscaled ( ttx->buttons[i]->qbits, clone->qbits );        
        ttx->buttons[NUM_TOOLS_BUTTONS + i] = clone;
    }
    
    qb_tools_mode_sel ( ctx );
    qb_tools_library ( ctx, 0 );
    qb_tools_palette ( ctx, 0 );
}

void qb_tools_world_select ( context_t* ctx, octant_t* octant )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_TOOLS );
    tools_context_t* ttx = ( tools_context_t* )ctx;
    
    if ( ttx->world_selected )
    {
        octant_t* next = ttx->world_selected;
        while ( next )
        {
            if ( next == octant ) return;
            next = next->next_selected;
        }
        octant->next_selected = ttx->world_selected;
    }
    
    ttx->world_selected = octant;
    
    vec3_t min_point = { FLT_MAX, FLT_MAX, FLT_MAX };
    vec3_t max_point = { FLT_MIN, FLT_MIN, FLT_MIN };
    octant_t* chain = ttx->world_selected;
    do
    {
        for ( int i=0; i<3; ++i )
        {
            if ( ( chain->aabb.origin[i] - chain->aabb.extents[i] ) < min_point[i] )
            {
                min_point[i] = chain->aabb.origin[i] - chain->aabb.extents[i];
            }
            if ( ( chain->aabb.origin[i] + chain->aabb.extents[i] ) > max_point[i] )
            {
                max_point[i] = chain->aabb.origin[i] + chain->aabb.extents[i];
            }
        }
        chain = chain->next_selected;
    }
    while ( chain );
    
    color_t bright_green = { 0, 255, 0, 255 };
    if ( ttx->world_selected_glow != 0 )
    {
        ttx->world_selected_glow->timer = 0.1f;
    }
    if ( octant )
    {
        aabb_t aabb = { 
            min_point[0] + ( max_point[0] - min_point[0] ) / 2,
            min_point[1] + ( max_point[1] - min_point[1] ) / 2,
            min_point[2] + ( max_point[2] - min_point[2] ) / 2,
            ( max_point[0] - min_point[0] ) / 2,
            ( max_point[1] - min_point[1] ) / 2,
            ( max_point[2] - min_point[2] ) / 2
        };
        ttx->world_selected_glow = qb_cuboid_draw ( ttx->world_ctx, &aabb, bright_green, 3, -1 );
    }
}

void qb_tools_model_select ( context_t* ctx, octant_t* octant )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_TOOLS );
    tools_context_t* ttx = ( tools_context_t* )ctx;
    
    ttx->model_selected = octant;
    color_t bright_green = { 0, 255, 0, 255 };
    if ( ttx->model_selected_glow != 0 )
    {
        ttx->model_selected_glow->timer = 0.1f;
    }
    if ( octant )
    {
        ttx->model_selected_glow = qb_cuboid_draw ( ttx->model_ctx, &octant->aabb, bright_green, 3, -1 );
    }
}

void qb_tools_world_deselect ( context_t* ctx, octant_t* octant )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_TOOLS );
    tools_context_t* ttx = ( tools_context_t* )ctx;
    
    octant_t* prev = octant;
    while ( prev ) 
    {
        octant_t* next = prev->next_selected;
        prev->next_selected = 0;
        prev = next;
    }
    ttx->world_selected = 0;
    
    if ( ttx->world_selected_glow != 0 )
    {
        ttx->world_selected_glow->timer = 0.1f;
    }
    ttx->world_selected_glow = 0;
}

void qb_tools_model_deselect ( context_t* ctx, octant_t* octant )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_TOOLS );
    tools_context_t* ttx = ( tools_context_t* )ctx;
    
    if ( octant != 0 && octant != ttx->model_selected )
    {
        return;
    }
    ttx->model_selected = 0;
    if ( ttx->model_selected_glow != 0 )
    {
        ttx->model_selected_glow->timer = 0.1f;
    }
    ttx->model_selected_glow = 0;
}

void qb_tools_pick ( context_t* ctx, octant_t* octant )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_TOOLS );
    tools_context_t* ttx = ( tools_context_t* )ctx;
    
    if ( octant == ttx->octants[TOOLS_BTN_SEL] )
    {
        qb_tools_mode_sel ( ctx );
    }
    else if ( octant == ttx->octants[TOOLS_BTN_ADD] )
    {
        qb_tools_mode_add ( ctx );
    }
    else if ( octant == ttx->octants[TOOLS_BTN_DEL] )
    {
        qb_tools_mode_del ( ctx );
    }
    else if ( octant == ttx->octants[TOOLS_BTN_ROT] )
    {
        qb_tools_mode_rot ( ctx );
    }
    else if ( octant == ttx->octants[TOOLS_BTN_LIB] )
    {
        qb_tools_library ( ctx, !ttx->library_on );
    }
    else if ( octant == ttx->octants[TOOLS_BTN_PAL] )
    {
        qb_tools_palette ( ctx, !ttx->palette_on );
    }
}

void qb_tools_mode_sel ( context_t* ctx )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_TOOLS );
    tools_context_t* ttx = ( tools_context_t* )ctx;
    
    for ( int i = 0; i < 5; ++i )
    {
        ttx->octants[i]->qube = ttx->buttons[NUM_TOOLS_BUTTONS + i];
    }
    ttx->octants[TOOLS_BTN_SEL]->qube = ttx->buttons[TOOLS_BTN_SEL];
    ttx->mode = TOOLS_MODE_SEL;
}

void qb_tools_mode_add ( context_t* ctx )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_TOOLS );
    tools_context_t* ttx = ( tools_context_t* )ctx;
    
    for ( int i = 0; i < 5; ++i )
    {
        ttx->octants[i]->qube = ttx->buttons[NUM_TOOLS_BUTTONS + i];
    }
    ttx->octants[TOOLS_BTN_ADD]->qube = ttx->buttons[TOOLS_BTN_ADD];
    ttx->mode = TOOLS_MODE_ADD;
}

void qb_tools_mode_del ( context_t* ctx )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_TOOLS );
    tools_context_t* ttx = ( tools_context_t* )ctx;
    
    for ( int i = 0; i < 5; ++i )
    {
        ttx->octants[i]->qube = ttx->buttons[NUM_TOOLS_BUTTONS + i];
    }
    ttx->octants[TOOLS_BTN_DEL]->qube = ttx->buttons[TOOLS_BTN_DEL];
    ttx->mode = TOOLS_MODE_DEL;
}

void qb_tools_mode_rot ( context_t* ctx )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_TOOLS );
    tools_context_t* ttx = ( tools_context_t* )ctx;
    
    for ( int i = 0; i < 5; ++i )
    {
        ttx->octants[i]->qube = ttx->buttons[NUM_TOOLS_BUTTONS + i];
    }
    ttx->octants[TOOLS_BTN_ROT]->qube = ttx->buttons[TOOLS_BTN_ROT];
    ttx->mode = TOOLS_MODE_ROT;
}

void qb_tools_library ( context_t* ctx, uint8_t show )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_TOOLS );
    tools_context_t* ttx = ( tools_context_t* )ctx;
    
    if ( show )
    {
        ttx->octants[TOOLS_BTN_LIB]->qube = ttx->buttons[TOOLS_BTN_LIB];
        ttx->library_on = 1;
    }
    else
    {
        ttx->octants[TOOLS_BTN_LIB]->qube = ttx->buttons[NUM_TOOLS_BUTTONS + TOOLS_BTN_LIB];
        ttx->library_on = 0;        
    }
}

void qb_tools_palette ( context_t* ctx, uint8_t show )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_TOOLS );
    tools_context_t* ttx = ( tools_context_t* )ctx;
    
    float view_ratio = ( float )( ctx->viewport[3] - ctx->viewport[1] ) /
    ( float )( ctx->viewport[2] - ctx->viewport[0] );
    
    if ( show )
    {
        ttx->octants[TOOLS_BTN_PAL]->qube = ttx->buttons[TOOLS_BTN_PAL];
        ttx->palette_on = 1;
        if ( ttx->world_selected )
        {
            VectorCopy ( ttx->world_ctx->camera_rot, ttx->model_ctx->camera_rot );
            VectorIncrement ( ttx->world_selected->rotation, ttx->model_ctx->camera_rot );
            VectorCopy ( ttx->model_ctx->camera_rot, ttx->model_ctx->camera_rot_trail );
            
            ttx->model_ctx->ortho_extents_trail = ttx->world_ctx->ortho_aabb.extents[0] * 16;
            ttx->model_ctx->ortho_aabb.extents[0] = 11;
            
            qb_model_set ( ttx->model_ctx, ttx->world_selected );
        }
    }
    else
    {
        ttx->octants[TOOLS_BTN_PAL]->qube = ttx->buttons[NUM_TOOLS_BUTTONS + TOOLS_BTN_PAL];
        ttx->palette_on = 0;
        
        ttx->model_ctx->ortho_aabb.extents[0] = ttx->world_ctx->ortho_aabb.extents[0] * 16;
        ttx->model_ctx->ortho_aabb.extents[1] = ttx->world_ctx->ortho_aabb.extents[0] * 16 * view_ratio;
        
        qb_model_set ( ttx->model_ctx, 0 );
        qb_tools_model_deselect ( ctx, ttx->model_selected );
        qb_tools_mode_sel ( ctx );
    }
}

extern int qb_qube_render_count;

void qb_tools_render ( context_t* ctx )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_TOOLS );
    tools_context_t* ttx = (tools_context_t*)ctx;
    
    if ( ttx->palette_on )
    {
        glClear ( GL_DEPTH_BUFFER_BIT );
        qb_gamut_render ( ttx->gamut_ctx );
    }
    
    glViewport ( ctx->viewport[0], ctx->viewport[1], ctx->viewport[2], ctx->viewport[3] );

    m4x4_ortho ( ctx->proj_mat, 
                0, ctx->ortho_aabb.extents[0] * 2, 
                0, ctx->ortho_aabb.extents[1] * 2,
                0, ctx->ortho_aabb.extents[2] * 2 );
    
    vec3_t eye = { 0, 0, 10 };
    vec3_t look = { 0, 0, 0 };
    vec3_t up = { 0, 1, 0 };
    m4x4_look_at ( ctx->view_mat, eye, look, up );
    
    m4x4_identity ( ctx->view_proj_mat );
    m4x4_multiply_by_m4x4 ( ctx->view_proj_mat, ctx->proj_mat );
    m4x4_multiply_by_m4x4 ( ctx->view_proj_mat, ctx->view_mat );
    
    qb_qube_render_count = 0;
    qb_qube_render ( ctx, ctx->octree_root, 1 );
    //printf ( "render_count: %i\n", qb_qube_render_count );
    qb_cuboid_render ( ctx );
    //qb_octant_render ( ctx );
}

void qb_tools_touch_began ( context_t* ctx, vec3_t screen_pos )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_TOOLS );
    tools_context_t* ttx = (tools_context_t*)ctx;
//    octant_t* octant = 0;
//    vec3_t plane;
    
    ttx->touch_ticks = 0;
    ttx->touch_picked = 0;
    VectorCopy ( screen_pos, ttx->touch_start );
    qb_pick_select ( ttx->world_ctx, screen_pos, &ttx->touch_picked, ttx->touch_plane );
}

void qb_tools_touch_moved ( context_t* ctx, vec3_t screen_pos )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_TOOLS );
    tools_context_t* ttx = (tools_context_t*)ctx;
    octant_t* octant = 0;
    vec3_t plane;
    
    ttx->touch_moved = 1;
    
    float w = 40 - ttx->world_ctx->ortho_aabb.extents[0];
    int threshold = 0;
    vec3_t touch_dt;
    VectorSubtract ( screen_pos, ttx->touch_start, touch_dt );
    touch_dt[1] *= -1;
    float move_dist = VectorNormalize ( touch_dt, touch_dt );
    
    vec3_t world_start;
    vec3_t world_end;
    vec3_t world_dt;
    qb_screen_to_world ( ttx->world_ctx, world_start, ttx->touch_start );
    qb_screen_to_world ( ttx->world_ctx, world_end, screen_pos );
    VectorSubtract ( world_end, world_start, world_dt );
    VectorNormalize ( world_dt, world_dt );
    
    if ( move_dist / w > ttx->touch_ticks && DotProduct ( ttx->touch_plane, world_dt ) > 0.3f )
    {
        threshold = 1;
        ++ttx->touch_ticks;
    }
    
    if ( ttx->touch_picked )
    {
        if ( ttx->mode == TOOLS_MODE_SEL ) {
            if ( !ttx->palette_on )
            {
                qb_pick_select ( ttx->world_ctx, screen_pos, &octant, plane );
                if ( octant )
                {
                    qb_tools_world_select ( ctx, octant );
                }
            }
            else
            {
                qb_pick_select ( ttx->model_ctx, screen_pos, &octant, plane );
                qb_tools_model_select ( ctx, octant );
            }
        }
        else if ( ttx->mode == TOOLS_MODE_ADD ) {
            if ( !ttx->palette_on )
            {
                if ( threshold )
                {
                    qb_octant_extrude_plane ( ttx->world_ctx, ttx->touch_picked, ttx->touch_plane, &octant );
                    //qb_pick_extrude ( ttx->world_ctx, screen_pos, &octant, plane );
                    if ( octant )
                    {
                        qb_tools_world_select ( ctx, octant );
                    }
                    ttx->touch_picked = ttx->world_selected;
                }
            }
            else
            {
                qb_pick_extrude ( ttx->model_ctx, screen_pos, &octant, plane );
                if ( octant )
                {
                    qb_tools_model_select ( ctx, octant );
                }
            }
        }
        else if ( ttx->mode == TOOLS_MODE_DEL ) {
            if ( !ttx->palette_on )
            {
                qb_pick_select ( ttx->world_ctx, screen_pos, &octant, plane );
                if ( octant )
                {
                    qb_tools_world_deselect ( ctx, octant );
                    qb_octant_collapse ( octant );
                    //printf ( "octant count: %i\n", ttx->world_ctx->octants_count );
                }
            }
            else
            {
                qb_pick_select ( ttx->model_ctx, screen_pos, &octant, plane );
                if ( octant )
                {
                    qb_tools_model_deselect ( ctx, octant );
                    qb_octant_collapse ( octant );
                }
            }
        }
    }
}

void qb_tools_touch_ended ( context_t* ctx, vec3_t screen_pos )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_TOOLS );
    tools_context_t* ttx = (tools_context_t*)ctx;
    octant_t* octant = 0;
    vec3_t plane;
    
    if ( !ttx->touch_moved )
    {
        if ( qb_pick_select ( ctx, screen_pos, &octant, plane ) ) {
            qb_tools_pick ( ctx, octant );
        }
        else if ( ttx->mode == TOOLS_MODE_SEL ) {
            if ( !ttx->palette_on )
            {
                qb_tools_world_deselect ( ctx, ttx->world_selected );
                qb_pick_select ( ttx->world_ctx, screen_pos, &octant, plane );
                if ( octant )
                {
                    qb_tools_world_select ( ctx, octant );
                    VectorCopy ( octant->aabb.origin, ttx->world_ctx->camera_target );
                }
            }
            else
            {
                qb_pick_select ( ttx->model_ctx, screen_pos, &octant, plane );
                qb_tools_model_select ( ctx, octant );
            }
        }
        else if ( ttx->mode == TOOLS_MODE_ADD ) {
            if ( !ttx->palette_on )
            {
                qb_pick_extrude ( ttx->world_ctx, screen_pos, &octant, ttx->world_selected_plane );
                if ( octant )
                {
                    //VectorCopy ( octant->aabb.origin, ttx->world_ctx->camera_target );
                    qb_tools_world_deselect ( ctx, ttx->world_selected );
                    qb_tools_world_select ( ctx, octant );
                }
            }
            else
            {
                qb_pick_extrude ( ttx->model_ctx, screen_pos, &octant, plane );
                if ( octant )
                {
                    qb_tools_model_select ( ctx, octant );
                }
            }
        }
        else if ( ttx->mode == TOOLS_MODE_DEL ) {
            if ( !ttx->palette_on )
            {
                qb_pick_select ( ttx->world_ctx, screen_pos, &octant, plane );
                if ( octant )
                {
                    qb_tools_world_deselect ( ctx, octant );
                    qb_octant_collapse ( octant );
                    //printf ( "octant count: %i\n", ttx->world_ctx->octants_count );
                }
            }
            else
            {
                qb_pick_select ( ttx->model_ctx, screen_pos, &octant, plane );
                if ( octant )
                {
                    qb_tools_model_deselect ( ctx, octant );
                    qb_octant_collapse ( octant );
                }
            }
        }
    }
    
    ttx->touch_moved = 0;
}

void qb_tools_on_gamut ( context_t* ctx, vec3_t screen_pos )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_TOOLS );
    tools_context_t* ttx = ( tools_context_t* )ctx;

    if ( ttx->model_selected )
    {
        qb_gamut_pick ( ttx->gamut_ctx, screen_pos, ttx->model_selected->color );
    }

}

