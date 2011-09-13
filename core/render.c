// author: rozgo

#include "qb.h"

GLint qb_gl_uniforms[NUM_UNIFORMS];
GLint qb_line_prog;
GLint qb_point_prog;
GLint qb_solid_prog;
GLuint qb_point_tex_gl_id;
GLuint qb_solid_tex_gl_id;
GLuint qb_solid_gl_id;
GLuint qb_solid_idx_gl_id;
GLuint qb_plane_gl_id;
GLuint qb_plane_idx_gl_id;

// cube ///////////////////////////////////////////////////////////////////////
//    v6----- v5
//   /|      /|
//  v1------v0|
//  | |     | |
//  | |v7---|-|v4
//  |/      |/
//  v2------v3


void qb_render_init ( void )
{
    const solid_vertex_t plane_vertexes[] = {
        { 255, 0, 0, 0,      1,  0,  1 },
        { 255, 255, 0, 0,    1,  0, -1 },
        { 0, 255, 0, 0,     -1,  0, -1 },
        { 0, 0, 0, 0,       -1,  0,  1 },
    };
    
    const GLubyte plane_indexes[] = {
        // Front
        0,  1,  2,
        2,  3,  0,
    };
    
    const vec4_t cube_lines[] = {
        
        { -1,  1,  1, 0 }, {  1,  1,  1, 0 }, // v1 - v0
        { -1, -1,  1, 0 }, {  1, -1,  1, 0 }, // v2 - v3
        { -1, -1, -1, 0 }, {  1, -1, -1, 0 }, // v7 - v4
        { -1,  1, -1, 0 }, {  1,  1, -1, 0 }, // v6 - v5
        
        { -1,  1,  1, 0 }, { -1, -1,  1, 0 }, // v1 - v2
        {  1,  1,  1, 0 }, {  1, -1,  1, 0 }, // v0 - v3
        {  1,  1, -1, 0 }, {  1, -1, -1, 0 }, // v5 - v4
        { -1,  1, -1, 0 }, { -1, -1, -1, 0 }, // v6 - v7
        
        { -1,  1,  1, 0 }, { -1,  1, -1, 0 }, // v1 - v6
        { -1, -1,  1, 0 }, { -1, -1, -1, 0 }, // v2 - v7
        {  1, -1,  1, 0 }, {  1, -1, -1, 0 }, // v3 - v4
        {  1,  1,  1, 0 }, {  1,  1, -1, 0 }, // v0 - v5
    };
    
    const solid_vertex_t solid_vertexes[] = {
        // Front
        { 255, 0, 0, 0,      1, -1,  1 },
        { 255, 255, 0, 0,    1,  1,  1 },
        { 0, 255, 0, 0,     -1,  1,  1 },
        { 0, 0, 0, 0,       -1, -1,  1 },
        // Back
        { 255, 0, 0, 0,      -1, -1, -1 },
        { 255, 255, 0, 0,    -1, 1, -1 },
        { 0, 255, 0, 0,     1, 1, -1 },
        { 0, 0, 0, 0,       1,  -1, -1 },
        // Left
        { 255, 0, 0, 0,     -1, -1,  1 },
        { 255, 255, 0, 0,   -1,  1,  1 },
        { 0, 255, 0, 0,     -1,  1, -1 },
        { 0, 0, 0, 0,       -1, -1, -1 },
        // Right
        { 255, 0, 0, 0,      1, -1, -1 },
        { 255, 255, 0, 0,    1,  1, -1 },
        { 0, 255, 0, 0,      1,  1,  1 },
        { 0, 0, 0, 0,        1, -1,  1 },
        // Top
        { 255, 0, 0, 0,      1,  1,  1 },
        { 255, 255, 0, 0,    1,  1, -1 },
        { 0, 255, 0, 0,     -1,  1, -1 },
        { 0, 0, 0, 0,       -1,  1,  1 },
        // Bottom
        { 255, 0, 0, 0,      1, -1, -1 },
        { 255, 255, 0, 0,    1, -1,  1 },
        { 0, 255, 0, 0,     -1, -1,  1 },
        { 0, 0, 0, 0,       -1, -1, -1 },
    };
    
    const GLubyte solid_indexes[] = {
        // Front
        0,  1,  2,
        2,  3,  0,
        // Back
        4,  5,  6,
        6,  7,  4,
        // Left
        8,  9, 10,
        10, 11,  8,
        // Right
        12, 13, 14,
        14, 15, 12,
        // Top
        16, 17, 18,
        18, 19, 16,
        // Bottom
        20, 21, 22,
        22, 23, 20,
    };
    
    glGenBuffers ( 1, &qb_cuboid_gl_id );
    glBindBuffer ( GL_ARRAY_BUFFER, qb_cuboid_gl_id );
    glBufferData ( GL_ARRAY_BUFFER, sizeof ( cube_lines ), cube_lines, GL_STATIC_DRAW );
    
    glGenBuffers ( 1, &qb_solid_gl_id );
    glBindBuffer ( GL_ARRAY_BUFFER, qb_solid_gl_id );
    glBufferData ( GL_ARRAY_BUFFER, sizeof ( solid_vertexes ), solid_vertexes, GL_STATIC_DRAW );
    
    glGenBuffers ( 1, &qb_solid_idx_gl_id );
    glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, qb_solid_idx_gl_id );
    glBufferData ( GL_ELEMENT_ARRAY_BUFFER, sizeof ( solid_indexes ), solid_indexes, GL_STATIC_DRAW );
    
    glGenBuffers ( 1, &qb_plane_gl_id );
    glBindBuffer ( GL_ARRAY_BUFFER, qb_plane_gl_id );
    glBufferData ( GL_ARRAY_BUFFER, sizeof ( plane_vertexes ), plane_vertexes, GL_STATIC_DRAW );
    
    glGenBuffers ( 1, &qb_plane_idx_gl_id );
    glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, qb_plane_idx_gl_id );
    glBufferData ( GL_ELEMENT_ARRAY_BUFFER, sizeof ( plane_indexes ), plane_indexes, GL_STATIC_DRAW );
}

