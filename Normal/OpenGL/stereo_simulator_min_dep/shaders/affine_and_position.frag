#version 460

out vec2 affine;
out vec3 position;

in vec2 tex_coord;

uniform sampler2D disparities;
uniform ivec2 resolution;
uniform float f;
uniform float fovy;
uniform float aspect;
uniform float b;

uniform float conv1[10*10];
uniform float conv2[10*10];
uniform vec2 v[10*10];
uniform int conv_size = 3;
uniform bool enable_lsq = false;
uniform float disparity_coeff = 1.0f;

vec2 affine_nb(vec2 center, vec2 pxsize) {
    float dc = texture(disparities, center).x * disparity_coeff;
    float dr = texture(disparities, center + vec2(pxsize.x, 0)).x * disparity_coeff;
    float dt = texture(disparities, center + vec2(0, pxsize.y)).x * disparity_coeff;

    float h = f * tan(fovy / 2);
    float w = aspect * h;

    float spx = 2 * w * pxsize.x;
    float spy = 2 * h * pxsize.y;
    float scale = (spx - dr + dc) / spx;
    float shear = (dc - dt) / spy;
    return vec2(scale, shear);
}

vec2 affine_lsq(vec2 center, vec2 pxsize) {
    float h = f * tan(fovy / 2);
    float w = aspect * h;
    // pxsize.x = 2*w*pxsize.x;
    // pxsize.y = 2*h*pxsize.y;
    // int idx = 0;
    float dc = texture(disparities, center).x * disparity_coeff;
    vec2 affine = vec2(0);

    // vec2 test = vec2(0);
    float test = 0;
    // for(int x = 0; x < conv_size; ++x){
    //     for(int y = 0; y < conv_size; ++y) {
    for(int i = 0; i < conv_size; ++i){
            // test += abs(vec2(x-conv_size/2,y-conv_size/2))*pxsize;
            float d = texture(disparities, center + v[i]*pxsize).x * disparity_coeff;
            // test += d;
            d -= dc;
            affine.x += d * conv1[i];
            affine.y += d * conv2[i];
            // ++idx;
    }
    //affine.x = affine.x+1;
    //temporary sign error fix:
    affine.x = 1-affine.x;
    affine.y *= -1;

    // return vec2(test, test - dc*(idx+1));
    return affine;
}

vec3 triangulate(vec2 uv, float d) {
    float h = f * tan(fovy / 2);
    float w = aspect * h;

    float z = f * b / d;
    
    float u1 = uv.x * 2 - 1;
    float x = u1 * b / -d * w;
    float v1 = uv.y * 2 - 1;
    float y = v1 * b / -d * h;
    return vec3(x, y, z);
}

void main() {
    vec2 uv = tex_coord;

    
    vec2 pxsize = vec2(1.0f/resolution.x, 1.0f/resolution.y);
    if(enable_lsq){
        affine = affine_lsq(uv, pxsize);
    } else {
        affine = affine_nb(uv, pxsize);
    }
    // affine = vec2(scale, shear);
    position = triangulate(uv,texture(disparities, uv).x * disparity_coeff);
}