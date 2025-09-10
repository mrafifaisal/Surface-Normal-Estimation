// NOTE: every disparity is negated in this file

#version 460

#define MAX_ADAPTIVE_DIRS 32

uniform sampler2D disparities;
uniform sampler2D weights;
uniform sampler2D depth;
uniform ivec2 resolution;
uniform float conv1[15 * 15];
uniform float conv2[15 * 15];
uniform vec2 v[15 * 15];
uniform float adaptive_simple_thresh = 1.0f;
uniform float adaptive_cumulative_thresh = 1.0f;
uniform int adaptive_dir_count = 8;
uniform vec2 adaptive_dirs[MAX_ADAPTIVE_DIRS];
uniform int adaptive_starts[MAX_ADAPTIVE_DIRS];
uniform int adaptive_max_step_count = 5;
uniform float adaptive_depth_thresh_ratio = 0.1f;
uniform int conv_size = 3;
uniform int mode = 0; // 0 basic, 1 lsq, 2 adaptive

uniform float costhresh = cos(0.3490659); // cos(20˚)
uniform int ang_min_steps = 3;

in vec2 uv;
out vec2 affine;

vec2 affine_nb(vec2 center, vec2 pxsize) {
    float dc = -texture(disparities, center).x;
    float dr = -texture(disparities, center + vec2(pxsize.x, 0)).x;
    float dt = -texture(disparities, center + vec2(0, pxsize.y)).x;

    float scale =
        (1 - dr + dc); // original // this is because positive x is left
    // otherwise it would be 1 + dr - dc;
    // float scale = (1+dr-dc); // new
    float shear = (dt - dc); /// TODO: explain -1 // it is right
    // float shear = (dc-dt); // new
    return vec2(scale, shear);
}

vec2 affine_lsq(vec2 center, vec2 pxsize) {
    float dc = -texture(disparities, center).x;
    vec2 affine = vec2(0);

    for (int i = 0; i < conv_size; ++i) {
        // float d = texture(disparities, center + v[i]*pxsize).x;
        float d = -texture(disparities, center + v[i] * pxsize).x;
        d -= dc;
        affine.x += d * conv1[i];
        affine.y += d * conv2[i];
    }
    affine.x = 1 - affine.x; /// TODO: explain 1-affine.x instead of 1+affine.x
    // affine.x += 1; // new
    // affine.y *= -1; // new
    return affine;
}

vec2 weight(vec2 pos) {
    vec2 lap = abs(texture(weights, pos).xy);
    // return adaptive_cumulative_thresh-(abs(lap.x) + abs(lap.y)); // TODO:
    // reverse change if(lap.x >= 1)
    //     return vec2(adaptive_cumulative_thresh,0)*1000;
    return lap;
}

vec2 adaptive_affine_simple_threshold(vec2 center, vec2 pxsize) {
    float a = 0;
    float b = 0;
    float c = 0;

    int ends[MAX_ADAPTIVE_DIRS];
    for (int i = 0; i < adaptive_dir_count; ++i) {
        int k = 0;
        vec2 pos = vec2(0, 0);
        float d;
        do {
            a += pos.x * pos.x;
            b += pos.x * pos.y;
            c += pos.y * pos.y;
            pos += adaptive_dirs[i];
            d = texture(depth, center + pos * pxsize).z;
            ++k;
        } while (k < adaptive_max_step_count &&
                 length(weight(center + pos * pxsize)) <
                     adaptive_simple_thresh*d);
        ends[i] = k;
    }

    float D = a * c - b * b;

    if (abs(D) < 1e-10) // division by 0, fall back to just taking 2 neighbours
        return affine_nb(center, pxsize);
    else {
        float dc = -texture(disparities, center).x;
        vec2 affine = vec2(0);
        float inv = 1 / D;
        for (int i = 0; i < adaptive_dir_count; ++i) {
            vec2 pos = vec2(0, 0);
            for (int j = 0; j < ends[i]; ++j) {
                float d = -texture(disparities, center + pos * pxsize).x;
                d -= dc;
                affine.x += d * (pos.x * c - pos.y * b) * inv;  // * w;
                affine.y += d * (-pos.x * b + pos.y * a) * inv; // * w;
                pos += adaptive_dirs[i];
            }
        }

        affine.x = 1 - affine.x;
        return affine;
    }
}

