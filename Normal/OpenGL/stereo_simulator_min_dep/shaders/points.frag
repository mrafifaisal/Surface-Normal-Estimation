#version 460

uniform sampler2D disparities;
uniform sampler2D colors;
out vec4 out_col;

in vec2 tex_coord;

void main() {   
    out_col = vec4(texture(disparities, tex_coord).xxx*-1,1);
     out_col = vec4(1,1,1,1);
     out_col = texture(colors, tex_coord);
    //  out_col = vec4(tex_coord, 0, 1);
    //out_col = vec4(1,0,1,1);
}