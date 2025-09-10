#version 460

uniform sampler2D depth;
uniform sampler2D laplacian;
uniform ivec2 resolution;

in vec2 uv;
layout(location=0) out vec4 init_state;
layout(location=1) out ivec2 selected_idx;

void main() {
    vec2 pxsize = vec2(1.0f / resolution.x, 1.0f / resolution.y);
    vec2 right = uv + vec2(pxsize.x, 0);
    vec2 left = uv + vec2(-pxsize.x, 0);
    vec2 up = uv + vec2(0, pxsize.y);
    vec2 down = uv + vec2(0, -pxsize.y);

    float dl = texture(depth, left).z;
    float dr = texture(depth, right).z;
    float du = texture(depth, up).z;
    float dd = texture(depth, down).z;
    float dc = texture(depth, uv).z;

    float gx = 0.5 * dr - 0.5 * dl; // symmetric dif x
    float gxr = dr - dc;            // forward dif x
    float gxl = dc - dl;            // backward dif x

    float gy = 0.5 * du - 0.5 * dd;
    float gyu = du - dc; // maybe *-1
    float gyd = dc - dd; // maybe *-1

    vec2 lap = abs(texture(laplacian, uv).xy);
    float lapr = abs(texture(laplacian, right).x);
    float lapl = abs(texture(laplacian, left).x);
    float lapu = abs(texture(laplacian, up).y);
    float lapd = abs(texture(laplacian, down).y);

    // select min of lap, lapleft, lapright
    float minEu = lapl;
    float Gu = gxl;
    selected_idx.x = 1; // 2
    if (lap.x < minEu) {
        minEu = lap.x;
        Gu = gx;
        selected_idx.x = 3; // 0
    }
    if (lapr < minEu) {
        minEu = lapr;
        Gu = gxr;
        selected_idx.x = 2; // 1
    }

    float minEv = lapd;
    float Gv = gyd;
    selected_idx.y = 2; // 1
    if (lap.y < minEv) {
        minEv = lap.y;
        Gv = gy;
        selected_idx.y = 3; // 0
    }
    if (lapu < minEv) {
        minEv = lapu;
        Gv = gyu;
        selected_idx.y = 1; // 2
    }

    init_state = vec4(Gu, Gv, minEu, minEv);
}