void qb_aabb_render_solid ( context_t* ctx, aabb_t* aabb, GLuint tex_gl_id, GLenum cull_mode )
{
    glUseProgram ( qb_solid_prog );
    glCullFace ( cull_mode );
    
    glEnable ( GL_TEXTURE_2D );
    glUniform1i ( qb_gl_uniforms[UNIFORM_SOLID_TEXTURE], 0 );
    glActiveTexture ( GL_TEXTURE0 );
    glBindTexture ( GL_TEXTURE_2D, tex_gl_id );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    
    glBindBuffer ( GL_ARRAY_BUFFER, qb_solid_gl_id );
    glVertexAttribPointer ( ATTRIB_UV, 2, GL_UNSIGNED_BYTE, 1, sizeof ( solid_vertex_t ), 0 );
    glEnableVertexAttribArray ( ATTRIB_UV );
    glVertexAttribPointer ( ATTRIB_POSITION, 3, GL_BYTE, 0, sizeof ( solid_vertex_t ), ( ( GLubyte * )0 + ( 4 ) ) );
    glEnableVertexAttribArray ( ATTRIB_POSITION );
    glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, qb_solid_idx_gl_id );
    
    vec4_t color = { 1, 1, 1, 1 };
    glUniform4fv ( qb_gl_uniforms[UNIFORM_SOLID_COLOR], 1, color );
    
    m4x4_t xform;
    m4x4_identity ( xform );
    m4x4_translate_by_vec3 ( xform, aabb->origin );
    m4x4_scale_by_vec3 ( xform, aabb->extents );
    m4x4_premultiply_by_m4x4 ( xform, ctx->view_proj_mat );
    glUniformMatrix4fv ( qb_gl_uniforms[UNIFORM_MVP_MAT], 1, 0, xform );
    
    glDrawElements ( GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, 0 );
    
    glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glDisableVertexAttribArray ( ATTRIB_POSITION );
    glDisableVertexAttribArray ( ATTRIB_UV );
    glBindBuffer ( GL_ARRAY_BUFFER, 0 );
    
    glBindTexture ( GL_TEXTURE_2D, 0 );
    glDisable ( GL_TEXTURE_2D );
    
    glCullFace ( GL_BACK );
    glUseProgram ( 0 );
}

void qb_render_plane ( context_t* ctx, GLuint tex_gl_id, m4x4_t xform )
{
    glUseProgram ( qb_solid_prog );
    
    glEnable ( GL_TEXTURE_2D );
    glUniform1i ( qb_gl_uniforms[UNIFORM_SOLID_TEXTURE], 0 );
    glActiveTexture ( GL_TEXTURE0 );
    glBindTexture ( GL_TEXTURE_2D, tex_gl_id );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    
    glBindBuffer ( GL_ARRAY_BUFFER, qb_plane_gl_id );
    glVertexAttribPointer ( ATTRIB_UV, 2, GL_UNSIGNED_BYTE, 1, sizeof ( solid_vertex_t ), 0 );
    glEnableVertexAttribArray ( ATTRIB_UV );
    glVertexAttribPointer ( ATTRIB_POSITION, 3, GL_BYTE, 0, sizeof ( solid_vertex_t ), ( ( GLubyte * )0 + ( 4 ) ) );
    glEnableVertexAttribArray ( ATTRIB_POSITION );
    glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, qb_plane_idx_gl_id );
    
    vec4_t color = { 1, 1, 1, 1 };
    glUniform4fv ( qb_gl_uniforms[UNIFORM_SOLID_COLOR], 1, color );
    
    glUniformMatrix4fv ( qb_gl_uniforms[UNIFORM_MVP_MAT], 1, 0, xform );
    
    glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0 );
    
    glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glDisableVertexAttribArray ( ATTRIB_POSITION );
    glDisableVertexAttribArray ( ATTRIB_UV );
    glBindBuffer ( GL_ARRAY_BUFFER, 0 );
    
    glBindTexture ( GL_TEXTURE_2D, 0 );
    glDisable ( GL_TEXTURE_2D );
    
    glUseProgram ( 0 );
}








