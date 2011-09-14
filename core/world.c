// author: rozgo

#include "world.h"
#include "spline.h"

const uint8_t QB_CTX_TYPE_WORLD = 12;

world_context_t* qb_world_context_create ( void )
{
    world_context_t* wtx = malloc ( sizeof ( world_context_t ) );
    assert ( wtx );
    memset ( wtx, 0, sizeof ( world_context_t ) );
    qb_context_init ( &wtx->ctx );
    wtx->ctx.ctx_type = QB_CTX_TYPE_WORLD;
    return wtx;
}

void qb_world_setup ( context_t* ctx )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_WORLD );
    world_context_t* wtx = ( world_context_t* )ctx;
    
    VectorSet ( ctx->camera_rot, -35.264f, 45 * 1, 0 );
    VectorCopy ( ctx->camera_rot, ctx->camera_rot_trail );
    VectorSet ( ctx->camera_target, 0.5f, 0.5f, 0.5f );
    VectorCopy ( ctx->camera_target, ctx->camera_target_trail );
    
    ctx->ortho_aabb.extents[0] = 12;
    ctx->point_size = 3;
    
    ctx->ortho_extents_trail = 4;
    //ctx->ortho_aabb.extents[0] = ctx->ortho_extents_trail;
    
    vec3_t pos;
    octant_t* octant;
    qube_t* qube;
    
    VectorSet ( pos, 0.5f, 0.5f, 0.5f );
    qube = qb_qube_from_image ( "assets/king" );
    qb_octant_expand ( ctx->octree_root, pos, &octant );
    assert ( octant );
    octant->color[0] = 1;
    octant->qube = qube;

    pos[0] += 2;
    qube = qb_qube_from_image ( "assets/megaman" );
    qb_octant_expand ( ctx->octree_root, pos, &octant );
    assert ( octant );
    octant->qube = qube;
    pos[0] -= 2;
    
    pos[2] += 2;
    qube = qb_qube_from_image ( "assets/link" );
    qb_octant_expand ( ctx->octree_root, pos, &octant );
    assert ( octant );
    octant->qube = qube;
    
    pos[0] += 2;
    qube = qb_qube_from_image ( "assets/bride" );
    qb_octant_expand ( ctx->octree_root, pos, &octant );
    assert ( octant );
    octant->qube = qube;
    pos[0] -= 2;
    
    pos[2] += 2;
    qube = qb_qube_from_image ( "assets/hen" );
    qb_octant_expand ( ctx->octree_root, pos, &octant );
    assert ( octant );
    octant->qube = qube;
    octant->rotation[1] = 90;
    
    pos[0] += 2;
    qube = qb_qube_from_image ( "assets/cube" );
    qb_octant_expand ( ctx->octree_root, pos, &octant );
    assert ( octant );
    octant->qube = qube;
    pos[0] -= 2;
    
    pos[2] += 2;
    qube = qb_qube_from_image ( "assets/grass_soil" );
    qb_octant_expand ( ctx->octree_root, pos, &octant );
    assert ( octant );
    octant->qube = qube;
    octant->rotation[1] = 90;
    
//    qube = qb_qube_from_image ( "assets/grass" );
//    
//    int s = 8;
//    
//    for ( int x = -s; x <= s; ++x )
//    {
//        for ( int z = -s; z <= s; ++z )
//        {
//            pos[0] = x + 0.5f;
//            pos[1] = 1.5f;
//            pos[2] = z + 0.5f;
//            qb_octant_expand ( ctx->octree_root, pos, &octant );
//            assert ( octant );
//            octant->qube = qube;
//            octant->rotation[1] = rand() % 90 * 90;
//        }
//    }
    

    vec3_t extents = { 10, 2, 10 };
    wtx->spline = qb_spline_from_random_elipse ( extents, 45, 1 );
    wtx->spline->position[1] = -2;
    wtx->spline->rotation[1] = 90;
    
    wtx->skybox_gl_id[0] = qb_load_texture ( "assets/skybox_sides" );
    wtx->skybox_gl_id[1] = qb_load_texture ( "assets/skybox_top" );
    
    wtx->ground_gl_id = qb_load_texture ( "assets/ground" );
}


extern int qb_qube_render_count;

