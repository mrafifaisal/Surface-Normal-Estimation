#version 460

layout(points) in;
layout(line_strip, max_vertices = 2) out;

in vec3 normal[];
uniform mat4 VP;
uniform float length;

void main() {
    gl_Position = VP * gl_in[0].gl_Position;
    EmitVertex();

    gl_Position =
        VP * vec4(gl_in[0].gl_Position.xyz + normal[0] * length, 1);
    gl_Position =
        VP * vec4(gl_in[0].gl_Position.xyz + normalize(normal[0]) * length, 1);
    EmitVertex();
    EndPrimitive();
}