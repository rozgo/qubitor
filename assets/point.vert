uniform mat4 mvp_mat;
uniform float point_size;

attribute vec4 position;
attribute vec4 color;

varying vec4 frag_color;

void main()
{
    gl_Position = mvp_mat * position;
    gl_PointSize = point_size;
    frag_color = color;
}

