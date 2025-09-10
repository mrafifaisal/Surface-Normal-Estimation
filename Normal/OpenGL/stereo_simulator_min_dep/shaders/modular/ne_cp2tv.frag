#version 460

uniform sampler2D depth;
uniform sampler2D depthGradient;
uniform float k11;
uniform float k22;
uniform ivec2 resolution;
uniform vec2 principle_point;

in vec2 uv;
out vec3 normal;

vec3 triangulate(vec2 uv, float z) {
    // float z = texture(depth, uv).z;

    vec2 uvpx = uv * resolution - principle_point;

    float x = uvpx.x / k11 * z;
    float y = uvpx.y / k22 * z;

    x *= -1; // change to x right, y up, z forward

    return vec3(x, y, z);
}

void main() {
    vec2 pos = ((uv+1)/2);
    // pos.y = 1-pos.y;
    pos = pos*resolution-principle_point;
    // pos = ((uv+1)/2);
    // pos.x *= 0;
    
    vec2 G = texture(depthGradient, uv).xy;
    float Z = texture(depth, uv).z;

    vec2 pxsize = 1.0f/resolution;
    vec2 G2;
    G2.x = (texture(depth,uv+vec2(pxsize.x,0)).z - texture(depth,uv).z);
    G2.y = (texture(depth,uv+vec2(0,pxsize.y)).z - texture(depth,uv).z);

    // G.y *= -1;

    // G=G2;

    // normal.x = G.x*k11;
    // normal.y = G.y*k22;

    // scaling of pos is probably wrong, but this is the code from the paper
    // normal.z = -(Z+pos.y*G.y+pos.x*G.x); 
    // normal.z = pos.y*G.y+pos.x*G.x;

    // based on cp2tv paper:
    // vec3 vx = vec3(
    //     Z/k11+(pos.x-principle_point.x)/k11*G.x,
    //     (pos.y-principle_point.y)/k22*G.y,
    //     G.x
    // );
    // vec3 vy = vec3(
    //     (pos.x-principle_point.x)/k11*G.y,
    //     Z/k22+(pos.y-principle_point.y)/k22*G.y,
    //     G.y
    // );
    // normal = cross(vx,vy);

    float Zc = texture(depth, uv).z;
    float Zr = Zc + G.x;
    float Zt = Zc + G.y;
    // float Zr = texture(depth, uv+vec2(1.0f/resolution.x,0)).z;
    // float Zt = texture(depth, uv+vec2(0,1.0f/resolution.y)).z;

    // vec3 pc = vec3(Zc/k11*(pos.x-principle_point.x), Zc/k22*(pos.y-principle_point.y), Zc);
    // vec3 pr = vec3(Zr/k11*(pos.x-principle_point.x+1), Zr/k22*(pos.y-principle_point.y), Zr);
    // vec3 pt = vec3(Zt/k11*(pos.x-principle_point.x), Zt/k22*(pos.y-principle_point.y+1), Zt);

    vec3 pc = triangulate(uv, Zc);
    vec3 pr = triangulate(uv+vec2(1.0f/resolution.x,0), Zr);
    vec3 pt = triangulate(uv+vec2(0,1.0f/resolution.y), Zt);
    normal = cross(pr-pc,pt-pc);
    normal = normalize(normal);

    

    // vec3 P = texture(depth, uv).xyz;
    // vec3 p2 = P + vec3(1.0f/k11*texture(depth,uv+vec2(pxsize.x,0)).z,0,1*G2.x);
    // vec3 p3 = P + vec3(0,1.0f/k22*texture(depth,uv+vec2(0,pxsize.y)).z,1*G2.y);
    // normal = normalize(cross(p2-P,p3-P));
}