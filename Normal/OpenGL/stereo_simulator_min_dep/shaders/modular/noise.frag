#version 460

uniform sampler2D base;
uniform sampler2D noise;
uniform bool pass;

in vec2 uv;
out float noisy;

void main() {
    float b = texture(base, uv).x;
    float n = texture(noise, uv).x;
    if (pass)
        noisy = b;
    else
        noisy = b + n;
}