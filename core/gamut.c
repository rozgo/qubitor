// author: rozgo

#include "gamut.h"

const uint8_t QB_CTX_TYPE_GAMUT = 56;

gamut_context_t* qb_gamut_context_create ( void )
{
    gamut_context_t* gtx = malloc ( sizeof ( gamut_context_t ) );
    assert ( gtx );
    memset ( gtx, 0, sizeof ( gamut_context_t ) );
    qb_context_init ( &gtx->ctx );
    gtx->ctx.ctx_type = QB_CTX_TYPE_GAMUT;
    return gtx;
}

void qb_gamut_setup ( context_t* ctx )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_GAMUT );
    gamut_context_t* gtx = ( gamut_context_t* )ctx;
    
    float view_ratio = ( float )( ctx->viewport[3] - ctx->viewport[1] ) /
    ( float )( ctx->viewport[2] - ctx->viewport[0] );
    
    ctx->point_size = 10;
    
    ctx->ortho_aabb.extents[1] = 2;
    ctx->ortho_aabb.extents[0] = ctx->ortho_aabb.extents[1] / view_ratio;
    ctx->ortho_aabb.extents[2] = 10;
    ctx->ortho_aabb.origin[0] = ctx->ortho_aabb.extents[0];
    ctx->ortho_aabb.origin[1] = ctx->ortho_aabb.extents[1];
    ctx->ortho_aabb.origin[2] = -ctx->ortho_aabb.extents[2];
    
    vec3_t pos = { 0.5f, ctx->ortho_aabb.extents[1] * 2 - 0.5f , 0.5f };
    gtx->gamut_octant = 0;
    
    qb_octant_expand ( ctx->octree_root, pos, &gtx->gamut_octant );
    assert ( gtx->gamut_octant );
    VectorSet ( gtx->gamut_octant->scale, 0.6f, 0.6f, 0.6f );
    VectorSet ( gtx->gamut_octant->rotation, -35.264f, 45 * 3, 0 );
    
    gtx->gamut_octant->qube = qb_qubes_get_free ();
    
    for ( int y = 0; y < 16; ++y )
    {
        for ( int z = 0; z < 16; ++z )
        {
            for ( int x = 0; x < 16; ++x )
            {
                if ( x > 0 && x < 15 && y > 0 && y < 15 && z > 0 && z < 15 ) continue;
                
                GLubyte r = x * 16;
                GLubyte g = y * 16;
                GLubyte b = z * 16;
                
                gtx->gamut_octant->qube->gl_id = 0;
                gtx->gamut_octant->qube->gl_count = 0;
                
                gtx->gamut_octant->qube->qbits[y][z][x][0] = r;
                gtx->gamut_octant->qube->qbits[y][z][x][1] = g;
                gtx->gamut_octant->qube->qbits[y][z][x][2] = b;
                gtx->gamut_octant->qube->qbits[y][z][x][3] = 255;
            }
        }
    }
}

extern int qb_qube_render_count;

void qb_gamut_render ( context_t* ctx )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_GAMUT );
    //gamut_context_t* gtx = ( gamut_context_t* )ctx;
    
    static float render_time = 0;
    float dt = qb_timer_elapsed () - render_time;
    if ( dt > 0.03f ) dt = 0.03f;
    render_time = qb_timer_elapsed ();
    if ( dt > 0.1f ) dt = 1;
    
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
    //qb_octant_render ( ctx, ctx->octree_root, 1 );
}

void qb_gamut_pick ( context_t* ctx, vec3_t screen_pos, color_t color )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_GAMUT );
    gamut_context_t* gtx = ( gamut_context_t* )ctx;
    
    float closest_dist = FLT_MAX;
    vec3_t world_pick;
    
    float d = 1 / 16.0f;
    
    world_pick[0] = ( screen_pos[0] / ctx->viewport[2] ) * ( ctx->ortho_aabb.extents[0] * 2 );
    world_pick[1] = ( screen_pos[1] / ctx->viewport[3] ) * ( ctx->ortho_aabb.extents[1] * 2 );
    world_pick[2] = 5;
    
//    aabb_t aabb2 = { world_pick[0], world_pick[1], world_pick[2], 0.5f, 0.5f, 0.5f };
//    color_t cb = { 255, 255, 0, 255 };
//    qb_cuboid_draw ( ctx, &aabb2, cb, 1, 1 );
    
    for ( int y = 0; y < 16; ++y )
    {
        for ( int z = 0; z < 16; ++z )
        {
            for ( int x = 0; x < 16; ++x )
            {
                if ( x > 0 && x < 15 && y > 0 && y < 15 && z > 0 && z < 15 ) continue;
                
                vec3_t qbit_pos = { ( ( x ) * d + d / 2 ) - 0.5f, ( ( y ) * d + d / 2 ) - 0.5f, ( ( z ) * d + d / 2 ) - 0.5f };
                m4x4_t xform;
                m4x4_identity ( xform );
                
                m4x4_translate_by_vec3 ( xform, gtx->gamut_octant->aabb.origin );
                m4x4_rotate_by_vec3 ( xform, gtx->gamut_octant->rotation, eZYX );
                m4x4_scale_by_vec3 ( xform, gtx->gamut_octant->scale );
                m4x4_transform_point ( xform, qbit_pos );
                
//                aabb_t aabb = { qbit_pos[0], qbit_pos[1], qbit_pos[2], d * 0.3f, d * 0.3f, d * 0.3f };
//                color_t c = { 0, 0, 255, 255 };
//                qb_cuboid_draw ( ctx, &aabb, c, 1, 1 );
                
                float r = d;
                if ( ( world_pick[0] - r ) < qbit_pos[0] && ( world_pick[0] + r ) > qbit_pos[0] && 
                     ( world_pick[1] - r ) < qbit_pos[1] && ( world_pick[1] + r ) > qbit_pos[1] )
                {
                    if ( -qbit_pos[2] < closest_dist )
                    {
                        closest_dist = -qbit_pos[2];
                        color[0] = gtx->gamut_octant->qube->qbits[y][z][x][0];
                        color[1] = gtx->gamut_octant->qube->qbits[y][z][x][1];
                        color[2] = gtx->gamut_octant->qube->qbits[y][z][x][2];
                        color[3] = 255;
                    }
                }
            }
        }
    }
}

