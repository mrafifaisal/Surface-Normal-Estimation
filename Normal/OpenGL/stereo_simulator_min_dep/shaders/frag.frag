#version 460

in vec3 vs_out_pos;
in vec3 vs_out_norm;
in vec3 vs_out_col;

out vec4 fs_out_col;

uniform vec3 light_dir = normalize(vec3(1, -3, -1));
// uniform vec3 cam_pos;
uniform vec3 ambient = vec3(0.05, 0.05, 0.05);

void main() {
    float diff_coeff = clamp(dot(vs_out_norm, -light_dir), 0, 1);
    fs_out_col = vec4(vs_out_col * diff_coeff + ambient, 1);
    fs_out_col = vec4(diff_coeff,diff_coeff,diff_coeff,1);
}