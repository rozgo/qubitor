#include "model.h"

const uint8_t QB_CTX_TYPE_MODEL = 21;

model_context_t* qb_model_context_create ( void )
{
    model_context_t* mtx = malloc ( sizeof ( model_context_t ) );
    assert ( mtx );
    memset ( mtx, 0, sizeof ( model_context_t ) );
    qb_context_init ( &mtx->ctx );
    mtx->ctx.ctx_type = QB_CTX_TYPE_MODEL;
    return mtx;
}

void qb_model_setup ( context_t* ctx )
{    
    ctx->ortho_extents_trail = 11;
    ctx->ortho_aabb.extents[0] = ctx->ortho_extents_trail;
}

void qb_model_render ( context_t* ctx )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_MODEL );
    model_context_t* mtx = ( model_context_t* )ctx;
    
    static float render_time = 0;
    float dt = qb_timer_elapsed () - render_time;
    if ( dt > 0.03f ) dt = 0.03f;
    render_time = qb_timer_elapsed ();
    
    if ( mtx->timer > 0 )
    {
        mtx->timer -= dt;
    }
    else
    {
        return;
    }
    
    glViewport ( ctx->viewport[0], ctx->viewport[1], ctx->viewport[2], ctx->viewport[3] );
    
    float ortho_dif = ctx->ortho_aabb.extents[0] - ctx->ortho_extents_trail;
    float ortho_vel = ortho_dif * 10 * dt;
    ctx->ortho_extents_trail += ortho_vel;
    
    float view_ratio = ( float )( ctx->viewport[3] - ctx->viewport[1] ) /
    ( float )( ctx->viewport[2] - ctx->viewport[0] );
    
    aabb_t ortho_trail;
    ortho_trail.extents[0] = ctx->ortho_extents_trail;
    ortho_trail.extents[1] = ortho_trail.extents[0] * view_ratio;
    ortho_trail.extents[2] = 40;
    ortho_trail.origin[0] = 0;
    ortho_trail.origin[1] = 0;
    ortho_trail.origin[2] = -ortho_trail.extents[2];   
    
    //printf("ortho: %f\n", ortho_trail.extents[0]);
    
    //ctx->ortho_aabb.extents[0] = ctx->ortho_extents_trail;
    ctx->ortho_aabb.extents[1] = ctx->ortho_aabb.extents[0] * view_ratio;
    ctx->ortho_aabb.extents[2] = 40;
    ctx->ortho_aabb.origin[0] = 0;
    ctx->ortho_aabb.origin[1] = 0;
    ctx->ortho_aabb.origin[2] = -ctx->ortho_aabb.extents[2];
    
    m4x4_ortho ( ctx->proj_mat,
                -ortho_trail.extents[0], ortho_trail.extents[0],
                -ortho_trail.extents[1], ortho_trail.extents[1],
                0, ortho_trail.extents[2] * 2 );
    
    vec3_t eye = { 0, 0, 20 };
    vec3_t look = { 0, 0, 0 };
    vec3_t up = { 0, 1, 0 };
    
    ctx->camera_rot_trail[0] += ( ctx->camera_rot[0] - ctx->camera_rot_trail[0] ) * dt * 10;
    ctx->camera_rot_trail[1] += ( ctx->camera_rot[1] - ctx->camera_rot_trail[1] ) * dt * 10;
    ctx->camera_rot_trail[2] += ( ctx->camera_rot[2] - ctx->camera_rot_trail[2] ) * dt * 10;
    
    vec3_t rot = { ctx->camera_rot_trail[0], ctx->camera_rot_trail[1], 0 };
    
    m4x4_look_at ( ctx->view_mat, eye, look, up );
    m4x4_rotate_by_vec3 ( ctx->view_mat, rot, eZYX );
    
    m4x4_identity ( ctx->view_proj_mat );
    m4x4_multiply_by_m4x4 ( ctx->view_proj_mat, ctx->proj_mat );
    m4x4_multiply_by_m4x4 ( ctx->view_proj_mat, ctx->view_mat );
    
    qb_qube_render_solid ( ctx, ctx->octree_root, 1 );
    //qb_qube_render_wired ( ctx, ctx->octree_root, 1 );
    qb_cuboid_render ( ctx );
    //qb_octant_render ( ctx, ctx->octree_root, 1 );
}

