#pragma once
#include <cmath>

struct Vector2 {
	float x, y;
	Vector2(float x, float y) : x(x), y(y) {}
	Vector2() : x(0.0f), y(0.0f) {}
};

struct Vector3 {
	float x, y, z;
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
	Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
};

struct Vector4 {
	float x, y, z, w;
	Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
	Vector4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
};

struct Matrix4x4 {
	float c0x;float c1x;float c2x;float c3x;
	float c0y;float c1y;float c2y;float c3y;
	float c0z;float c1z;float c2z;float c3z;
	float c0w;float c1w;float c2w;float c3w;

	Matrix4x4(const Vector4& a, const Vector4& b, const Vector4& c, const Vector4& d) {
		c0x = a.x;
		c0y = a.y;
		c0z = a.z;
		c0w = a.w;
		c1x = b.x;
		c1y = b.y;
		c1z = b.z;
		c1w = b.w;
		c2x = c.x;
		c2y = c.y;
		c2z = c.z;
		c2w = c.w;
		c3x = d.x;
		c3y = d.y;
		c3z = d.z;
		c3w = d.w;

	}

	Matrix4x4() {
		c0x = 0.0f; c0y = 0.0f; c0z = 0.0f; c0w = 0.0f;
		c1x = 0.0f; c1y = 0.0f; c1z = 0.0f; c1w = 0.0f;
		c2x = 0.0f; c2y = 0.0f; c2z = 0.0f; c2w = 0.0f;
		c3x = 0.0f; c3y = 0.0f; c3z = 0.0f; c3w = 0.0f;
	}



	static Matrix4x4 Identity() {
		return Matrix4x4{
			{1.0f,0.0f,0.0f,0.0f},
			{0.0f,1.0f,0.0f,0.0f},
			{0.0f,0.0f,1.0f,0.0f},
			{0.0f,0.0f,0.0f,1.0f}
		};
	}
};

struct PackedMatrix {
	float c0x;float c1x;float c2x;float c3x;
	float c0y;float c1y;float c2y;float c3y;
	float c0z;float c1z;float c2z;float c3z;
	PackedMatrix(const Matrix4x4& mat) {
		c0x = mat.c0x;
		c0y = mat.c0y;
		c0z = mat.c0z;

		c1x = mat.c1x;
		c1y = mat.c1y;
		c1z = mat.c1z;

		c2x = mat.c2x;
		c2y = mat.c2y;
		c2z = mat.c2z;

		c3x = mat.c3x;
		c3y = mat.c3y;
		c3z = mat.c3z;
	}

	static PackedMatrix Identity() {
		Matrix4x4 mat = Matrix4x4::Identity();
		return PackedMatrix(mat);
	}
};




inline Vector3 operator+(Vector3 a, Vector3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }

inline Vector3 operator-(Vector3 a, Vector3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }

inline Vector3 operator*(Vector3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }

inline float dot(Vector3 a, Vector3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

inline Vector3 cross(Vector3 a, Vector3 b) {
	return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

inline Vector3 normalise(Vector3 a) { float L = sqrt(dot(a, a)); return a * (1.0f / (L > 0 ? L : 1)); }

inline Vector4 mul(const Matrix4x4& mat, Vector4 v) {
	Vector4 r{ 0.0f,0.0f,0.0f,0.0f };
	r.x = v.x * mat.c0x + v.y * mat.c1x + v.z * mat.c2x + v.w * mat.c3x;
	r.y = v.x * mat.c0y + v.y * mat.c1y + v.z * mat.c2y + v.w * mat.c3y;
	r.z = v.x * mat.c0z + v.y * mat.c1z + v.z * mat.c2z + v.w * mat.c3z;
	r.w = v.x * mat.c0w + v.y * mat.c1w + v.z * mat.c2w + v.w * mat.c3w;
	return r;
}

inline Vector3 mul(const PackedMatrix& mat, Vector4 v) {
	Vector3 r{};
	r.x = v.x * mat.c0x + v.y * mat.c1x + v.z * mat.c2x + v.w * mat.c3x;
	r.y = v.x * mat.c0y + v.y * mat.c1y + v.z * mat.c2y + v.w * mat.c3y;
	r.z = v.x * mat.c0z + v.y * mat.c1z + v.z * mat.c2z + v.w * mat.c3z;
	return r;
}

inline Matrix4x4 mul(const Matrix4x4& M1, const Matrix4x4& M2) {
	Vector4 v1 = { M2.c0x, M2.c0y, M2.c0z, M2.c0w };
	Vector4 v2 = { M2.c1x, M2.c1y, M2.c1z, M2.c1w };
	Vector4 v3 = { M2.c2x, M2.c2y, M2.c2z, M2.c2w };
	Vector4 v4 = { M2.c3x, M2.c3y, M2.c3z, M2.c3w };
	Matrix4x4 OutMatrix{
		mul(M1, v1),
		mul(M1, v2),
		mul(M1, v3),
		mul(M1, v4)
	};
	return OutMatrix;
}



inline Matrix4x4 translate(const Vector3 v) {
	Matrix4x4 matrixOut = Matrix4x4::Identity();
	matrixOut.c3x = v.x;
	matrixOut.c3y = v.y;
	matrixOut.c3z = v.z;
	return matrixOut;
}

inline Matrix4x4 scale(Vector3 s) {
	Matrix4x4 matrixOut = Matrix4x4::Identity();
	matrixOut.c0x = s.x;
	matrixOut.c1y = s.y;
	matrixOut.c2z = s.z;
	return matrixOut;
}

inline Matrix4x4 rotateX(float angle) {
	Matrix4x4 matrixOut = Matrix4x4::Identity();

	matrixOut.c1y = cos(angle);
	matrixOut.c1z = sin(angle);
	matrixOut.c2y = -sin(angle);
	matrixOut.c2z = cos(angle);
}

inline Matrix4x4 rotateY(float angle) {
	Matrix4x4 matrixOut = Matrix4x4::Identity();

	matrixOut.c0x = cos(angle);
	matrixOut.c0z = -sin(angle);
	matrixOut.c2x = sin(angle);
	matrixOut.c2z = cos(angle);
}

inline Matrix4x4 rotateZ(float angle) {
	Matrix4x4 matrixOut = Matrix4x4::Identity();

	matrixOut.c0x = cos(angle);
	matrixOut.c0y = sin(angle);
	matrixOut.c1x = -sin(angle);
	matrixOut.c1y = cos(angle);
}

inline Matrix4x4 LookAt(Vector3 EyePos, Vector3 Center, Vector3 Up) {
	Vector3 forward = normalise(Center - EyePos);
	Vector3 right = normalise(cross(forward, Up));
	Vector3 up = cross(right, forward);

	Matrix4x4 Matrix = Matrix4x4::Identity();
	Matrix.c0x = right.x,    Matrix.c1x = right.y,    Matrix.c2x = right.z,    Matrix.c3x = -dot(right, EyePos);
	Matrix.c0y = up.x,       Matrix.c1y = up.y,       Matrix.c2y = up.z,       Matrix.c3y = -dot(up, EyePos);
	Matrix.c0z = -forward.x, Matrix.c1z = -forward.y, Matrix.c2z = -forward.z, Matrix.c3z = dot(forward, EyePos);
	return Matrix;
}

inline Matrix4x4 Perspective(float FOV_Y, float aspect, float zNear, float zFar) {
	float focalLen = 1.0f / tan(FOV_Y / 2.0f);

	Matrix4x4 M{};
	M.c0x = focalLen / aspect; M.c1x = 0.0f; M.c2x = 0.0f;                            M.c3x = 0.0f;
	M.c0y = 0.0f;      M.c1y = focalLen;     M.c2y = 0.0f;                            M.c3y = 0.0f;
	M.c0z = 0.0f;      M.c1z = 0.0f;         M.c2z = (zFar + zNear) / (zNear - zFar); M.c3z = (2.0f * zFar * zNear) / (zNear - zFar);
	M.c0w = 0.0f;      M.c1w = 0.0f;         M.c2w = -1.0f;                           M.c3w = 0.0f;
	return M;

}