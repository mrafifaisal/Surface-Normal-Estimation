pkg load symbolic
syms n f l r t b baseline
s0 = sym(0);
s1 = sym(1);

%P = [sym(2)*n/(r-l), sym(0), (r+l)/(r-l), sym(0); sym(0), sym(2)*n/(t-b), (t+b)/(t-b), sym(0); sym(0), sym(0), -(f+n)/(f-n), -(sym(2)*f*n)/(f-n); sym(0), sym(0), sym(-1), sym(0)]

syms fovy aspect zNear zFar

tanHalfFovy = tan(fovy/2);

%Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
%Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
%Result[2][2] = - (zFar + zNear) / (zFar - zNear);
%Result[2][3] = - static_cast<T>(1);
%Result[3][2] = - (static_cast<T>(2) * zFar * zNear) / (zFar - zNear);

P = sym(zeros(4));
P(1,1) = s1 / (aspect * tanHalfFovy);
P(2,2) = s1 / tanHalfFovy;
P(3,3) = - (zFar + zNear) / (zFar - zNear);
P(4,3) = - s1;
P(3,4) = - (sym(2) * zFar * zNear) / (zFar - zNear);

T = [sym(1), s0, s0, -baseline; s0, sym(1), s0, s0; s0,s0,sym(1),s0;s0,s0,s0,sym(1)];
%T = inv(T)

syms x y z
P1 = P * [x;y;z;s1]; P1 = P1 / P1(4)
P2 = P * T * [x;y;z;s1]; P2 = P2 / P2(4)

P1x = P1(1)
P1y = P1(2)
GP1x = gradient(P1x, [x,y,z])
GP1y = gradient(P1y, [x,y,z])
P2x = P2(1)
P2y = P2(2)
GP2x = gradient(P2x, [x,y,z])
GP2y = gradient(P2y, [x,y,z])

w1=simplify(cross(GP1y,GP2x))
w2=simplify(cross(GP2x,GP1x))
 w3 = simplify(cross(GP1y,GP2y))
 w4 = simplify(cross(GP2y,GP1x))
 w5 = simplify(cross(GP1x,GP1y))

syms n1 n2 n3
n = [n1;n2;n3]

A = [n.'*w1, n.'*w2; n.'*w3, n.'*w4]
tmp = n.'*w5
A = simplify(A / tmp(1))

syms a_1 a_2
%v1 = [baseline-x;-y;-z]
%v2 = [x;y;z]
%v3 = [s0;baseline*(-b+t);s0]
%v4 = [(r-l)*x;(r-l)*y;(r-l)*z]

v1 = [baseline - x; -y; -z];
v2 = [x;y;z];
v3 = [s0;baseline;s0];
v4 = aspect*[x;y;z];

simplify(cross(a_1*v2-v1,a_2*v4-v3))
