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

int qb_pick_select ( context_t* ctx, const vec3_t screen_pos, octant_t** octant, vec3_t plane )
{
    assert ( ctx );
    assert ( octant );
    
    ray_t ray;
    qb_screen_pick_ray ( ctx, screen_pos, &ray );
    float dist = MAXFLOAT;
    vec3_t hit;
    octant_t* picked = 0;
    int axis;
    
    qb_octant_intersect_ray ( ctx->octree_root, &ray, &dist, &axis, &picked );
    
    if ( picked && picked->qube )
    {
        hit[0] = ray.origin[0] + ray.direction[0] * dist;
        hit[1] = ray.origin[1] + ray.direction[1] * dist;
        hit[2] = ray.origin[2] + ray.direction[2] * dist;
        
        float side = copysignf ( 1.0f, hit[axis] - picked->aabb.origin[axis] );
        plane[0] = plane[1] = plane[2] = 0;
        plane[axis] = side;
        
        *octant = picked;
        return 1;
    }
    return 0;
}

int qb_pick_extrude ( context_t* ctx, const vec3_t screen_pos, octant_t** _octant, vec3_t plane )
{
    assert ( ctx );
    assert ( _octant );
    
    ray_t ray;
    qb_screen_pick_ray ( ctx, screen_pos, &ray );
    
    float dist = MAXFLOAT;
    vec3_t hit;
    octant_t* picked = 0;
    int axis;
    qb_octant_intersect_ray ( ctx->octree_root, &ray, &dist, &axis, &picked );
    
    if ( picked && picked->qube )
    {
        static color_t dark_green = { 0, 128, 0, 255 };
        qb_cuboid_draw ( ctx, &picked->aabb, dark_green, 3, 0.3f );
        
        hit[0] = ray.origin[0] + ray.direction[0] * dist;
        hit[1] = ray.origin[1] + ray.direction[1] * dist;
        hit[2] = ray.origin[2] + ray.direction[2] * dist;
        
        float side = copysignf ( 1.0f, hit[axis] - picked->aabb.origin[axis] );
        vec3_t pos;
        pos[0] = picked->aabb.origin[0];
        pos[1] = picked->aabb.origin[1];
        pos[2] = picked->aabb.origin[2];
        pos[axis] += side;
        
        plane[0] = plane[1] = plane[2] = 0;
        plane[axis] = side;
        
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

void qb_context_select ( context_t* ctx, octant_t* octant )
{
    if ( ctx->selection.head )
    {
        octant_t* next = ctx->selection.head;
        while ( next )
        {
            if ( next == octant ) return;
            next = next->next_selected;
        }
        octant->next_selected = ctx->selection.head;
    }
    
    ctx->selection.head = octant;
    
    vec3_t min_point = { FLT_MAX, FLT_MAX, FLT_MAX };
    vec3_t max_point = { FLT_MIN, FLT_MIN, FLT_MIN };
    octant_t* chain = ctx->selection.head;
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
    if ( ctx->selection.glow != 0 )
    {
        ctx->selection.glow->timer = 0.1f;
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
        ctx->selection.glow = qb_cuboid_draw ( ctx, &aabb, bright_green, 3, -1 );
    }
}

void qb_context_deselect ( context_t* ctx, octant_t* octant )
{
    octant_t* prev = octant;
    while ( prev ) 
    {
        octant_t* next = prev->next_selected;
        prev->next_selected = 0;
        prev = next;
    }
    ctx->selection.head = 0;
    
    if ( ctx->selection.glow != 0 )
    {
        ctx->selection.glow->timer = 0.1f;
    }
    ctx->selection.glow = 0;
}

