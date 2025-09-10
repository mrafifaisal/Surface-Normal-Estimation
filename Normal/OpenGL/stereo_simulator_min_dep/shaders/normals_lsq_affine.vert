#version 460

uniform int nth;
uniform float b;
uniform float f;
uniform float fovy;
uniform float aspect;
uniform sampler2D disparities;
uniform sampler2D positionsTruth;
uniform sampler2D normalsTruth;
uniform sampler2D noise;
uniform sampler2D affineTexture;
uniform sampler2D positionTexture;
uniform bool useNoise;
uniform ivec2 resolution;

uniform int window_size = 1; // 2*1+1 = 3  =>  3x3 window

uniform vec2 custom_k;

out vec3 normal;

vec3 calc_normal(vec2 a, vec3 p, float baseline, float aspect) {
    float ku = 1;
    float kv = 1;

    float x = baseline * ku * p.z * (a.x-1);
    float y = a.y * baseline * kv * p.z;
    float z = baseline * (-a.x*ku*p.x-a.y*kv*p.y+baseline*ku+ku*p.x);
    return vec3(x,y,z);
}

vec2 affine_lsq(ivec2 center) {
    float step_x = 1.0f/resolution.x;
    float step_y = 1.0f/resolution.y;

    // TODO
}

void main() {
    int col = gl_VertexID % resolution.x;
    int row = gl_VertexID / resolution.x;

    vec2 uv = vec2((col + 0.5f) / float(resolution.x),
                   (row + 0.5f) / float(resolution.y));

    vec3 center = texture(positionTexture, uv).xyz;

    float dc = texture(disparities, uv).x;
    float dr = texture(disparities, uv + vec2(1.0f / resolution.x, 0)).x;
    float dt = texture(disparities, uv + vec2(0, 1.0f / resolution.y)).x;

    vec2 aff = texture(affineTexture, uv).xy;

    vec3 n4 = calc_normal(aff, center, b, aspect); // correct
    normal = n4;

    gl_Position = vec4(center, 1);

    if(col%nth != 0 || row%nth != 0){
        // poor man's discard: clipping (not geometry shder...)
        gl_Position = vec4(100000,100000,100000,1);
        normal = vec3(0,0,0);
    }
}