uniform sampler2D point_texture;

varying lowp vec4 frag_color;

void main()
{
    lowp vec4 tex_color = texture2D ( point_texture, gl_PointCoord );

//    if ( tex_color.a < 1.0 )
//    {
//        discard;
//    }
    
//    gl_FragColor = tex_color * frag_color;
    gl_FragColor = frag_color;
}


