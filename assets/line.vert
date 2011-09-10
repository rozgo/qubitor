uniform mat4 mvp_mat;

attribute vec4 position;

void main()
{
    gl_Position = mvp_mat * position;
}
