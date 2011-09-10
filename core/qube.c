// author: rozgo

#include "qb.h"

static qube_t* qubes;
static qube_t* qube_node_free;
static uint32_t qubes_max = 0;
static uint32_t qubes_count = 0;

int qb_qube_render_count = 0;

void qb_qubes_init ( uint32_t max_qubes )
{
    qubes_max = max_qubes;
    qubes_count = 0;
    int qubes_mem_size = qubes_max * sizeof ( qube_t );
    qubes = malloc ( qubes_mem_size );
    assert ( qubes );
    memset ( qubes, 0, qubes_mem_size );
    for ( int i = 0; i < qubes_max; ++i )
    {
        qubes[i].next_free = qubes + i + 1;
    }
    qubes[qubes_max - 1].next_free = 0;
    qube_node_free = qubes;
}

qube_t* qb_qubes_get_free ( void )
{
    assert ( qube_node_free );
    
    if ( qube_node_free )
    {
        ++qubes_count;
        qube_t* free_node = qube_node_free;
        qube_node_free = free_node->next_free;
        memset ( free_node, 0, sizeof ( qube_t ) );
        return free_node;
    }
    return 0;
}

qube_t* qb_qube_from_texels ( qube_t* prev_frame, GLubyte* texels, uint8_t frames )
{
    qube_t* qube = qb_qubes_get_free ();
    
    if ( prev_frame )
    {
        qube->prev_frame = prev_frame;
        prev_frame->next_frame = qube;
    }
    
    for ( int y = 0; y < 16; ++y )
    {
        for ( int z = 0; z < 16; ++z )
        {
            for ( int x = 0; x < 16; ++x )
            {
                qube->qbits[y][z][x][0] = *( texels++ );
                qube->qbits[y][z][x][1] = *( texels++ );
                qube->qbits[y][z][x][2] = *( texels++ );
                qube->qbits[y][z][x][3] = *( texels++ );
            }
        }
    }
    
    if ( frames > 1 )
    {
        qb_qube_from_texels ( qube, texels, --frames );
    }
    
    return qube;
}

static void qb_qube_save_gpu ( qube_t* qube )
{
    assert ( qube );
    
    qube_vertex_t verts[4096];
    qube->gl_count = 0;
    
    for ( int y = 0; y < 16; ++y )
    {
        for ( int z = 0; z < 16; ++z )
        {
            for ( int x = 0; x < 16; ++x )
            {
                if ( qube->qbits[y][z][x][3] == 255 )
                {
                    verts[qube->gl_count].pos[0] = ( 16 - x );
                    verts[qube->gl_count].pos[1] = ( y + 1 );
                    verts[qube->gl_count].pos[2] = ( z + 1 );
                    verts[qube->gl_count].pos[3] = 0;
                    verts[qube->gl_count].col[0] = qube->qbits[y][z][x][0];
                    verts[qube->gl_count].col[1] = qube->qbits[y][z][x][1];
                    verts[qube->gl_count].col[2] = qube->qbits[y][z][x][2];
                    verts[qube->gl_count].col[3] = qube->qbits[y][z][x][3];
                    ++qube->gl_count;
                }
            }
        }
    }
    
    glGenBuffers ( 1, &qube->gl_id );
    assert ( qube->gl_id > 0 );
    if ( qube->gl_count > 0 )
    {
        glBindBuffer ( GL_ARRAY_BUFFER, qube->gl_id );
        glBufferData ( GL_ARRAY_BUFFER, sizeof ( qube_vertex_t ) * qube->gl_count, verts, GL_STATIC_DRAW );
    }
}

