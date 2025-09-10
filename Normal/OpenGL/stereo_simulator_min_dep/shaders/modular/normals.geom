#version 460

layout(points) in;
layout(line_strip, max_vertices = 2) out;

in vec3 normal[];
uniform mat4 VP;
uniform float length;

uniform bool both_sides = true;
uniform bool smart_invert = true;
uniform bool screen_space_length = true;
uniform vec3 cam_pos;

void main() {
    vec3 n = normalize(normal[0]) * length;
    if(screen_space_length)
        n *= distance(gl_in[0].gl_Position.xyz, cam_pos)/3;
    if(smart_invert){
        if(n.z > 0 || n.y < 0){
            n = -n;
        }
    }
    
    if(both_sides)
        gl_Position = VP * vec4(gl_in[0].gl_Position.xyz - n, 1);
    else
        gl_Position = VP * vec4(gl_in[0].gl_Position.xyz, 1);
    EmitVertex();

    gl_Position =
        VP * vec4(gl_in[0].gl_Position.xyz + normal[0] * length, 1);
    gl_Position =
        VP * vec4(gl_in[0].gl_Position.xyz + n, 1);
    EmitVertex();
    EndPrimitive();
}