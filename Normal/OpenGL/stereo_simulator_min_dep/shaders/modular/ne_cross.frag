#version 460

uniform sampler2D positions;
uniform ivec2 resolution;
uniform bool symmetric = false; // forward or symmetric mode

in vec2 uv;

out vec3 normal;

void main() {
    float stepx = 2.0f / resolution.x;
    float stepy = 2.0f / resolution.y;

    vec3 right = texture(positions, uv + vec2(stepx, 0)).xyz;
    vec3 top = texture(positions, uv + vec2(0, stepy)).xyz;

    if (symmetric) {
        vec3 left = texture(positions, uv + vec2(-stepx, 0)).xyz;
        vec3 bottom = texture(positions, uv + vec2(0, -stepy)).xyz;
        normal = normalize(cross(right - left, top - bottom));
    } else {
        vec3 center = texture(positions, uv).xyz;
        normal = normalize(cross(right - center, top - center));
    }
}