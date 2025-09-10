#version 460
// source: https://gist.github.com/alexsr/5065f0189a7af13b2f3bc43d22aff62f
// This is a GLSL implementation of
// "Computing the Singular Value Decomposition of 3 x 3 matrices with
// minimal branching and elementary floating point operations"
// by Aleka McAdams et.al.
// http://pages.cs.wisc.edu/~sifakis/papers/SVD_TR1690.pdf

// This should also work on the CPU using glm
// Then you probably should use glm::quat instead of vec4
// and mat3_cast to convert to mat3.

// GAMMA = 3 + sqrt(8)
// C_STAR = cos(pi/8)
// S_STAR = sin(pi/8)

#define GAMMA 5.8284271247
#define C_STAR 0.9238795325
#define S_STAR 0.3826834323
#define SVD_EPS 0.0000001

#define PI 3.14159274101257324

vec2 approx_givens_quat(float s_pp, float s_pq, float s_qq) {
    float c_h = 2 * (s_pp - s_qq);
    float s_h2 = s_pq * s_pq;
    float c_h2 = c_h * c_h;
    if (GAMMA * s_h2 < c_h2) {
        float omega = 1.0f / sqrt(s_h2 + c_h2);
        return vec2(omega * c_h, omega * s_pq);
    }
    return vec2(C_STAR, S_STAR);
}

// the quaternion is stored in vec4 like so:
// (c, s * vec3) meaning that .x = c
mat3 quat_to_mat3(vec4 quat) {
    float qx2 = quat.y * quat.y;
    float qy2 = quat.z * quat.z;
    float qz2 = quat.w * quat.w;
    float qwqx = quat.x * quat.y;
    float qwqy = quat.x * quat.z;
    float qwqz = quat.x * quat.w;
    float qxqy = quat.y * quat.z;
    float qxqz = quat.y * quat.w;
    float qyqz = quat.z * quat.w;

    return mat3(
        1.0f - 2.0f * (qy2 + qz2), 2.0f * (qxqy + qwqz), 2.0f * (qxqz - qwqy),
        2.0f * (qxqy - qwqz), 1.0f - 2.0f * (qx2 + qz2), 2.0f * (qyqz + qwqx),
        2.0f * (qxqz + qwqy), 2.0f * (qyqz - qwqx), 1.0f - 2.0f * (qx2 + qy2));
}

// uniform int trick_uniform = 5;
mat3 symmetric_eigenanalysis(mat3 A) {
    mat3 S = transpose(A) * A;
    // jacobi iteration
    mat3 q = mat3(1.0f);
    for (int i = 0; i < 20; i++) {
        vec2 ch_sh = approx_givens_quat(S[0].x, S[0].y, S[1].y);
        vec4 ch_sh_quat = vec4(ch_sh.x, 0, 0, ch_sh.y);
        mat3 q_mat = quat_to_mat3(ch_sh_quat);
        S = transpose(q_mat) * S * q_mat;
        q = q * q_mat;

        ch_sh = approx_givens_quat(S[0].x, S[0].z, S[2].z);
        ch_sh_quat = vec4(ch_sh.x, 0, -ch_sh.y, 0);
        q_mat = quat_to_mat3(ch_sh_quat);
        S = transpose(q_mat) * S * q_mat;
        q = q * q_mat;

        ch_sh = approx_givens_quat(S[1].y, S[1].z, S[2].z);
        ch_sh_quat = vec4(ch_sh.x, ch_sh.y, 0, 0);
        q_mat = quat_to_mat3(ch_sh_quat);
        S = transpose(q_mat) * S * q_mat;
        q = q * q_mat;
    }
    return q;
    // return mat3(1);
}

vec3 svd_smallest_eigvec(mat3 A) { // based on svd
    mat3 V = symmetric_eigenanalysis(A);
    // mat3 V = mat3(1);

    mat3 B = A * V;
    // mat3 B = A;
    // sort singular values
    float rho0 = dot(B[0], B[0]);
    float rho1 = dot(B[1], B[1]);
    float rho2 = dot(B[2], B[2]);
    // float rho0 = 1;
    // float rho1 = 2;
    // float rho2 = 3;
    int idx = 0;
    if (rho1 < rho0) {
        idx = 1;
        rho0 = rho1;
    }
    if (rho2 < rho0)
        idx = 2;
    // return B[0];
    return V[idx];
    // return vec3(0);
}

