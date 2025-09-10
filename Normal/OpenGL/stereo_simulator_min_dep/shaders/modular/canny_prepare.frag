#version 460

uniform sampler2D tex;

in vec2 uv;
out vec4 col;

void main() {
    col = texture(tex, uv).zzzw;
    col.xyz = col.xyz/50;
}