vec2 adaptive_affine_lsq(vec2 center, vec2 pxsize) {
    float a = 0;
    float b = 0;
    float c = 0;

    int ends[MAX_ADAPTIVE_DIRS];
    for (int i = 0; i < adaptive_dir_count; ++i) {
        int k = 0;
        vec2 pos = vec2(0, 0);
        vec2 w = weight(center);
        vec2 w2 = w * w;
        vec2 lastw = vec2(0, 0);
        do {
            w2 = vec2(1, 1);    // TODO: go back to weighting (remove this line)
            a += pos.x * pos.x; // * w2;
            b += pos.x * pos.y; // * w2;
            c += pos.y * pos.y; // * w2;
            pos += adaptive_dirs[i];
            lastw = weight(center + pos * pxsize);
            w += lastw; // TODO: remove sum
            w2 = w * w;
            ++k;
        } while (k < adaptive_max_step_count &&
                 length(w) < adaptive_cumulative_thresh);
        ends[i] = k;
    }

    float D = a * c - b * b;

    if (abs(D) < 1e-10) // division by 0, fall back to just taking 2 neighbours
        return affine_nb(center, pxsize);
    else {
        float dc = -texture(disparities, center).x;
        vec2 affine = vec2(0);
        float inv = 1 / D;
        for (int i = 0; i < adaptive_dir_count; ++i) {
            vec2 pos = vec2(0, 0);
            for (int j = 0; j < ends[i]; ++j) {
                float d = -texture(disparities, center + pos * pxsize).x;
                d -= dc;
                affine.x += d * (pos.x * c - pos.y * b) * inv;  // * w;
                affine.y += d * (-pos.x * b + pos.y * a) * inv; // * w;
                pos += adaptive_dirs[i];
            }
        }

        affine.x =
            1 - affine.x; /// TODO: explain 1-affine.x instead of 1+affine.x
        // affine.x += 1; // new
        // affine.y *= -1; // new
        return affine;
        // return vec2(c,b);
    }
}

vec2 adaptive_affine_lsq_min_max(vec2 center, vec2 pxsize) {
    float a = 0;
    float b = 0;
    float c = 0;
    float d_o = texture(depth, center).z;
    float d_thresh = d_o * adaptive_depth_thresh_ratio;
    for (int i = 0; i < adaptive_dir_count; ++i) {
        int k = 0;
        vec2 pos = vec2(0, 0);
        float d_min = d_o;
        float d_max = d_o;
        do {
            a += pos.x * pos.x; // * w2;
            b += pos.x * pos.y; // * w2;
            c += pos.y * pos.y; // * w2;
            pos += adaptive_dirs[i]*sqrt(2);
            float d = texture(depth, center + pos * pxsize).z;
            d_min = min(d_min, d);
            d_max = max(d_max, d);
            ++k;
        } while (k < adaptive_max_step_count && (d_max - d_min) < d_thresh);
    }

    float dc = -texture(disparities, center).x;
    vec2 affine = vec2(0);
    float inv = 1 / (a * c - b * b);
    for (int i = 0; i < adaptive_dir_count; ++i) {
        int k = 0;
        vec2 pos = vec2(0, 0);
        float d_min = d_o;
        float d_max = d_o;
        do {
            float disp = -texture(disparities, center + pos * pxsize).x;
            disp -= dc;
            affine.x += disp * (pos.x * c - pos.y * b) * inv;  // * w;
            affine.y += disp * (-pos.x * b + pos.y * a) * inv; // * w;
            pos += adaptive_dirs[i]*sqrt(2);
            float d = texture(depth, center + pos * pxsize).z;
            d_min = min(d_min, d);
            d_max = max(d_max, d);
            ++k;
        } while (k < adaptive_max_step_count && (d_max - d_min) < d_thresh);
    }

    affine.x = 1 - affine.x; /// TODO: explain 1-affine.x instead of 1+affine.x
    // affine.x += 1; // new
    // affine.y *= -1; // new
    return affine;
    // return vec2(c,b);
}

