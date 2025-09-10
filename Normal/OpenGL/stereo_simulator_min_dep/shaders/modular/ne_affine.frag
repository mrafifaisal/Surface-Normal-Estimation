#version 460

uniform sampler2D affine_params;
uniform sampler2D positions;

uniform float fx;
uniform float fy;
uniform float baseline;

in vec2 uv;
out vec3 normal;

void main() {
    vec3 p = texture(positions, uv).xyz;
    vec2 a = texture(affine_params, uv).xy;

    // old, wrong, bot produces right output:
    // float x = baseline * fx * p.z * (a.x-1);
    // float y = a.y * baseline * fy * p.z;
    // float z = baseline * (-a.x*fx*p.x-a.y*fy*p.y+baseline*fx+fx*p.x);

    // actual paper:
    float x = baseline*fx*p.z*(1-a.x);
    float y = -a.y*baseline*fy*p.z;
    float z = baseline*(a.x*fx*p.x+a.y*fy*p.y+baseline*fx-fx*p.x);
    normal = normalize(vec3(x,y,z));
}