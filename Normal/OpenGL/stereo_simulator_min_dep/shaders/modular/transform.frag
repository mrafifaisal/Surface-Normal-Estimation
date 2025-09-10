#version 460

uniform sampler2D base;
uniform bool flipX;
uniform bool flipY;
uniform float multiply;

uniform mat4 color_transform;
uniform vec4 color_shift;
uniform bool normalize3;

in vec2 uv;
out vec4 col;

void main() {
    vec2 coord = uv;
    if (flipX)
        coord.x = 1 - coord.x;
    if (flipY)
        coord.y = 1 - coord.y;

    col = texture(base, coord) * multiply;
    col = color_transform * col + color_shift;
    if (normalize3) {
        col.xyz = normalize(col.xyz);
    }
}