void qb_qube_render ( context_t* ctx, octant_t* octant, uint8_t top )
{
    assert ( ctx );
    
    if ( top )
    {
        glUseProgram ( qb_point_prog );
        
        glEnable ( GL_TEXTURE_2D );
        glUniform1i ( qb_gl_uniforms[UNIFORM_POINT_TEXTURE], 0 );
        glActiveTexture ( GL_TEXTURE0 );
        glBindTexture ( GL_TEXTURE_2D, qb_point_tex_gl_id );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        
        glUniform1f( qb_gl_uniforms[UNIFORM_POINT_SIZE], ctx->point_size );
    }
    
    vec3_t origin;
    VectorCopy ( octant->aabb.origin, origin );
    m4x4_transform_point ( ctx->view_mat, origin );
    
    if ( !aabb_intersect_sphere ( &ctx->ortho_aabb, origin, octant->aabb.radius ) )
    {
        return;
    }
    
    if ( octant->qube )
    {
        if ( octant->qube->gl_id > 0 )
        {
            if ( octant->qube->gl_count > 0 )
            {
                m4x4_t xform;
                m4x4_identity( xform );
                m4x4_translate_by_vec3 ( xform, octant->position );
                m4x4_translate_by_vec3 ( xform, octant->aabb.origin );
                m4x4_rotate_by_vec3 ( xform, octant->rotation, eZYX );
                m4x4_scale_by_vec3 ( xform, octant->scale );
                m4x4_premultiply_by_m4x4 ( xform, ctx->view_proj_mat );
                glUniformMatrix4fv ( qb_gl_uniforms[UNIFORM_MVP_MAT], 1, 0, xform );
                
                glBindBuffer ( GL_ARRAY_BUFFER, octant->qube->gl_id );
                glVertexAttribPointer ( ATTRIB_COLOR, 3, GL_UNSIGNED_BYTE, 1, sizeof ( qube_vertex_t ), 0 );
                glEnableVertexAttribArray ( ATTRIB_COLOR );
                glVertexAttribPointer ( ATTRIB_POSITION, 3, GL_UNSIGNED_BYTE, 0, sizeof ( qube_vertex_t ), ( ( GLubyte * )0 + ( 4 ) ) );
                glEnableVertexAttribArray ( ATTRIB_POSITION );
                glDrawArrays ( GL_POINTS, 0, octant->qube->gl_count );
                
                ++qb_qube_render_count;
            }
        }
        else
        {
            qb_qube_save_gpu ( octant->qube );
        }
    }
    
    for ( uint8_t i = 0; i < 8; ++i )
    {
        if ( octant->octants[i] )
        {
            qb_qube_render ( ctx, octant->octants[i], 0 );
        }
    }
    
    if ( top )
    {
        glDisableVertexAttribArray ( ATTRIB_POSITION );
        glDisableVertexAttribArray ( ATTRIB_COLOR );
        glBindTexture ( GL_TEXTURE_2D, 0 );
        glDisable ( GL_TEXTURE_2D );
        glBindBuffer ( GL_ARRAY_BUFFER, 0 );
        glUseProgram ( 0 );
    }
}