void qb_model_set ( context_t* ctx, octant_t* octant )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_MODEL );
    model_context_t* mtx = ( model_context_t* )ctx;
    
    if ( octant )
    {
        mtx->timer = FLT_MAX;
        
        if ( mtx->model )
        {
            if ( mtx->limits )
            {
                mtx->limits->timer = -1;
            }
            qb_octant_collapse ( ctx->octree_root );
            mtx->model = 0;
            //printf ( "octant count: %i\n", ctx->octants_count );
        }
        
        aabb_t aabb = { 0, 0, 0, 8, 8, 8 };
        color_t gray = { 200, 200, 200, 255 };
        mtx->limits = qb_cuboid_draw(ctx, &aabb, gray, 1, -1);
        
        mtx->model = octant;
        
        for ( int y = 0; y < 16; ++y )
        {
            for ( int z = 0; z < 16; ++z )
            {
                for ( int x = 0; x < 16; ++x )
                {
                    if ( mtx->model->qube->qbits[y][z][x][3] != 0 )
                    {
                        vec3_t pos;
                        pos[0] = 8.5f - ( x + 1 );
                        pos[1] = 8.5f - ( 16 - y );
                        pos[2] = 8.5f - ( 16 - z );
                        
                        octant_t* bit_octant = 0;
                        qb_octant_expand ( ctx->octree_root, pos, &bit_octant );
                        assert ( bit_octant != 0 );
                        
                        bit_octant->qube = ( qube_t* )1;
                        //bit_octant->line_width = 1;
                        
                        bit_octant->color[0] = mtx->model->qube->qbits[y][z][x][0];
                        bit_octant->color[1] = mtx->model->qube->qbits[y][z][x][1];
                        bit_octant->color[2] = mtx->model->qube->qbits[y][z][x][2];
                        bit_octant->color[3] = mtx->model->qube->qbits[y][z][x][3];
                    }
                }
            }
        }
        //printf ( "octant count: %i\n", ctx->octants_count );
    }
    else
    {
        mtx->timer = 0.5f;
        
        if ( mtx->model && mtx->model->qube )
        {
            for ( int y = 0; y < 16; ++y )
            {
                for ( int z = 0; z < 16; ++z )
                {
                    for ( int x = 0; x < 16; ++x )
                    {
                        glDeleteBuffers ( 0, &mtx->model->qube->gl_id );
                        mtx->model->qube->gl_id = 0;
                        mtx->model->qube->gl_count = 0;
                        
                        mtx->model->qube->qbits[y][z][x][0] = 0;
                        mtx->model->qube->qbits[y][z][x][1] = 0;
                        mtx->model->qube->qbits[y][z][x][2] = 0;
                        mtx->model->qube->qbits[y][z][x][3] = 0;
                        
                        vec3_t pos;
                        pos[0] = 8.5f - ( x + 1 );
                        pos[1] = 8.5f - ( 16 - y );
                        pos[2] = 8.5f - ( 16 - z );
                        
                        octant_t* bit_octant = 0;
                        qb_octant_intersect_point ( ctx->octree_root, pos, &bit_octant );
                        if ( bit_octant )
                        {
                            mtx->model->qube->qbits[y][z][x][0] = bit_octant->color[0];
                            mtx->model->qube->qbits[y][z][x][1] = bit_octant->color[1];
                            mtx->model->qube->qbits[y][z][x][2] = bit_octant->color[2];
                            mtx->model->qube->qbits[y][z][x][3] = bit_octant->color[3];
                        }
                    }
                }
            }
        }
    }
}

void qb_model_zoom_end ( context_t* ctx )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_MODEL );
    model_context_t* mtx = ( model_context_t* )ctx;
    
    if ( ctx->ortho_aabb.extents[0] < 10 )
    {
        ctx->ortho_aabb.extents[0] = 10;
    }
    else if ( mtx->timer < 1.0f )
    {
    }
    else if ( ctx->ortho_aabb.extents[0] > 24 )
    {
        ctx->ortho_aabb.extents[0] = 24;
    }
}
