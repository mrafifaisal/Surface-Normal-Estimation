#version 460

uniform sampler2D disparities;

in vec2 uv;
out vec3 pos;

uniform float fx;
uniform float fy;
uniform float b;
uniform ivec2 res;
uniform vec2 c;
uniform float doffs = 0;

uniform float scale = 1.0f;

void main() {
    float d = texture(disparities, uv).x;

    float z = fx / (d+doffs) * b;

    vec2 uvpx = uv * res - c;

    float x = uvpx.x / fx * z;
    float y = uvpx.y / fy * z;

    x *= -1; // change to x right, y up, z forward

    pos = vec3(x, y, z) * scale;
}