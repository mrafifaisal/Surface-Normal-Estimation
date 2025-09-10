#version 460

const vec4[4] pos = vec4[4](vec4(-1, -1, 1, 1), vec4(1, -1, 1, 1),
                            vec4(-1, 1, 1, 1), vec4(1, 1, 1, 1));

out vec2 tex_coord;

void main() {
    gl_Position = pos[gl_VertexID];
    tex_coord = pos[gl_VertexID].xy/2+0.5;
}