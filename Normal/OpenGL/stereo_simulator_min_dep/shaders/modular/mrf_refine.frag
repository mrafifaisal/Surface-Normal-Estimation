#version 460

uniform sampler2D normals;
uniform sampler2D laplacian;
uniform ivec2 resolution;
uniform bool pass;

in vec2 uv;
out vec3 norm;

#define dc 5
const vec2 dirs[dc] =
    vec2[dc](vec2(0, 0), vec2(-1, 0), vec2(1, 0), vec2(0, 1), vec2(0, -1));

float vsum(vec2 v) { return v.x + v.y; }

// based on the simplified implementation of Rui Fan
// https://github.com/fengyi233/depth-to-normal-translator
void main() {
    if(pass){
        norm = texture(normals,uv).xyz;
        return;
    }

    vec2 pxsize = vec2(1.0f / resolution.x, 1.0f / resolution.y);
    float lowest = vsum(abs(texture(laplacian, uv + dirs[0] * pxsize).xy))/2;
    int mini = 0;
    for (int i = 1; i < dc; ++i) {
        vec2 lap = abs(texture(laplacian, uv + dirs[i] * pxsize).xy);
        float s = lap.x;
        if(i > 2)
            s = lap.y;
        if (s < lowest) {
            lowest = s;
            mini = i;
        }
    }

    norm = texture(normals, uv + dirs[mini] * pxsize).xyz;
}