#include "qb.h"

octant_t* octants;
octant_t* octant_free;
uint32_t octants_max;
uint32_t octants_count;

void qb_octants_init ( uint32_t max_octants )
{
    static int once = 0;
    assert ( once++ == 0 );
    
    octants_max = max_octants;
    octants_count = 0;
    int octants_mem_size = octants_max * sizeof ( octant_t );
    octants = malloc ( octants_mem_size );
    assert ( octants );
    memset ( octants, 0, octants_mem_size );
    
    for ( uint32_t i = 0; i < octants_max; ++i )
    {
        octants[i].next_free = octants + i + 1;
    }
    
    octants[octants_max - 1].next_free = 0;
    octant_free = octants;
}

octant_t* qb_octants_get_free ( void )
{
    assert ( octant_free );
    
    if ( octant_free )
    {
        ++octants_count;
        octant_t* free_node = octant_free;
        octant_free = free_node->next_free;
        memset ( free_node, 0, sizeof ( octant_t ) );
        VectorSet ( free_node->scale, 1, 1, 1 );
        return free_node;
    }
    return 0;
}

static void qb_octant_collapse_children ( octant_t* octant )
{
    assert ( octant );
    
    for ( int i = 0; i < 8; ++i )
    {
        octant_t* child = octant->octants[i];
        if ( child )
        {
            qb_octant_collapse_children ( child );
            octant->octants[i] = 0;
            
            child->next_free = octant_free;
            octant_free = child;
            --octants_count;
        }
    }
}

static void qb_octant_collapse_parents ( octant_t* octant )
{
    assert ( octant );
    
    int has_children = 1;
    octant_t* parent = octant->parent;
    
    if ( parent )
    {
        for ( int i = 0; i < 8; ++i )
        {
            if ( parent->octants[i] == octant )
            {
                parent->octants[i] = 0;
            }
            else if ( parent->octants[i] )
            {
                has_children = 0;
            }
        }
    }
    
    if ( has_children && parent && parent->parent )
    {
        qb_octant_collapse_parents ( parent );
        
        parent->next_free = octant_free;
        octant_free = parent;
        --octants_count;
    }
}

void qb_octant_collapse ( octant_t* octant )
{
    assert ( octant );
    
    qb_octant_collapse_children ( octant );
    qb_octant_collapse_parents ( octant );
    
    if ( octant->parent )
    {
        octant->next_free = octant_free;
        octant_free = octant;
        --octants_count;
    }
}

void qb_octant_expand ( octant_t* parent, const vec3_t pos, octant_t** octant )
{
    assert ( parent );
    assert ( octant );
    
    int q = 0;
    int qs[] = { -1, 1 };
    for ( uint32_t x = 0; x < 2; ++x )
    {
        for ( uint32_t y = 0; y < 2; ++y )
        {
            for ( uint32_t z = 0; z < 2; ++z )
            {
                aabb_t aabb;
                aabb.extents[0] = aabb.extents[1] = aabb.extents[2] = parent->aabb.extents[0] / 2;
                aabb.origin[0] = parent->aabb.origin[0] + qs[x] * parent->aabb.extents[0] / 2;
                aabb.origin[1] = parent->aabb.origin[1] + qs[y] * parent->aabb.extents[1] / 2;
                aabb.origin[2] = parent->aabb.origin[2] + qs[z] * parent->aabb.extents[2] / 2;
                
                if ( aabb_intersect_point ( &aabb, pos ) )
                {
                    octant_t* child = parent->octants[q];
                    if ( !child )
                    {
                        aabb_update_radius ( &aabb );
                        child = qb_octants_get_free ();
                        assert ( child != 0 );
                        parent->octants[q] = child;
                        child->parent = parent;
                        memcpy ( &child->aabb, &aabb, sizeof ( aabb_t ) );
                    }
                    
                    *octant = child;
                    
                    if ( parent->aabb.extents[0] > 1  )
                    {
                        qb_octant_expand ( child, pos, octant );
                    }
                }
                
                ++q;
            }
        }
    }
}

void qb_octant_intersect_point ( octant_t* parent, vec3_t point, octant_t** octant )
{
    assert ( parent );
    assert ( octant );
    
    if ( aabb_intersect_point ( &parent->aabb, point ) )
    {
        if ( parent->aabb.extents[0] < 1 )
        {
            *octant = parent;
            return;
        }
        
        for ( int i = 0; i < 8; ++i )
        {
            if ( parent->octants[i] )
            {
                qb_octant_intersect_point ( parent->octants[i], point, octant );
            }
        }
    }
}

void qb_octant_intersect_ray ( octant_t* parent, const ray_t* pick_ray, float* dist, int* plane, octant_t** qube )
{
    assert ( parent );
    assert ( pick_ray );
    assert ( dist );
    assert ( plane );
    assert ( qube );
    
    float local_dist;
    int local_plane;
    
    if ( aabb_intersect_ray ( &parent->aabb, pick_ray, &local_dist, &local_plane ) )
    {
        if ( parent->qube && local_dist < *dist )
        {
            *dist = local_dist;
            *plane = local_plane;
            *qube = parent;
        }
        
        for ( int i = 0; i < 8; ++i )
        {
            if ( parent->octants[i] )
            {
                qb_octant_intersect_ray ( parent->octants[i], pick_ray, dist, plane, qube );
            }
        }
    }
}

void qb_octant_render ( context_t* ctx, octant_t* octant, uint8_t top )
{
    assert ( ctx );
    assert ( octant );
    
    if ( top )
    {
        glUseProgram ( qb_line_prog );
        
        glBindBuffer ( GL_ARRAY_BUFFER, qb_cuboid_gl_id );
        glVertexAttribPointer( ATTRIB_POSITION, 3, GL_FLOAT, 0, sizeof ( vec4_t ), 0);
        glEnableVertexAttribArray( ATTRIB_POSITION );
        
        glLineWidth ( 1 );
        vec4_t gray = { 0.8f, 0.8f, 0.8f, 1 };
        glUniform4fv ( qb_gl_uniforms[UNIFORM_LINE_COLOR], 1, gray );
    }
    
    vec3_t origin;
    VectorCopy ( octant->aabb.origin, origin );
    m4x4_transform_point ( ctx->view_mat, origin );
    
    if ( !aabb_intersect_sphere ( &ctx->ortho_aabb, origin, octant->aabb.radius ) )
    {
        return;
    }
    
    m4x4_t xform;
    m4x4_translation_for_vec3 ( xform, octant->aabb.origin );
    m4x4_scale_by_vec3 ( xform, octant->aabb.extents );
    m4x4_premultiply_by_m4x4 ( xform, ctx->view_proj_mat );
    glUniformMatrix4fv ( qb_gl_uniforms[UNIFORM_MVP_MAT], 1, 0, xform );
    
    glDrawArrays ( GL_LINES, 0, 24 );
    
    for ( uint8_t i = 0; i < 8; ++i )
    {
        if ( octant->octants[i] )
        {
            qb_octant_render ( ctx, octant->octants[i], 0 );
        }
    }
    
    if ( top )
    {
        glDisableVertexAttribArray ( ATTRIB_POSITION );
        glBindBuffer ( GL_ARRAY_BUFFER, 0 );
        glUseProgram ( 0 );
    }
}

