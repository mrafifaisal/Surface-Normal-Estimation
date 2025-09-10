#version 460

uniform sampler2D disparities;
uniform float disparity_coeff = 1.0f;
uniform ivec2 resolution;
uniform mat4 VP;
uniform float b;
uniform float f;
uniform float fovy;
uniform float aspect;
uniform float pixel_density;

uniform bool loaded = false;

uniform mat4 VPinv;

out vec2 tex_coord;

void main() {
    int col = gl_VertexID % resolution.x;
    int row = gl_VertexID / resolution.x;

    float h = f*tan(fovy/2);
    float w = aspect * h;

    vec2 uv = vec2((col+0.5f) / float(resolution.x), (row+0.5f)  / float(resolution.y));
    tex_coord = uv;

    float d = texture(disparities, uv).x * disparity_coeff;
    // if(loaded){
    //     d = d / pixel_density;
    //     d *= -1;
    //     // d *= 1000;
    // }
    float z = f*b/d;

    

    float nc = 0.1; float fc = 10000;
    // z = 2*fc*nc/(fc-nc)/(z+(fc+nc)/fc-nc);

    float u1 = uv.x*2-1;
    float x = u1*b/-d*w;
    // float x = u1*z/f;
     //x = b*(2*u1+d)/(2*-d);
    float v1 = uv.y*2-1;
    float y = v1*b/-d*h;
    // float y = v1*z/f;

    vec4 wpos = vec4(x, -y, z*9.9+0.1, 1.0f);
    wpos = vec4(x, y, z, 1.0f);
    //  wpos = vec4(uv, z, 1.0f);
    gl_Position = VP * wpos;
    gl_PointSize = 5.0f;
}