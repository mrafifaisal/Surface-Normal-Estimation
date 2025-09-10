#version 460

out vec4 out_col;

in vec2 tex_coord;

uniform sampler2D leftView;
uniform sampler2D rightView;
uniform sampler2D disparities;

void main() {
    out_col = vec4(1,0,0,1);

    float d = texture(disparities, tex_coord).x+1;
    out_col = vec4(d,d,d,1);
    // out_col = vec4(tex_coord, 0, 1);
     out_col = (texture(leftView, tex_coord) + texture(rightView, tex_coord))/2;
}