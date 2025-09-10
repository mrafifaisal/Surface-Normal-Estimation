#version 460

out vec4 col;

uniform vec3 color;

void main() {
    col = vec4(color, 1);
}