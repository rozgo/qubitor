// author: rozgo

#include "qb.h"

void qb_context_init ( context_t* ctx )
{
    assert ( ctx );
    
    ctx->ortho_aabb.extents[0] = 8;
    ctx->ortho_aabb.extents[1] = 8;
    ctx->ortho_aabb.extents[2] = 1;
    ctx->ortho_extents_trail = ctx->ortho_aabb.extents[0];
    ctx->point_size = 4;
    
    ctx->octree_depth = 8;
    ctx->octree_size = round( pow ( pow ( 8, ctx->octree_depth ), 1 / 3.0 ) );
    
    ctx->octree_root = qb_octants_get_free ();
    assert ( ctx->octree_root );
    
    VectorSet ( ctx->octree_root->aabb.origin, 0, 0, 0 );
    VectorSet ( ctx->octree_root->aabb.extents, ctx->octree_size / 2, ctx->octree_size / 2, ctx->octree_size / 2 );
    aabb_update_radius ( &ctx->octree_root->aabb );
}

void qb_screen_to_world ( context_t* ctx, vec3_t pos, const vec3_t screen_pos )
{
    assert ( ctx );
    
    vec3_t w;
    w[0] = screen_pos[0];
    w[1] = (float)ctx->viewport[3] - screen_pos[1];
    w[2] = 0;
    m4x4_un_project ( w, ctx->view_mat, ctx->proj_mat, ctx->viewport, pos );
}

void qb_screen_pick_ray ( context_t* ctx, const vec3_t screen_pos, ray_t* ray )
{
    assert ( ctx );
    assert ( ray );
    
    vec3_t p;
    qb_screen_to_world ( ctx, p, screen_pos );
    ray->origin[0] = p[0];
    ray->origin[1] = p[1];
    ray->origin[2] = p[2];
    ray->direction[0] = -ctx->view_mat[2];
    ray->direction[1] = -ctx->view_mat[6];
    ray->direction[2] = -ctx->view_mat[10];
}

int qb_pick_select ( context_t* ctx, const vec3_t screen_pos, octant_t** octant )
{
    assert ( ctx );
    assert ( octant );
    
    ray_t ray;
    qb_screen_pick_ray ( ctx, screen_pos, &ray );
    float dist = MAXFLOAT;
    octant_t* picked = 0;
    int plane;
    qb_octant_intersect_ray ( ctx->octree_root, &ray, &dist, &plane, &picked );
    
    if ( picked && picked->qube )
    {
        *octant = picked;
        return 1;
    }
    return 0;
}

int qb_pick_extrude ( context_t* ctx, const vec3_t screen_pos, octant_t** _octant )
{
    assert ( ctx );
    assert ( _octant );
    
    ray_t ray;
    qb_screen_pick_ray ( ctx, screen_pos, &ray );
    
    float dist = MAXFLOAT;
    vec3_t hit;
    octant_t* picked = 0;
    int plane;
    qb_octant_intersect_ray ( ctx->octree_root, &ray, &dist, &plane, &picked );
    
    if ( picked && picked->qube )
    {
        static color_t dark_green = { 0, 128, 0, 255 };
        qb_cuboid_draw ( ctx, &picked->aabb, dark_green, 3, 0.3f );
        
        hit[0] = ray.origin[0] + ray.direction[0] * dist;
        hit[1] = ray.origin[1] + ray.direction[1] * dist;
        hit[2] = ray.origin[2] + ray.direction[2] * dist;
        
        float side = copysignf ( 1.0f, hit[plane] - picked->aabb.origin[plane] );
        vec3_t pos;
        pos[0] = picked->aabb.origin[0];
        pos[1] = picked->aabb.origin[1];
        pos[2] = picked->aabb.origin[2];
        pos[plane] += side * 1;
        
        octant_t* octant = 0;
        qb_octant_expand ( ctx->octree_root, pos, &octant );
        if ( &octant )
        {
            *_octant = octant;
            octant->qube = picked->qube;
            octant->flags = picked->flags;
            VectorCopy ( picked->rotation, octant->rotation );
            VectorCopy ( picked->position, octant->position );
            VectorCopy ( picked->scale, octant->scale );
            octant->color[0] = picked->color[0];
            octant->color[1] = picked->color[1];
            octant->color[2] = picked->color[2];
            octant->color[3] = picked->color[3];
            
            static color_t bright_green = { 0, 255, 0, 255 };
            qb_cuboid_draw ( ctx, &octant->aabb, bright_green, 3, 0.5f );
            return 1;
        }
    }
    return 0;
}

void qb_camera_rotate ( context_t* ctx, const vec3_t rotation )
{
    ctx->camera_rot[0] += rotation[0];
    if ( ctx->camera_rot[0] < -60 )
        ctx->camera_rot[0] = -60;
    if ( ctx->camera_rot[0] > 60 )
        ctx->camera_rot[0] = 60;
    ctx->camera_rot[1] += rotation[1];
}


