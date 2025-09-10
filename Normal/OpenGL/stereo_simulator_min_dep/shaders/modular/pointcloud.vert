#version 460

uniform sampler2D positions;
uniform sampler2D colors;
uniform mat4 MVP;
uniform ivec2 resolution;
uniform ivec2 nth;
uniform float size;

out vec4 color;

void main() {
    int col = gl_VertexID % resolution.x;
    int row = gl_VertexID / resolution.x;

    vec2 uv = vec2((col + 0.5f) / float(resolution.x),
                   (row + 0.5f) / float(resolution.y));
    
    vec3 pos = texture(positions, uv).xyz;
    color = texture(colors, uv);

    gl_Position = MVP * vec4(pos,1);
    gl_PointSize = size;

    if(col % nth.x != 0 || row % nth.y != 0){
        // poor man's discard
        gl_Position = vec4(10000,10000,10000,1);
    }
}