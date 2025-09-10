#version 460

layout (location=0) in vec3 pos;
layout (location=1) in vec3 norm;
layout (location=2) in vec3 col;

out vec3 vs_out_pos;
out vec3 vs_out_norm;
out vec3 vs_out_world_norm;
out vec3 vs_out_col;
out vec3 vs_out_view_pos;

out vec4 vs_out_ndc;

uniform mat4 V;
uniform mat4 MVP;
uniform mat4 world;
uniform mat4 VworldIT;
uniform mat4 worldIT;

void main() {
    gl_Position = MVP * vec4(pos,1);
    vs_out_pos = (world * vec4(pos,1)).xyz;
    vs_out_norm = (VworldIT * vec4(norm, 0)).xyz;
    vs_out_world_norm = (worldIT * vec4(norm,0)).xyz;
    vs_out_col = col;
    vs_out_ndc = MVP * vec4(pos,1);
    vs_out_ndc /= vs_out_ndc.w;

    vs_out_view_pos = (V * world * vec4(pos,1)).xyz;
}