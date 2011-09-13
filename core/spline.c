#include "spline.h"

spline_t* qb_spline_from_random_elipse ( vec3_t extents, uint8_t num_points, vec_t min_dist )
{
    assert ( num_points % 3 == 0 );
    spline_t* spline = malloc ( sizeof ( spline_t ) + num_points * sizeof ( vec3_t ) );
    
    VectorSet ( spline->position, 0, 0, 0 );
    VectorSet ( spline->scale, 1, 1, 1 );
    VectorSet ( spline->rotation, 0, 0, 0 );
    
    spline->num_points = num_points;
    spline->points = ( vec3_t* )( &spline->points + 1 );
    
    for ( uint8_t i = 0; i < num_points; ++i )
    {
        float t = i / ( float )num_points * Q_PI * 2;
        
        spline->points[i][0] = sinf ( t ) * cosf ( t ) * extents[0];
        spline->points[i][1] = sinf ( t * t ) * extents[1];
        spline->points[i][2] = cosf ( t ) * extents[2];
    }
    
    return spline;
}

static void cubic_bezier_interpolate ( vec3_t out, vec3_t* points, float t )
{
    float it = 1 - t;
    for ( int i = 0; i < 3; ++i )
    {
        out[i] = ( it * it * it ) * points[0][i];
        out[i] += 3 * ( it * it ) * t * points[1][i];
        out[i] += 3 * it * ( t * t ) * points[2][i];
        out[i] += ( t * t * t ) * points[3][i];
    }
}

void qb_spline_interpolate ( spline_t* spline, vec3_t out, float t )
{    
    float w = spline->num_points / 3 * t;
    uint8_t i = w;
    
    assert ( t >= 0 && t <= 1.0f );
//    
//    if ( t > 1 )
//    {
//        VectorCopy ( spline->points[0], out );
//        return;
//    }
    
    float t0 = w - i;
    
    //printf ( "i: %i, t: %f\n", i, t0 );
    
    vec3_t points[4];
    
    VectorCopy ( spline->points[i * 3 + 0], points[0] );
    VectorCopy ( spline->points[i * 3 + 1], points[1] );
    VectorCopy ( spline->points[i * 3 + 2], points[2] );
    VectorCopy ( spline->points[i * 3 + 3], points[3] );
    if ( i * 3 + 3 == spline->num_points )
    {
        VectorCopy ( spline->points[0], points[3] );
    }
    
    cubic_bezier_interpolate ( out, points, t0 );
}

void qb_spline_render ( context_t* ctx, spline_t* spline )
{    
    glUseProgram ( qb_line_prog );
    
    glBindBuffer ( GL_ARRAY_BUFFER, qb_cuboid_gl_id );                
    glVertexAttribPointer ( ATTRIB_POSITION, 3, GL_FLOAT, 0, sizeof ( vec4_t ), 0 );
    glEnableVertexAttribArray ( ATTRIB_POSITION );
    
    vec4_t* color;
    vec4_t green = { 0, 1, 0, 1 };
    vec4_t red = { 1, 0, 0, 1 };
    vec4_t gray = { 0.7f, 0.7f, 0.7f, 1 };
    
    for ( uint32_t i = 0; i < spline->num_points; ++i )
    {
        if ( i % 3 == 0 )
        {
            color = &green;
        }
        else
        {
            color = &red;
        }
        
        glLineWidth ( 10 );
        glUniform4fv ( qb_gl_uniforms[UNIFORM_LINE_COLOR], 1, *color );
        
        aabb_t aabb = { spline->points[i][0], spline->points[i][1], spline->points[i][2], 0.01f, 0.01f, 0.01f };
        
        m4x4_t xform;
        m4x4_identity ( xform );
        m4x4_translate_by_vec3 ( xform, spline->position );
        m4x4_rotate_by_vec3 ( xform, spline->rotation, eZYX );
        m4x4_scale_by_vec3 ( xform, spline->scale );
        m4x4_translate_by_vec3 ( xform, aabb.origin );
        m4x4_scale_by_vec3 ( xform, aabb.extents );
        m4x4_premultiply_by_m4x4 ( xform, ctx->view_proj_mat );
        glUniformMatrix4fv ( qb_gl_uniforms[UNIFORM_MVP_MAT], 1, 0, xform );
        glDrawArrays ( GL_LINES, 0, 24 );
    }
    
    glLineWidth ( 5 );
    glUniform4fv ( qb_gl_uniforms[UNIFORM_LINE_COLOR], 1, gray );
    
    m4x4_t xform;
    m4x4_identity ( xform );
    m4x4_translate_by_vec3 ( xform, spline->position );
    m4x4_rotate_by_vec3 ( xform, spline->rotation, eZYX );
    m4x4_scale_by_vec3 ( xform, spline->scale );
    m4x4_premultiply_by_m4x4 ( xform, ctx->view_proj_mat );
    glUniformMatrix4fv ( qb_gl_uniforms[UNIFORM_MVP_MAT], 1, 0, xform );
    
    vec3_t points[4];
    int samples = 5;
    vec3_t tpoints[spline->num_points / 3 * samples][2];
    
    int count = 0;
    
    for ( uint32_t i = 0; i < spline->num_points; i+=3 )
    {
        VectorCopy ( spline->points[i + 0], points[0] );
        VectorCopy ( spline->points[i + 1], points[1] );
        VectorCopy ( spline->points[i + 2], points[2] );
        VectorCopy ( spline->points[i + 3], points[3] );
        if ( i + 3 == spline->num_points )
        {
            VectorCopy ( spline->points[0], points[3] );
        }
        
        for ( int s = 0; s < samples; ++s )
        {
            float t0 = s / ( float )samples;
            float t1 = ( s + 1 ) / ( float )samples;
            cubic_bezier_interpolate ( tpoints[count][0], points, t0 );
            cubic_bezier_interpolate ( tpoints[count][1], points, t1 );
            ++count;
        }
    }
    
    glBindBuffer ( GL_ARRAY_BUFFER, 0 );                
    glVertexAttribPointer ( ATTRIB_POSITION, 3, GL_FLOAT, 0, sizeof ( vec3_t ), tpoints );
    glEnableVertexAttribArray ( ATTRIB_POSITION );
    glDrawArrays ( GL_LINE_STRIP, 0, sizeof ( tpoints ) / sizeof ( vec3_t ) );
    
    glDisableVertexAttribArray ( ATTRIB_POSITION );
    glBindBuffer ( GL_ARRAY_BUFFER, 0 );
    glUseProgram ( 0 );
}

