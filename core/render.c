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
        { 255, 0, 0, 0,      1,  1, -1 },
        { 255, 255, 0, 0,    1, -1, -1 },
        { 0, 255, 0, 0,     -1, -1, -1 },
        { 0, 0, 0, 0,       -1,  1, -1 },
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
}









