#version 460

#define PI                                                                     \
    3.1415926535897932384626433832795028841971693993751058209749445923078164062

uniform sampler2D reference;
uniform sampler2D observed;
uniform isampler2D mask;
uniform bool useMask = false;

uniform bool gradient_map = true;
uniform int gradient_steps = 4;
uniform float gradient_positions[4] = {0.0f, 1.0f / 3.0f, 2.0f / 3.0f, 1.0f};
uniform vec3 gradient_colors[4] = {
    vec3(1.0f, 1.0f, 1.0f), vec3(0.973, 0.663, 0.139), vec3(1.0f, 0.0f, 0.0f),
    vec3(0.233, 0.0f, 1.0f)};

// 0 - normal difference
// 1 - angle between (with optional flip)
uniform int error_type = 0;

in vec2 uv;
out vec3 diff;

void main() {
    if (useMask) {
        int m = texture(mask, uv).x;
        if (m == 0) {
            diff = vec3(0, 0, 0);
            return;
        }
    }

    vec3 r = texture(reference, uv).xyz;
    vec3 o = texture(observed, uv).xyz;
    vec3 nr = normalize(r);
    vec3 no = normalize(o);

    if (error_type == 0)
        diff = o - r;
    else if (error_type == 1) {
        float d = dot(nr, no);
        d = clamp(d, -1, 1);
        float a = acos(d);
        float b = PI - a;
        diff = vec3(min(a, b), 0, 0);
    } else if (error_type == 2) {
        vec3 m1 = abs(o - r);
        vec3 m2 = abs(o + r);
        float a = m1.x + m1.y + m1.z;
        float b = m2.x + m2.y + m2.z;
        diff = vec3(min(a, b), 0, 0);
    } else if (error_type == 3) {
        diff = vec3(min(length(o - r), length(o + r)), 0, 0);
    }
    if (isnan(o.x + o.y + o.z) /*|| abs(dot(nr, no)) > 1*/)
        diff.y = 1;
    if (isnan(diff.x))
        diff.z = 1;

    if (gradient_map) {
        float t = diff.x;
        if (error_type == 1) {
            t /= (PI / 2);
        }
        int i = 1;
        while (i < gradient_steps && t > gradient_positions[i]) {
            ++i;
        }
        vec3 a = gradient_colors[i - 1];
        vec3 b = gradient_colors[i];
        t -= gradient_positions[i-1];
        t /= gradient_positions[i]-gradient_positions[i-1];
        // diff = vec3(t,0,0);
        diff = mix(a,b,t);
    }

    if (isnan(o.x + o.y + o.z) /*|| abs(dot(nr, no)) > 1*/)
        diff = vec3(0,1,0);
    if (isnan(diff.x))
        diff.z = 1;
        
    // else if(dot(o,o) == 0)
    // diff.z = 1;
}