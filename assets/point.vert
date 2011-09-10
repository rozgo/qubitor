uniform mat4 mvp_mat;
uniform float point_size;

attribute vec4 position;
attribute vec4 color;

varying vec4 frag_color;

void main()
{
    vec4 bit_pos = position * 0.0625 - 0.53125;
    bit_pos.w = 1.0;
    gl_Position = mvp_mat * bit_pos;
    gl_PointSize = point_size;
    frag_color = color;
}