vec3 pca_min(mat3 cov) {
    float cov_xx = cov[0][0], cov_xy = cov[1][0];
    float cov_yy = cov[1][1], cov_yz = cov[2][1];
    float cov_xz = cov[2][0], cov_zz = cov[2][2];

    // viktor-vad: Very strange, but the pca.hpp and pca.inc disappeared from
    // glm/gtx. Did not find any explanation for this. Instead of some header
    // file copy-hacking, I implemented a 3x3 verion of eigen decomposition. It
    // was not intended, but most likely it is faster than the original glm pca,
    // since that is a general method with Housholder and QR.
    // https://dl.acm.org/doi/epdf/10.1145/355578.366316
    // https://en.wikipedia.org/wiki/Eigenvalue_algorithm#2%C3%972_matrices
    float p1 = cov_xy * cov_xy + cov_xz * cov_xz + cov_yz * cov_yz;
    float trC = cov_xx + cov_yy + cov_zz;
    float eig1 = 0.0f, eig2 = 0.0f, eig3 = 0.0f;

    // normal case
    if (p1 > 1e-15f) {
        float q = trC / 3.0f;
        float p2 = (cov_xx - q) * (cov_xx - q) + (cov_yy - q) * (cov_yy - q) +
                   (cov_zz - q) * (cov_zz - q) + 2.0f * p1;
        float p = sqrt(p2 / 6.0f);

        float cov_xx_q = cov_xx - q;
        float cov_yy_q = cov_yy - q;
        float cov_zz_q = cov_zz - q;

        float r = clamp(
            (cov_xx_q * cov_yy_q * cov_zz_q + 2.0f * cov_xy * cov_yz * cov_xz -
             cov_xx_q * cov_yz * cov_yz - cov_yy_q * cov_xz * cov_xz -
             cov_zz_q * cov_xy * cov_xy) /
                (2.0f * p * p * p),
            -1.0f, 1.0f);

        float phi = acos(r) / 3.0f;

        eig1 = q + 2.0f * p * cos(phi);
        eig2 = q + 2.0f * p * cos(phi + (2.0f * PI / 3.0f));
        eig3 = trC - eig1 - eig2;
    } else // covariance matrix is numericaly diagonal. We assume eigen
           // values are the diagonal values.
    {
        eig1 = max(max(cov_xx, cov_yy), cov_zz);
        eig3 = min(min(cov_xx, cov_yy), cov_zz);
        eig2 = trC - eig1 - eig2;
    }

    // We only need the eigen vector corresponding to the smallest
    // eigenvalue
    float minEig = min(min(eig1, eig2), eig3);

    if (eig3 == minEig) {
        return vec3(
            cov_xz * ((cov_xx - eig1) + (cov_zz - eig2)) + cov_xy * cov_yz,
            cov_yz * ((cov_yy - eig1) + (cov_zz - eig2)) + cov_xy * cov_xz,
            cov_yz * cov_yz + cov_xz * cov_xz +
                (cov_zz - eig1) * (cov_zz - eig2));
    } else if (eig2 == minEig) {
        return vec3(
            cov_xy * ((cov_xx - eig1) + (cov_yy - eig3)) + cov_xz * cov_yz,
            cov_yz * cov_yz + cov_xy * cov_xy +
                (cov_yy - eig1) * (cov_yy - eig3),
            cov_yz * ((cov_yy - eig3) + (cov_zz - eig1)) + cov_xy * cov_xz);
    } else // if ( eig1 == minEig ) most unlikly case
    {
        return vec3(
            cov_xy * cov_xy + cov_xz * cov_xz +
                (cov_xx - eig2) * (cov_xx - eig3),
            cov_xy * ((cov_xx - eig3) + (cov_yy - eig2)) + cov_xz * cov_yz,
            cov_xz * ((cov_xx - eig3) + (cov_zz - eig2)) + cov_xy * cov_yz);
    }
}

// OWN code

uniform sampler2D positions;
uniform ivec2 resolution;
uniform int square_size = 3;
uniform int version = 1;

in vec2 uv;
out vec4 normal;

void main() {

    // STEP 1: compute Cx covariance matrix
    // compute means and substract them
    vec3 mean = vec3(0);
    vec2 pxsize = vec2(1.0f / resolution.x, 1.0f / resolution.y);
    for (int i = -square_size / 2; i <= square_size / 2; ++i) {
        for (int j = -square_size / 2; j <= square_size / 2; ++j) {
            mean += texture(positions, uv + vec2(i, j) * pxsize).xyz;
        }
    }
    mean /= square_size * square_size;

    // compute dot products in Cx
    mat3 Cx = mat3(0);
    for (int i = -square_size / 2; i <= square_size / 2; ++i) {
        for (int j = -square_size / 2; j <= square_size / 2; ++j) {
            vec3 p = texture(positions, uv + vec2(i, j) * pxsize).xyz - mean;
            Cx[0][0] += p.x * p.x; // first column, first row
            Cx[0][1] += p.y * p.x; // first column, second row
            Cx[0][2] += p.z * p.x; // first column, third row

            // Cx is symmetric, Cx[1][0] = Cx[0][1]
            Cx[1][1] += p.y * p.y; // second column, second row
            Cx[1][2] += p.z * p.y; // second column, third row

            // Cx is symmetric, Cx[2][0] = Cx[0][2] and Cx[2][1] = Cx[1][2]
            Cx[2][2] += p.z * p.z; // third column, third row
        }
    }
    // resolve symmetry
    Cx[1][0] = Cx[0][1];
    Cx[2][0] = Cx[0][2];
    Cx[2][1] = Cx[1][2];

    // divide by n-1
    Cx /= (square_size * square_size - 1);

    vec3 n;
    if (version == 0)
        n = svd_smallest_eigvec(Cx);
    else if (version == 1)
        n = pca_min(Cx);
    normal = vec4(n, 1);
}