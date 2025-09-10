#version 460

uniform sampler2D inputTex;
uniform ivec2 resolution;

in vec2 uv;
out vec2 laplacian;

void main() {
    vec2 pxsize = vec2(1.0f/resolution.x, 1.0f/resolution.y);

    // use the z coordinate for compatibility with position textures
    float left = texture(inputTex, uv + vec2(-pxsize.x,0)).z;
    float right = texture(inputTex, uv + vec2(pxsize.x,0)).z;
    float up = texture(inputTex, uv + vec2(0,pxsize.y)).z;
    float down = texture(inputTex, uv + vec2(0,-pxsize.y)).z;

    float center = texture(inputTex, uv).z;

    laplacian = vec2(right-2*center+left, up-2*center+down);
}