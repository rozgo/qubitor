uniform mat4 mvp_mat;

attribute vec4 position;
attribute vec4 uv;

varying vec2 frag_uv;

void main()
{
    gl_Position = mvp_mat * position;
    frag_uv = uv.xy;
}