vec2 affine_test(vec2 center, vec2 pxsize) {
    int ends[MAX_ADAPTIVE_DIRS];
    float cd = texture(depth, center).z;

    for (int i = 0; i < adaptive_dir_count; ++i) {
        vec2 pos = vec2(0, 0);
        int k = 1;
        float sum = 0;
        float xsum = 0;
        // sum += texture(depth, center+pos*pxsize).z-cd; // 0
        pos += adaptive_dirs[i];
        sum += texture(depth, center + pos * pxsize).z - cd;
        float cosang = 1;

        float d = 0;
        // first pass
        do {
            xsum += k;
            sum += d;
            pos += adaptive_dirs[i];
            k++;
            d = texture(depth, center + pos * pxsize).z - cd;
            vec2 avg = vec2(xsum, sum); // /k
            vec2 cur = vec2(k, d);
            cosang = dot(normalize(avg), normalize(cur));

        } while (k <= adaptive_max_step_count &&
                 (k < ang_min_steps || cosang > costhresh));

        ends[i] = k - 1;

        // second pass
        pos = vec2(0, 0);
        float maxcos = -2;
        int bestidx = 0;
        float leftsum = 0;
        float xleftsum = 0;
        vec2 endp =
            vec2(ends[i], texture(depth, center + pos * pxsize * ends[i]));
        for (int j = 1; j < ends[i]; ++j) {
            float d =
                texture(depth, center + adaptive_dirs[i] * j * pxsize).z - cd;

            // current is included on both sides (avoiding zero vectors at
            // start)
            float rightsum = sum - leftsum;
            leftsum += d;
            float xrightsum = xsum - xleftsum;
            xleftsum += j;

            vec2 avgleft = vec2(xleftsum, leftsum);
            vec2 cur = vec2(j, d);
            float cosleft = dot(normalize(avgleft), cur);
            vec2 avgright = vec2(xrightsum, rightsum) - endp * (ends[i] - j);
            vec2 curright = cur - endp;
            float cosright = dot(normalize(avgright), normalize(curright));
            float totalcos = cosleft + cosright;
            if (totalcos > maxcos) {
                maxcos = totalcos;
                bestidx = j;
            }
        }

        ends[i] = bestidx - 1;
    }

    // affine.x = ends[0];
    // affine.y = ends[1];
    // return affine;

    float a = 0;
    float b = 0;
    float c = 0;
    for (int i = 0; i < adaptive_dir_count; ++i) {
        for (int j = 0; j <= ends[i]; ++j) {
            vec2 pos = adaptive_dirs[i] * j;
            a += pos.x * pos.x; // * w2;
            b += pos.x * pos.y; // * w2;
            c += pos.y * pos.y; // * w2;
        }
    }

    float dispc = -texture(disparities, center).x;
    vec2 affine = vec2(0);
    float inv = 1 / (a * c - b * b);
    for (int i = 0; i < adaptive_dir_count; ++i) {
        for (int j = 0; j <= ends[i]; ++j) {
            vec2 pos = adaptive_dirs[i] * j;
            float disp = -texture(disparities, center + pos * pxsize).x;
            disp -= dispc;
            affine.x += disp * (pos.x * c - pos.y * b) * inv;  // * w;
            affine.y += disp * (-pos.x * b + pos.y * a) * inv; // * w;
        }
    }

    affine.x = 1 - affine.x; /// TODO: explain 1-affine.x instead of 1+affine.x
    // affine.x += 1; // new
    // affine.y *= -1; // new
    return affine;
}

