#include "qb.h"

cuboid_t* cuboids;
cuboid_t* cuboid_free;
uint32_t cuboids_max;
uint32_t cuboids_count;
float cuboid_time;
GLuint qb_cuboid_gl_id;
float cuboid_timer;

void qb_cuboids_init ( uint32_t max_cuboids )
{
    static int once = 0;
    assert ( once++ == 0 );
    
    cuboid_time = 0;
    cuboids_max = max_cuboids;
    cuboids_count = 0;
    
    int cuboid_mem_size = cuboids_max * sizeof ( cuboid_t );
    cuboids = malloc ( cuboid_mem_size );
    assert ( cuboids );
    memset ( cuboids, 0, cuboid_mem_size );
    for ( int i = 0; i < cuboids_max; ++i )
    {
        cuboids[i].next_free = cuboids + i + 1;
    }
    cuboids[cuboids_max - 1].next_free = 0;
    cuboid_free = cuboids;
}

void qb_cuboid_update ( void )
{
    float dt = qb_timer_elapsed () - cuboid_time;
    if ( dt > 0.03f ) dt = 0.03f;
    cuboid_time += dt;
}

cuboid_t* qb_cuboid_draw ( context_t* ctx, const aabb_t* aabb, color_t color, uint8_t line_width, float timer )
{
    assert ( ctx );
    assert ( cuboids );
    assert ( cuboid_free );
    
    if ( cuboid_free )
    {
        ++cuboids_count;
        cuboid_t* cuboid = cuboid_free;
        cuboid_free = cuboid->next_free;
        memcpy ( &cuboid->aabb, aabb, sizeof ( aabb_t ) );
        memcpy ( cuboid->color, color, sizeof ( color_t ) );
        cuboid->next_linked = 0;
        cuboid->line_width = line_width;
        cuboid->timer = timer;
        if ( cuboid->timer == 0 ) cuboid->timer = 0.01f;
        if ( cuboid->timer < 0 ) cuboid->timer = FLT_MAX;
        cuboid->ctx = ctx;
        return cuboid;
    }
    return 0;
}

void qb_cuboid_render ( context_t* ctx )
{
    float dt = qb_timer_elapsed () - cuboid_time;
    if ( dt > 0.03f ) dt = 0.03f;
    
    glUseProgram ( qb_line_prog );
    
    glBindBuffer ( GL_ARRAY_BUFFER, qb_cuboid_gl_id );                
    glVertexAttribPointer( ATTRIB_POSITION, 3, GL_FLOAT, 0, sizeof ( vec4_t ), 0);
    glEnableVertexAttribArray( ATTRIB_POSITION );
    
    vec4_t color;
    
    for ( uint32_t i = 0; i < cuboids_max; ++i )
    {
        cuboid_t* cuboid = cuboids + i;
        
        if ( cuboid->ctx == ctx && cuboid->timer > 0 )
        {
            glLineWidth ( cuboid->line_width );
            glUniform4fv ( qb_gl_uniforms[UNIFORM_LINE_COLOR], 1, color_normalize( cuboid->color, color ) );
            
            m4x4_t xform;
            m4x4_translation_for_vec3 ( xform, cuboid->aabb.origin );
            m4x4_scale_by_vec3 ( xform, cuboid->aabb.extents );
            m4x4_premultiply_by_m4x4 ( xform, ctx->view_proj_mat );
            glUniformMatrix4fv ( qb_gl_uniforms[UNIFORM_MVP_MAT], 1, 0, xform );
            glDrawArrays ( GL_LINES, 0, 24 );
            
            cuboid->timer -= dt;
            if ( cuboid->timer < 0.01f )
            {
                cuboid->timer = 0;
                cuboid->next_free = cuboid_free;
                cuboid_free = cuboid;
                --cuboids_count;
            }
        }
    }
    
    glDisableVertexAttribArray ( ATTRIB_POSITION );
    glBindBuffer ( GL_ARRAY_BUFFER, 0 );
    glUseProgram ( 0 );
}

