// author: rozgo

#ifndef _QB_H
#define _QB_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <float.h>
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include "mathlib.h"

extern GLint qb_gl_uniforms[];
extern GLint qb_line_prog;
extern GLint qb_point_prog;
extern GLint qb_solid_prog;
extern GLuint qb_point_tex_gl_id;
extern GLuint qb_cuboid_gl_id;
extern GLuint qb_solid_gl_id;
extern GLuint qb_solid_idx_gl_id;
extern GLuint qb_solid_tex_gl_id;

vec_t qb_timer_elapsed ( void );

typedef unsigned char color_t[4];

vec_t* color_normalize ( const color_t in_color, vec4_t out_color );

enum
{
    UNIFORM_MVP_MAT,
    UNIFORM_POINT_SIZE,
    UNIFORM_POINT_TEXTURE,
    UNIFORM_LINE_COLOR,
    UNIFORM_SOLID_COLOR,
    UNIFORM_SOLID_TEXTURE,
    NUM_UNIFORMS
};

enum
{
    ATTRIB_POSITION,
    ATTRIB_COLOR,
    ATTRIB_UV,
    NUM_ATTRIBUTES
};

typedef struct qube_t
{
    GLubyte qbits[16][16][16][4];
    struct qube_t* next_free;
    struct qube_t* prev_frame;
    struct qube_t* next_frame;
    GLuint gl_id;
    GLuint gl_count;
    
} qube_t;

typedef struct qube_vertex_t
{
    GLubyte col[4];
    GLfloat pos[3];
    
} qube_vertex_t;

typedef struct
{
    GLubyte uv[4];
    GLfloat pos[3];
    
} solid_vertex_t;

#define OCTANT_FLAG_NONE 0x0

typedef struct octant_t
{
    aabb_t aabb;
    uint32_t flags;
    struct octant_t* next_free;
    struct octant_t* next_selected;
    struct octant_t* parent;
    struct octant_t* octants[8];
    vec3_t position;
    vec3_t rotation;
    vec3_t scale;
    color_t color;
    qube_t* qube;
    
} octant_t;

typedef struct cuboid_t
{
    aabb_t aabb;
    float timer;
    color_t color;
    uint8_t line_width;
    uint8_t solid;
    struct context_t* ctx;
    struct cuboid_t* next_free;
    struct cuboid_t* next_linked;
    
} cuboid_t;

typedef struct context_t
{
    m4x4_t model_mat;
    m4x4_t view_mat;
    m4x4_t proj_mat;
    m4x4_t view_proj_mat;
    
    int viewport[4];
    
    octant_t* octants;
    octant_t* octree_root;
    octant_t* octant_free;
    uint32_t octants_max;
    uint32_t octants_count;
    float octree_size;
    uint32_t octree_depth;
    
    vec3_t camera_rot;
    vec3_t camera_rot_trail;
    vec3_t camera_target;
    vec3_t camera_target_trail;
    
    aabb_t ortho_aabb;
    float ortho_extents_trail;
    
    float point_size;
    
    uint8_t ctx_type;
    
} context_t;

void qb_qubes_init ( uint32_t max_qubes );
qube_t* qb_qube_from_image ( const char* path );
qube_t* qb_qube_from_texels ( qube_t* prev_frame, GLubyte* texels, uint8_t frames );
qube_t* qb_qubes_get_free ( void );
void qb_qube_render ( context_t* ctx, octant_t* octant, uint8_t top );
void qb_qube_render_solid ( context_t* ctx, octant_t* octant, uint8_t top );
void qb_qube_render_wired ( context_t* ctx, octant_t* octant, uint8_t top );

void qb_context_init ( context_t* ctx );
int qb_pick_select ( context_t* ctx, const vec3_t screen_pos, octant_t** octant );
int qb_pick_extrude ( context_t* ctx, const vec3_t screen_pos, octant_t** octant );
void qb_screen_pick_ray ( context_t* ctx, const vec3_t screen_pos, ray_t* ray );
void qb_screen_to_world ( context_t* ctx, vec3_t pos, const vec3_t screen_pos );
void qb_camera_rotate ( context_t* ctx, const vec3_t rotation );

void qb_octants_init ( uint32_t max_octants );
octant_t* qb_octants_get_free ( void );
void qb_octant_collapse ( octant_t* octant );
void qb_octant_expand ( octant_t* parent, const vec3_t pos, octant_t** octant );
void qb_octant_intersect_ray ( octant_t* parent, const ray_t* pick_ray, float* dist, int* plane, octant_t** qube );
void qb_octant_intersect_point ( octant_t* parent, vec3_t point, octant_t** octant );
void qb_octant_render ( context_t* ctx, octant_t* octant, uint8_t top );

void qb_cuboids_init ( uint32_t max_cuboids );
void qb_cuboid_update ( void );
cuboid_t* qb_cuboid_draw ( context_t* ctx, const aabb_t* aabb, color_t color, uint8_t line_width, float timer );
void qb_cuboid_render ( context_t* ctx );

void qb_render_init ( void );

#endif



