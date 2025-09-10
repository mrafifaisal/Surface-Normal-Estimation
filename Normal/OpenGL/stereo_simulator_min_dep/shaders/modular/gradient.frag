#version 460

#define E 2.7182818284590452353602874

uniform sampler2D inputTex;
uniform sampler2D laplacian;
uniform ivec2 resolution;

uniform bool discontinuity_aware = true;
uniform float softness = 1;

in vec2 uv;
out vec2 gradient;

vec2 softmin(vec2 vals) {
    float p1 = pow(E, -vals.x / softness);
    float p2 = pow(E, -vals.y / softness);
    float s = p1 + p2;

    vec2 res = vec2(p1 / s, p2 / s);
    return res;
}

vec2 trunc_softmin(vec2 vals) {
    vec2 sm = softmin(vals);
    if (sm.x / sm.y > E) {
        sm = vec2(1, 0);
    } else if (sm.y / sm.x > E) {
        sm = vec2(0, 1);
    }
    return sm;
}

void main() {
    vec2 pxsize = vec2(1.0f / resolution.x, 1.0f / resolution.y);

    // use the z coordinate for compatibility with position textures
    float left = texture(inputTex, uv + vec2(-pxsize.x, 0)).z;
    float right = texture(inputTex, uv + vec2(pxsize.x, 0)).z;
    float up = texture(inputTex, uv + vec2(0, pxsize.y)).z;
    float down = texture(inputTex, uv + vec2(0, -pxsize.y)).z;
    float center = texture(inputTex, uv).z;

    if (!discontinuity_aware)
        gradient = vec2(right - left, up - down) / 2;
    else {
        vec2 grad_ver = vec2(center - down, up - center);
        vec2 grad_hor = vec2(center - left, right - center);

        float sl = abs(texture(laplacian, uv + vec2(-pxsize.x, 0)).x);
        float sr = abs(texture(laplacian, uv + vec2(pxsize.x, 0)).x);
        float su = abs(texture(laplacian, uv + vec2(0, pxsize.y)).y);
        float sd = abs(texture(laplacian, uv + vec2(0, -pxsize.y)).y);

        vec2 lambdah = trunc_softmin(vec2(sl,sr));
        vec2 lambdav = trunc_softmin(vec2(sd,su));

        gradient.x = grad_hor.x*lambdah.x + grad_hor.y*lambdah.y;
        gradient.y = grad_ver.x*lambdav.x + grad_ver.y*lambdav.y;
    }
}