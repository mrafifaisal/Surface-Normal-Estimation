#version 460

uniform float b;
uniform float f;
uniform float fovy;
uniform float aspect;
uniform sampler2D disparities;
uniform sampler2D normalsTruth;
uniform ivec2 resolution;

out vec3 normal;

vec3 triangulate(vec2 uv) {
    float d = texture(disparities, uv).x;
    float z = f*b/d;
    float h = f*tan(fovy/2);
    float w = aspect * h;
    float u1 = uv.x*2-1;
    float x = u1*b/-d*w;
    float v1 = uv.y*2-1;
    float y = v1*b/-d*h;
    return vec3(x,y,z);
}

void main() {
    int col = gl_VertexID % resolution.x;
    int row = gl_VertexID / resolution.x;

    vec2 uv = vec2((col+0.5f) / float(resolution.x), (row+0.5f)  / float(resolution.y));
    vec3 center = triangulate(uv);
    vec3 right = triangulate(uv + vec2(1.0f/resolution.x, 0));
    vec3 up = triangulate(uv + vec2(0, 1.0f/resolution.y));

    normal = cross(right-center, up-center);
    
    // normal = texture(normalsTruth, uv).xyz;
    gl_Position = vec4(center,1);
}