void qb_world_render ( context_t* ctx )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_WORLD );
    world_context_t* wtx = ( world_context_t* )ctx;
    
    //TODO: unstatic
    static float render_time = 0;
    float dt = qb_timer_elapsed () - render_time;
    if ( dt > 0.03f ) dt = 0.03f;
    render_time = qb_timer_elapsed ();
    
    static float spline_time = 0;
    spline_time += dt;
    
    for ( int i = 0; i < 10; ++i )
    {
        float t = spline_time * 0.1f + i * 0.01f;
        float int_part;
        t = modff ( t, &int_part );
        
        aabb_t cursor = { 0, 0, 0, 0.2f, 0.2f, 0.2f };
        color_t red = { 255, 0, 0, 255 };
        qb_spline_interpolate ( wtx->spline, cursor.origin, t );
        
        m4x4_t xform;
        m4x4_identity ( xform );
        m4x4_translate_by_vec3 ( xform, wtx->spline->position );
        m4x4_rotate_by_vec3 ( xform, wtx->spline->rotation, eZYX );
        m4x4_scale_by_vec3 ( xform, wtx->spline->scale );
        m4x4_transform_point( xform, cursor.origin);
        
        qb_cuboid_draw ( ctx, &cursor, red, 5, 0 );        
    }
    
    float ortho_dif = ctx->ortho_aabb.extents[0] - ctx->ortho_extents_trail;
    float ortho_vel = ortho_dif * 10 * dt;
    ctx->ortho_extents_trail += ortho_vel;
    
    float view_ratio = ( float )( ctx->viewport[3] - ctx->viewport[1] ) /
    ( float )( ctx->viewport[2] - ctx->viewport[0] );
    
    aabb_t ortho_trail;
    ortho_trail.extents[0] = ctx->ortho_extents_trail;
    ortho_trail.extents[1] = ortho_trail.extents[0] * view_ratio;
    ortho_trail.extents[2] = 100;
    ortho_trail.origin[0] = 0;
    ortho_trail.origin[1] = 0;
    ortho_trail.origin[2] = -ortho_trail.extents[2];   
    
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
    
    ctx->point_size = ( 20 - ortho_trail.extents[0] ) * 0.45f;
    if ( ctx->point_size < 1 )
    {
        ctx->point_size = 1;
    }
    
    vec3_t eye = { 0, 0, 50 };
    vec3_t look = { 0, 0, 0 };
    vec3_t up = { 0, 1, 0 };
    
    ctx->camera_rot_trail[0] += ( ctx->camera_rot[0] - ctx->camera_rot_trail[0] ) * dt * 10;
    ctx->camera_rot_trail[1] += ( ctx->camera_rot[1] - ctx->camera_rot_trail[1] ) * dt * 10;
    ctx->camera_rot_trail[2] += ( ctx->camera_rot[2] - ctx->camera_rot_trail[2] ) * dt * 10;
    
    vec3_t rot = { ctx->camera_rot_trail[0], ctx->camera_rot_trail[1], 0 };
    
    m4x4_look_at ( ctx->view_mat, eye, look, up );
    m4x4_rotate_by_vec3 ( ctx->view_mat, rot, eZYX );
    
    vec3_t target_dir;
    VectorScale ( ctx->camera_target, -1, target_dir );
    VectorSubtract ( target_dir, ctx->camera_target_trail, target_dir );
    vec_t dist = VectorNormalize ( target_dir, target_dir );
    ctx->camera_target_trail[0] += target_dir[0] * dist * dt * 10;
    ctx->camera_target_trail[1] += target_dir[1] * dist * dt * 10;
    ctx->camera_target_trail[2] += target_dir[2] * dist * dt * 10;
    m4x4_translate_by_vec3( ctx->view_mat, ctx->camera_target_trail );
    
    m4x4_identity ( ctx->view_proj_mat );
    m4x4_multiply_by_m4x4 ( ctx->view_proj_mat, ctx->proj_mat );
    m4x4_multiply_by_m4x4 ( ctx->view_proj_mat, ctx->view_mat );
    
    vec3_t plane_pos;
    vec3_t plane_rot;
    vec3_t plane_sca = { 32, 32, 32 };
    vec3_t offset = { 32, 32, 0 };
    m4x4_t xform;
    
    VectorSet ( plane_pos, 0, offset[0], offset[1] );
    VectorSet ( plane_rot, 90, 0, 0 );
    m4x4_identity(xform);
    m4x4_rotate_by_vec3 ( xform, plane_rot, eZYX );
    m4x4_translate_by_vec3 ( xform, plane_pos );
    m4x4_scale_by_vec3 ( xform, plane_sca );
    m4x4_premultiply_by_m4x4 ( xform, ctx->view_proj_mat );
    qb_render_plane ( ctx, wtx->skybox_gl_id[0], xform );
    
    VectorSet ( plane_pos, 0, offset[0], offset[1] );
    VectorSet ( plane_rot, 90, 0, -180 );
    m4x4_identity(xform);
    m4x4_rotate_by_vec3 ( xform, plane_rot, eZYX );
    m4x4_translate_by_vec3 ( xform, plane_pos );
    m4x4_scale_by_vec3 ( xform, plane_sca );
    m4x4_premultiply_by_m4x4 ( xform, ctx->view_proj_mat );
    qb_render_plane ( ctx, wtx->skybox_gl_id[0], xform );

    VectorSet ( plane_pos, 0, offset[0], offset[1] );
    VectorSet ( plane_rot, 90, 0, 90 );
    m4x4_identity(xform);
    m4x4_rotate_by_vec3 ( xform, plane_rot, eZYX );
    m4x4_translate_by_vec3 ( xform, plane_pos );
    m4x4_scale_by_vec3 ( xform, plane_sca );
    m4x4_premultiply_by_m4x4 ( xform, ctx->view_proj_mat );
    qb_render_plane ( ctx, wtx->skybox_gl_id[0], xform );
    
    VectorSet ( plane_pos, 0, -offset[0], offset[1] );
    VectorSet ( plane_rot, 90, 0, -270 );
    m4x4_identity(xform);
    m4x4_rotate_by_vec3 ( xform, plane_rot, eZYX );
    m4x4_translate_by_vec3 ( xform, plane_pos );
    m4x4_scale_by_vec3 ( xform, plane_sca );
    m4x4_premultiply_by_m4x4 ( xform, ctx->view_proj_mat );
    glCullFace ( GL_FRONT );
    qb_render_plane ( ctx, wtx->skybox_gl_id[0], xform );
    glCullFace ( GL_BACK );
    
    VectorSet ( plane_pos, 0, offset[0] + offset[1], 0 );
    VectorSet ( plane_rot, 90, 90, 90 );
    m4x4_identity(xform);
    m4x4_rotate_by_vec3 ( xform, plane_rot, eZYX );
    m4x4_translate_by_vec3 ( xform, plane_pos );
    m4x4_scale_by_vec3 ( xform, plane_sca );
    m4x4_premultiply_by_m4x4 ( xform, ctx->view_proj_mat );
    qb_render_plane ( ctx, wtx->skybox_gl_id[1], xform );
    
    VectorSet ( plane_pos, 0, 1, 0 );
    VectorSet ( plane_rot, 0, 0, 0 );
    //VectorSet ( plane_sca, 16, 16, 16 );
    m4x4_identity(xform);
    m4x4_rotate_by_vec3 ( xform, plane_rot, eZYX );
    m4x4_translate_by_vec3 ( xform, plane_pos );
    m4x4_scale_by_vec3 ( xform, plane_sca );
    m4x4_premultiply_by_m4x4 ( xform, ctx->view_proj_mat );
    qb_render_plane ( ctx, wtx->ground_gl_id, xform );
    
//    static int uno = 0;
//    if ( !uno )
//    {
//    aabb_t aabb = { 0, 0, 0, 2, 2, 2 };
//    qb_aabb_render_solid ( ctx, &aabb, wtx->skybox_gl_id[0], GL_BACK );
//        uno = 1;
//    }

    qb_qube_render_count = 0;
    qb_qube_render ( ctx, ctx->octree_root, 1 );
    //qb_qube_render_solid ( ctx, ctx->octree_root, 1 );
    //qb_qube_render_wired ( ctx, ctx->octree_root, 1 );
    //printf ( "render_count: %i\n", qb_qube_render_count );
    qb_cuboid_render ( ctx );
    //qb_octant_render ( ctx, ctx->octree_root, 1 );
    
    qb_spline_render ( ctx, wtx->spline );
}

void qb_world_zoom_end ( context_t* ctx )
{
    assert ( ctx && ctx->ctx_type == QB_CTX_TYPE_WORLD );
    
    if ( ctx->ortho_aabb.extents[0] < 5 )
    {
        ctx->ortho_aabb.extents[0] = 5;
    }
    else if ( ctx->ortho_aabb.extents[0] > 12 )
    {
        ctx->ortho_aabb.extents[0] = 12;
    }
}
