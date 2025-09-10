#version 460

uniform sampler2D depth;
uniform sampler2D gradient;

uniform float fx;
uniform float fy;
uniform vec2 c;
uniform ivec2 res;

in vec2 uv;
out vec3 norm;

void main() {
    float z = texture(depth, uv).z;
    vec2 g = texture(gradient, uv).xy;

    vec2 px = uv * res;

    norm.x = fx * g.x;
    norm.y = -fy * g.y;
    norm.z = (px.x - c.x) * g.x + (px.y - c.y) * g.y + z;
}