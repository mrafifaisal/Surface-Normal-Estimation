#version 460

in vec3 vs_out_pos;
in vec3 vs_out_view_pos;
in vec3 vs_out_norm;
in vec3 vs_out_world_norm;
in vec3 vs_out_col;
in vec4 vs_out_ndc;

layout(location = 0) out vec4 out_col;
layout(location = 1) out float out_disp;
layout(location = 2) out vec3 out_pos;
layout(location = 3) out vec3 out_norm;
layout(location = 4) out ivec2 out_mask;

// uniform float f;
// uniform float fovy;
// uniform float aspect;
uniform mat4 VP;
uniform mat4 rightVP;

uniform int resx;
uniform vec3 color;
uniform bool useColor = false;

uniform vec3 light_dir = normalize(vec3(1, -3, -1));
uniform vec3 cam_pos;
uniform vec3 ambient = vec3(0.15, 0.15, 0.15);

void main() {
    out_mask = ivec2(127,0);
    out_norm = vs_out_norm;
    out_pos = vs_out_view_pos;
    out_pos *= vec3(-1, 1, -1);
    out_norm *= vec3(-1, 1, -1);
    out_norm = normalize(out_norm);

    vec4 rightPos = rightVP * vec4(vs_out_pos, 1);
    rightPos /= rightPos.w;

    vec4 leftPos = VP * vec4(vs_out_pos, 1);
    leftPos /= leftPos.w;

    // out_disp = rightPos.x - leftPos.x;
    out_disp = leftPos.x - rightPos.x;
    out_disp = out_disp / 2 * resx;

    // float h = f * tan(fovy / 2);
    // float w = aspect * h;
    // out_disp = out_disp * w;

    vec3 base_color = vs_out_col;
    if (useColor)
        base_color = color;

    vec3 wnorm = normalize(vs_out_world_norm);
    float diff_coeff = clamp(dot(wnorm, -light_dir), 0, 1);
    float spec_coeff = pow(
        clamp(dot(reflect(light_dir, wnorm), normalize(cam_pos - vs_out_pos)),
              0, 1),
        35);
    out_col = vec4(base_color * diff_coeff + spec_coeff + ambient, 1);

    // out_disp = (vs_out_ndc/vs_out_ndc.w).x - rightPos.x ;
    // out_disp = (vs_out_ndc/vs_out_ndc.w).z;
}