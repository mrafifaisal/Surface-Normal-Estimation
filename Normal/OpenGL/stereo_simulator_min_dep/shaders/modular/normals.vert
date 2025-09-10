#version 460

uniform sampler2D normals;
uniform sampler2D positions;
uniform isampler2D mask;
uniform ivec2 resolution;
uniform ivec2 nth;
uniform bool useMask = true;

out vec3 normal;

void main() {
    int col = gl_VertexID % resolution.x;
    int row = gl_VertexID / resolution.x;

    vec2 uv = vec2((col + 0.5f) / float(resolution.x),
                   (row + 0.5f) / float(resolution.y));
    vec3 pos = texture(positions, uv).xyz;
    normal = texture(normals, uv).xyz;

    gl_Position = vec4(pos, 1);

    if (col % nth.x != 0 || row % nth.y != 0) {
        // poor mans discard
        gl_Position = vec4(10000, 10000, 10000, 1);
        normal = vec3(0, 0, 0);
    }

    if (useMask) {
        int m = texture(mask, uv).x;
        if (m == 0) {
            gl_Position = vec4(10000, 10000, 10000, 1);
            normal = vec3(0, 0, 0);
            return;
        }
    }
}