void qb_qube_render_solid ( context_t* ctx, octant_t* octant, uint8_t top )
{
    assert ( ctx );
    assert ( octant );
    
    if ( top )
    {
        glUseProgram ( qb_solid_prog );
        glCullFace ( GL_FRONT );
        
        glEnable ( GL_TEXTURE_2D );
        glUniform1i ( qb_gl_uniforms[UNIFORM_SOLID_TEXTURE], 0 );
        glActiveTexture ( GL_TEXTURE0 );
        glBindTexture ( GL_TEXTURE_2D, qb_solid_tex_gl_id );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        
        glBindBuffer ( GL_ARRAY_BUFFER, qb_solid_gl_id );
        glVertexAttribPointer ( ATTRIB_UV, 2, GL_UNSIGNED_BYTE, 1, sizeof ( solid_vertex_t ), 0 );
        glEnableVertexAttribArray ( ATTRIB_UV );
        glVertexAttribPointer ( ATTRIB_POSITION, 3, GL_BYTE, 0, sizeof ( solid_vertex_t ), ( ( GLubyte * )0 + ( 4 ) ) );
        glEnableVertexAttribArray ( ATTRIB_POSITION );
        glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, qb_solid_idx_gl_id );
    }
    
    vec3_t origin;
    VectorCopy ( octant->aabb.origin, origin );
    m4x4_transform_point ( ctx->view_mat, origin );
    
    if ( !aabb_intersect_sphere ( &ctx->ortho_aabb, origin, octant->aabb.radius ) )
    {
        return;
    }
    
    if ( octant->qube )
    {
        vec4_t color;
        glUniform4fv ( qb_gl_uniforms[UNIFORM_SOLID_COLOR], 1, color_normalize( octant->color, color ) );
        
        m4x4_t xform;
        m4x4_identity ( xform );
        m4x4_translate_by_vec3 ( xform, octant->position );
        m4x4_translate_by_vec3 ( xform, octant->aabb.origin );
        m4x4_rotate_by_vec3 ( xform, octant->rotation, eZYX );
        m4x4_scale_by_vec3 ( xform, octant->scale );
        m4x4_scale_by_vec3 ( xform, octant->aabb.extents );
        m4x4_premultiply_by_m4x4 ( xform, ctx->view_proj_mat );
        glUniformMatrix4fv ( qb_gl_uniforms[UNIFORM_MVP_MAT], 1, 0, xform );
        
        glDrawElements ( GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, 0 );
        
        ++qb_qube_render_count;
    }
    
    for ( uint8_t i = 0; i < 8; ++i )
    {
        if ( octant->octants[i] )
        {
            qb_qube_render_solid ( ctx, octant->octants[i], 0 );
        }
    }
    
    if ( top )
    {
        glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, 0 );
        glDisableVertexAttribArray ( ATTRIB_POSITION );
        glDisableVertexAttribArray ( ATTRIB_UV );
        glBindTexture ( GL_TEXTURE_2D, 0 );
        glDisable ( GL_TEXTURE_2D );
        glBindBuffer ( GL_ARRAY_BUFFER, 0 );
        glCullFace ( GL_BACK );
        glUseProgram ( 0 );
    }
}

void qb_qube_render_wired ( context_t* ctx, octant_t* octant, uint8_t top )
{
    assert ( ctx );
    assert ( octant );
    
    if ( top )
    {
        glUseProgram ( qb_line_prog );
        
        glBindBuffer ( GL_ARRAY_BUFFER, qb_cuboid_gl_id );
        glVertexAttribPointer ( ATTRIB_POSITION, 3, GL_FLOAT, 0, sizeof ( vec4_t ), 0);
        glEnableVertexAttribArray ( ATTRIB_POSITION );
        
        glLineWidth ( 1 );
    }
    
    vec3_t origin;
    VectorCopy ( octant->aabb.origin, origin );
    m4x4_transform_point ( ctx->view_mat, origin );
    
    if ( !aabb_intersect_sphere ( &ctx->ortho_aabb, origin, octant->aabb.radius ) )
    {
        return;
    }
    
    if ( octant->qube )
    {
        vec4_t color;
        glUniform4fv ( qb_gl_uniforms[UNIFORM_LINE_COLOR], 1, color_normalize( octant->color, color ) );
        
        m4x4_t xform;
        m4x4_identity ( xform );
        m4x4_translate_by_vec3 ( xform, octant->aabb.origin );
        m4x4_scale_by_vec3 ( xform, octant->scale );
        m4x4_scale_by_vec3 ( xform, octant->aabb.extents );
        m4x4_premultiply_by_m4x4 ( xform, ctx->view_proj_mat );
        glUniformMatrix4fv ( qb_gl_uniforms[UNIFORM_MVP_MAT], 1, 0, xform );
        
        glDrawArrays ( GL_LINES, 0, 24 );
        
        ++qb_qube_render_count;
    }
    
    for ( uint8_t i = 0; i < 8; ++i )
    {
        if ( octant->octants[i] )
        {
            qb_qube_render_wired ( ctx, octant->octants[i], 0 );
        }
    }
    
    if ( top )
    {
        glDisableVertexAttribArray ( ATTRIB_POSITION );
        glBindBuffer ( GL_ARRAY_BUFFER, 0 );
        glUseProgram ( 0 );
    }
}
