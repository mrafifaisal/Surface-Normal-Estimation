#version 460

layout (location=0) in vec3 pos;
layout (location=1) in vec3 norm;
layout (location=2) in vec3 col;

out vec3 vs_out_pos;
out vec3 vs_out_norm;
out vec3 vs_out_col;

uniform mat4 MVP;
uniform mat4 world;
uniform mat4 worldIT;

void main() {
    gl_Position = MVP * vec4(pos,1);
    vs_out_pos = (world * vec4(pos,1)).xyz;
    vs_out_norm = (worldIT * vec4(norm, 0)).xyz;
    vs_out_col = col;
}