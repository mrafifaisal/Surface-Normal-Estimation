#version 460

in vec2 tex_coord;

out float noisy_disparity;

uniform bool useNoise;
uniform sampler2D disparities;
uniform sampler2D noise;

uniform float f;
uniform float fovy;
uniform float aspect;
uniform ivec2 resolution;

void main() {
    float d = texture(disparities, tex_coord).x;

    if (useNoise) {
        float h = f * tan(fovy / 2);
        float w = aspect * h;

        float n = texture(noise, tex_coord).x;
        n = n / resolution.x * 2 * w;
        d += n;
    }

    noisy_disparity = d;
}