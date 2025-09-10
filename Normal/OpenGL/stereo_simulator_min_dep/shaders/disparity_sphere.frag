#version 460

in vec2 uv;

layout(location = 0) out vec4 out_col;
layout(location = 1) out float out_disp;
layout(location = 2) out vec3 out_pos;
layout(location = 3) out vec3 out_norm;
layout(location = 4) out ivec2 out_mask;

uniform vec3 cam_pos;
uniform mat4 invVP;
uniform int resx;
uniform mat4 VP;
uniform mat4 rightVP;
uniform mat4 V;

uniform vec3 center;
uniform float radius;

uniform vec3 color = vec3(1,0,0);
uniform vec3 light_dir = normalize(vec3(1, -3, -1));
uniform vec3 ambient = vec3(0.15, 0.15, 0.15);

void main() {
    // https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection

    vec4 ndc = vec4(uv.x * 2 - 1, uv.y * 2 - 1, 1, 1);
    vec4 ray4 = invVP * ndc;
    ray4 /= ray4.w;

    vec3 ray = normalize(ray4.xyz);
    vec3 from_sphere = cam_pos - center; // o-c
    float D = pow(dot(ray, from_sphere), 2) -
              (dot(from_sphere, from_sphere) - radius * radius);
    // out_pos = ray;
    if (D < 0)
        discard;
    float d = -dot(ray, from_sphere) - sqrt(D);
    // d *= -1;
    // out_norm = vec3(D,d,0);
    // out_col = vec4(color,1);
    // return;

    out_mask = ivec2(127,0);

    // position and normal
    vec3 wpos = cam_pos + d * ray;
    out_pos = (V * vec4(wpos, 1)).xyz;
    vec3 wnorm = normalize(wpos - center);
    out_norm = (V * vec4(wnorm, 0)).xyz;

    out_pos *= vec3(-1, 1, -1);
    out_norm *= vec3(-1, 1, -1);
    vec4 proj = VP * vec4(wpos,1);
    gl_FragDepth = proj.z/proj.w*0.5+0.5;

    // disparity
    vec4 rightPos = rightVP * vec4(wpos, 1);
    rightPos /= rightPos.w;

    vec4 leftPos = VP * vec4(wpos, 1);
    leftPos /= leftPos.w;

    out_disp = leftPos.x - rightPos.x;
    out_disp = out_disp / 2 * resx;

    // color
    vec3 base_color = color;

    float diff_coeff = clamp(dot(wnorm, -light_dir), 0, 1);
    float spec_coeff =
        pow(clamp(dot(reflect(light_dir, wnorm), normalize(cam_pos - wpos)),
                  0, 1),
            35);
    out_col = vec4(base_color * diff_coeff + spec_coeff + ambient, 1);
}