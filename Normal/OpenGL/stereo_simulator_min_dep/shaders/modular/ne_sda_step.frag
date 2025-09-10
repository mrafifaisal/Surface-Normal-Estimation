#version 460

uniform sampler2D laplacian;
uniform sampler2D prevGE;
uniform sampler2D depth;
uniform ivec2 resolution;

uniform isampler2D prev_idx;
uniform int iter_idx;

in vec2 uv;
layout(location=0) out vec4 newGE;
layout(location=1) out ivec2 new_idx;

void main() {
    vec2 pxsize = vec2(1.0f / resolution.x, 1.0f / resolution.y);
    vec2 right = uv + vec2(pxsize.x, 0);
    vec2 left = uv + vec2(-pxsize.x, 0);
    vec2 up = uv + vec2(0, pxsize.y);
    vec2 down = uv + vec2(0, -pxsize.y);

    float temp_xlap_u = abs(texture(laplacian, up).x);
    float temp_xlap_d = abs(texture(laplacian, down).x);
    vec2 temp_E_u = texture(prevGE, up).zw;
    vec2 temp_E_d = texture(prevGE, down).zw;
    vec2 E_ul = texture(prevGE, uv + vec2(-pxsize.x, pxsize.y)).zw;
    vec2 E_ur = texture(prevGE, uv + vec2(pxsize.x, pxsize.y)).zw;
    vec2 E_dl = texture(prevGE, uv + vec2(-pxsize.x, -pxsize.y)).zw;
    vec2 E_dr = texture(prevGE, uv + vec2(pxsize.x, -pxsize.y)).zw;
    vec2 E_l = texture(prevGE, left).zw;
    vec2 E_r = texture(prevGE, right).zw;

    float xlap_l = abs(texture(laplacian, left).x);
    float xlap_r = abs(texture(laplacian, right).x);
    float ylap_u = abs(texture(laplacian, up).y);
    float ylap_d = abs(texture(laplacian, down).y);
    vec2 lap_ul = abs(texture(laplacian, uv + vec2(-pxsize.x, pxsize.y)).xy);
    vec2 lap_ur = abs(texture(laplacian, uv + vec2(pxsize.x, pxsize.y)).xy);
    vec2 lap_dl = abs(texture(laplacian, uv + vec2(-pxsize.x, -pxsize.y)).xy);
    vec2 lap_dr = abs(texture(laplacian, uv + vec2(pxsize.x, -pxsize.y)).xy);

    float con_L = 1 - sign(texture(laplacian, left).x *
                           texture(laplacian, uv + vec2(-2 * pxsize.x, 0)).x);
    float con_R = 1 - sign(texture(laplacian, right).x *
                           texture(laplacian, uv + vec2(2 * pxsize.x, 0)).x);
    float con_U = 1 - sign(texture(laplacian, up).y *
                           texture(laplacian, uv + vec2(0, 2 * pxsize.y)).y);
    float con_D = 1 - sign(texture(laplacian, down).y *
                           texture(laplacian, uv + vec2(0, -2 * pxsize.y)).y);

    ivec2 lastminidx = texture(prev_idx, uv).xy;

    float MAX = 1e10;
    //(abs(umin_index-1)+abs([inf*ones(VMAX, 1), umin_index(:,
    // 1:UMAX-1)]-1)+con_L)*MAX

    // [Z_ulaplace(:, 2:UMAX), inf*ones(VMAX,
    // 1)]+(abs(umin_index-2)+abs([umin_index(:, 2:UMAX), inf*ones(VMAX,
    // 1)]-2)+con_R)*MAX
    float Eu_stack[] =
        float[](xlap_l + (abs(lastminidx.x - 1) +
                          abs(texture(prev_idx, left).x - 1) + con_L) *
                             MAX, // left
                xlap_r + (abs(lastminidx.x - 2) +
                          abs(texture(prev_idx, right).x - 2) + con_R) *
                             MAX,                        // right
                texture(prevGE, uv).z,                   // prev Eu
                2 * (ylap_u + temp_E_u.x),               // up
                2 * (ylap_d + temp_E_d.x),               // down
                ylap_u + temp_E_u.x + lap_ul.x + E_ul.y, // up left
                ylap_u + temp_E_u.x + lap_ur.x + E_ur.y, // up right
                ylap_d + temp_E_d.x + lap_dl.x + E_dl.y, // down left
                ylap_d + temp_E_d.x + lap_dr.x + E_dr.y  // down right
        );

    // [inf*ones(1, UMAX); Z_vlaplace(1:VMAX-1,
    // :)]+(abs(vmin_index-1)+abs([inf*ones(1, UMAX); vmin_index(1:VMAX-1,
    // :)]-1)+con_U)*MAX

    //[Z_vlaplace(2:VMAX, :); inf*ones(1,
    // UMAX)]+(abs(vmin_index-2)+abs([vmin_index(2:VMAX, :); inf*ones(1,
    // UMAX)]-2)+con_D)*MAX
    float Ev_stack[] =
        float[](ylap_u + (abs(lastminidx.y - 1) +
                          abs(texture(prev_idx, up).y - 1) + con_U) *
                             MAX, // up
                ylap_d + (abs(lastminidx.y - 2) +
                          abs(texture(prev_idx, down).y - 2) + con_D) *
                             MAX,                   // down
                texture(prevGE, uv).w,              // no change
                2 * (xlap_l + E_l.y),               // left
                2 * (xlap_r + E_r.y),               // right
                xlap_l + E_l.y + lap_ul.y + E_ul.x, // up left
                xlap_l + E_l.y + lap_dl.y + E_dl.x, // down left
                xlap_r + E_r.y + lap_ur.y + E_ur.x, // up right
                xlap_r + E_r.y + lap_dr.y + E_dr.x  // down right
        );

    vec2 G = texture(prevGE, uv).xy;
    vec2 G_l = texture(prevGE, left).xy;
    vec2 G_r = texture(prevGE, right).xy;
    vec2 G_u = texture(prevGE, up).xy;
    vec2 G_d = texture(prevGE, down).xy;
    float Z = texture(depth, uv).z;
    float Z_l = texture(depth, left).z;
    float Z_r = texture(depth, right).z;
    float Z_u = texture(depth, up).z;
    float Z_d = texture(depth, down).z;
    vec2 G_ul = texture(prevGE, uv + vec2(-pxsize.x, pxsize.y)).xy;
    vec2 G_ur = texture(prevGE, uv + vec2(pxsize.x, pxsize.y)).xy;
    vec2 G_dl = texture(prevGE, uv + vec2(-pxsize.x, -pxsize.y)).xy;
    vec2 G_dr = texture(prevGE, uv + vec2(pxsize.x, -pxsize.y)).xy;
    float Gu_stack[] =
        float[](G.x + 1 / (iter_idx + 1.0f) * Z -
                    1 / (iter_idx + 1.0f) * (Z_l + G_l.x), // left
                G.x - 1 / (iter_idx + 1.0f) * Z +
                    1 / (iter_idx + 1.0f) * (Z_r - G_r.x), // right
                G.x,                                       // no change
                G_u.x,                                     // up
                G_d.x,                                     // down
                G_u.x - G_ul.y + G.y,                      // up left
                G_u.x + G_ur.y - G.y,                      // up right
                G_d.x + G_dl.y - G.y,                      // down left
                G_d.x - G_dr.y + G.y                       // down right
        );

    // original checked:
    // float Gv_stack[] =
    //     float[](G.y + 1 / (iter_idx + 1.0f) * Z -
    //                 1 / (iter_idx + 1.0f) * (Z_u + G_u.y), // up
    //             G.y - 1 / (iter_idx + 1.0f) * Z +
    //                 1 / (iter_idx + 1.0f) * (Z_d - G_d.y), // down
    //             G.y,                                       // no change
    //             G_l.y,                                     // left
    //             G_r.y,                                     // right
    //             G_l.y - G_ul.x + G.x,                      // up left
    //             G_l.y + G_dl.x - G.x,                      // down left
    //             G_r.y + G_ur.x - G.x,                      // up right
    //             G_r.y - G_dr.x + G.x                       // down right
    //     );
    
    // vertical flip fix:
    float Gv_stack[] =
        float[](G.y - 1 / (iter_idx + 1.0f) * Z +
                    1 / (iter_idx + 1.0f) * (Z_u - G_u.y), // up
                G.y + 1 / (iter_idx + 1.0f) * Z -
                    1 / (iter_idx + 1.0f) * (Z_d + G_d.y), // down
                G.y,                                       // no change
                G_l.y,                                     // left
                G_r.y,                                     // right
                G_l.y - G_ul.x + G.x,                      // up left
                G_l.y + G_dl.x - G.x,                      // down left
                G_r.y + G_ur.x - G.x,                      // up right
                G_r.y - G_dr.x + G.x                       // down right
        );

    new_idx = ivec2(0, 0);
    for (int i = 1; i < Eu_stack.length(); ++i) {
        if (Eu_stack[i] < Eu_stack[new_idx.x])
            new_idx.x = i;
    }
    for (int i = 1; i < Ev_stack.length(); ++i) {
        if (Ev_stack[i] < Ev_stack[new_idx.y])
            new_idx.y = i;
    }

    newGE = vec4(Gu_stack[new_idx.x], Gv_stack[new_idx.y], Eu_stack[new_idx.x],
                 Ev_stack[new_idx.y]);
    new_idx += 1;
}