vec2 affine_test_3d(vec2 center, vec2 pxsize) {
    int ends[MAX_ADAPTIVE_DIRS];
    vec3 cd = texture(depth, center).xyz; // depth is actually pos

    for (int i = 0; i < adaptive_dir_count; ++i) {
        vec2 pos = vec2(0, 0);
        int k = 1;
        vec3 sum = vec3(0, 0, 0);
        // sum += texture(depth, center+pos*pxsize).z-cd; // 0
        pos += adaptive_dirs[i];
        sum += texture(depth, center + pos * pxsize).xyz - cd;
        float cosang = 1;

        vec3 d = vec3(0, 0, 0);
        // first pass
        do {
            sum += d;
            pos += adaptive_dirs[i];
            k++;
            d = texture(depth, center + pos * pxsize).xyz - cd;
            vec3 avg = sum; // /k
            vec3 cur = d;
            cosang = dot(normalize(avg), normalize(cur));

        } while (k <= adaptive_max_step_count &&
                 (k < ang_min_steps || cosang > costhresh));

        ends[i] = k - 1;

        // second pass
        pos = vec2(0, 0);
        float maxcos = -2;
        int bestidx = 0;
        vec3 leftsum = vec3(0, 0, 0);
        vec3 endp = texture(depth, center + pos * pxsize * ends[i]).xyz;
        for (int j = 1; j < ends[i]; ++j) {
            vec3 d =
                texture(depth, center + adaptive_dirs[i] * j * pxsize).xyz - cd;

            // current is included on both sides (avoiding zero vectors at
            // start)
            vec3 rightsum = sum - leftsum;
            leftsum += d;

            vec3 avgleft = leftsum;
            vec3 cur = d;
            float cosleft = dot(normalize(avgleft), cur);
            vec3 avgright = rightsum - endp * (ends[i] - j);
            vec3 curright = cur - endp;
            float cosright = dot(normalize(avgright), normalize(curright));
            float totalcos = cosleft + cosright;
            if (totalcos > maxcos) {
                maxcos = totalcos;
                bestidx = j;
            }
        }

        ends[i] = bestidx - 1;
    }

    // affine.x = ends[0];
    // affine.y = ends[1];
    // return affine;

    float a = 0;
    float b = 0;
    float c = 0;
    for (int i = 0; i < adaptive_dir_count; ++i) {
        for (int j = 0; j <= ends[i]; ++j) {
            vec2 pos = adaptive_dirs[i] * j;
            a += pos.x * pos.x; // * w2;
            b += pos.x * pos.y; // * w2;
            c += pos.y * pos.y; // * w2;
        }
    }

    float dispc = -texture(disparities, center).x;
    vec2 affine = vec2(0);
    float inv = 1 / (a * c - b * b);
    for (int i = 0; i < adaptive_dir_count; ++i) {
        for (int j = 0; j <= ends[i]; ++j) {
            vec2 pos = adaptive_dirs[i] * j;
            float disp = -texture(disparities, center + pos * pxsize).x;
            disp -= dispc;
            affine.x += disp * (pos.x * c - pos.y * b) * inv;  // * w;
            affine.y += disp * (-pos.x * b + pos.y * a) * inv; // * w;
        }
    }

    affine.x = 1 - affine.x; /// TODO: explain 1-affine.x instead of 1+affine.x
    // affine.x += 1; // new
    // affine.y *= -1; // new
    return affine;
}

void main() {
    vec2 pxsize = vec2(1.0f / resolution.x, 1.0f / resolution.y);

    vec2 center = uv;

    if (mode == 0)
        affine = affine_nb(center, pxsize);
    else if (mode == 1)
        affine = affine_lsq(center, pxsize);
    else if (mode == 2)
        affine = adaptive_affine_lsq(center, pxsize);
    else if (mode == 3)
        affine = adaptive_affine_lsq_min_max(center, pxsize);
    else if (mode == 4)
        affine = affine_test_3d(center, pxsize);
    else if (mode == 5)
        affine = adaptive_affine_simple_threshold(center, pxsize);
}