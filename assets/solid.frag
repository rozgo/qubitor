uniform sampler2D solid_texture;
uniform lowp vec4 solid_color;

varying lowp vec2 frag_uv;

void main()
{    
    gl_FragColor = texture2D ( solid_texture, frag_uv ) * solid_color;
}
