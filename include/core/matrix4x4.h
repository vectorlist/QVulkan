
#ifndef MATRIX4X4_H
#define MATRIX4X4_H

#include <vml.h>
#include <assert.h>
#include <vec3f.h>
#include <iostream>
#include <iomanip>

enum class AXIS : unsigned int{
	X = 0, Y, Z
};

class Matrix4x4
{
public:
	Matrix4x4();
	Matrix4x4(const Matrix4x4 &other);
	Matrix4x4(float other[16]);
	Matrix4x4(float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33);
	~Matrix4x4(){};

	static const Matrix4x4 zero;

	/*OPERATOR*/

	Matrix4x4 operator*(const Matrix4x4 &other) const;
	Matrix4x4& operator*=(const Matrix4x4 &other);
	Matrix4x4 operator*(float f) const;
	Matrix4x4& operator*=(float f);

	const float* operator[](int i) const;
	float* operator[](int i);

	/*FOR GENERIC NORMAL*/
	vec3f normal(const vec3f &n);

	/*FUNCTION*/
	void setToIdentity();
	Matrix4x4& transpose();
	Matrix4x4 transposed() const;

	/*TRANSFORM*/
	void translate(vec3f t);
	void rotate(AXIS axis, float angle);
	void scale(vec3f s);


	float m[4][4];

	Matrix4x4 rotatedX(float angle) const;
	Matrix4x4 rotatedY(float angle) const;
	Matrix4x4 rotatedZ(float angle) const;

	Matrix4x4 inverted() const;
	static Matrix4x4 vulkandClip();

	inline float* data() { return *m; }
	inline const float* constData() const { return *m; }

	friend std::ostream& operator<<(std::ostream &o, const Matrix4x4 &mat);
};

vec3f operator*(const Matrix4x4 &m, const vec3f &v);

inline const float* Matrix4x4::operator[](int i) const {
	assert(i >= 0 && i < 4);
	return m[i];
}

inline float* Matrix4x4::operator[](int i) {
	assert(i >= 0 && i < 4);
	return m[i];
}

inline std::ostream& operator<<(std::ostream &o, const Matrix4x4 &mat)
{
#define FM std::setw(12)
	//row major order
	o << "Matrix4x4[" << "\n" <<
		FM << mat[0][0] << FM << mat[0][1] << FM << mat[0][2] << FM << mat[0][3] << "\n" <<
		FM << mat[1][0] << FM << mat[1][1] << FM << mat[1][2] << FM << mat[1][3] << "\n" <<
		FM << mat[2][0] << FM << mat[2][1] << FM << mat[2][2] << FM << mat[2][3] << "\n" <<
		FM << mat[3][0] << FM << mat[3][1] << FM << mat[3][2] << FM << mat[3][3] << " ]" << "\n";
#undef FM
	return o;
}


namespace vml
{
	inline Matrix4x4 perspectiveRH(float fovY, float aspect, float znear, float zfar)
	{
		float const tanHalfFov = tan(RADIANS * fovY * 0.5f);

		Matrix4x4 M;
		M[0][0] = 1.f / (aspect* tanHalfFov);
		M[1][1] = 1.f / tanHalfFov;
		M[3][2] = -1.f;

#if VML_DEPTH_CLIP_SPACE == VML_DEPTH_ZERO_TO_ONE

		M[2][2] = zfar / (znear - zfar);
		M[2][3] = -(zfar * znear) / (zfar - znear);
#else
		M[2][2] = -(zfar + znear) / (zfar - znear);
		M[2][3] = -(2.f * zfar * znear) / (zfar - znear);
#endif
		return M;
	}

	inline Matrix4x4 perspectiveLH(float fovY, float aspect, float znear, float zfar)
	{
		float const tanHalfFov = tan(RADIANS * fovY * 0.5f);

		Matrix4x4 M;
		M[0][0] = 1.f / (aspect* tanHalfFov);
		M[1][1] = 1.f / tanHalfFov;
		M[3][2] = 1.f;

#if VML_DEPTH_CLIP_SPACE == VML_DEPTH_ZERO_TO_ONE

		M[2][2] = zfar / (znear - zfar);
		M[2][3] = -(zfar * znear) / (zfar - znear);
#else
		M[2][2] = (zfar + znear) / (zfar - znear);
		M[2][3] = -(2.f * zfar * znear) / (zfar - znear);
#endif
		return M;
	}

	inline Matrix4x4 perspective(float fovY, float aspect, float znear, float zfar)
	{
#	if VML_COORDINATE_SYSTEM == VML_LEFT_HAND
		if(VML_VULKAN_CLIP)
			return Matrix4x4::vulkandClip() * perspectiveLH(fovY, aspect, znear, zfar);
		return perspectiveLH(fovY, aspect, znear, zfar);
#	else
		if (VML_VULKAN_CLIP)
			return Matrix4x4::vulkandClip() * perspectiveRH(fovY, aspect, znear, zfar);
		return perspectiveRH(fovY, aspect, znear, zfar);	//row for ray tracing(d12)
#	endif 
		
	}

	inline Matrix4x4 lookAtRH(const vec3f &eye,const vec3f& center,const vec3f& upvector)
	{
		vec3f const forward = (center - eye).normalized();
		vec3f const right	= vec3f::cross(forward, upvector).normalized();
		vec3f const up		= vec3f::cross(right, forward);

		float x = -vec3f::dot(  right, eye);
		float y = -vec3f::dot(     up, eye);
		float z =  vec3f::dot(forward, eye);

		Matrix4x4 view = {
		       right.x,     right.y,	 right.z,		x,
			      up.x,        up.y,        up.z,		y,
			-forward.x,  -forward.y,  -forward.z,		z,
				 0.0f,      0.0f,       0.0f,    1.0f};
		return view;
	}

	inline Matrix4x4 lookAtLH(const vec3f &eye, const vec3f& center, const vec3f& upvector)
	{
		vec3f const forward = (center - eye).normalized();
		vec3f const right	= vec3f::cross(upvector, forward).normalized();
		vec3f const up		= vec3f::cross(forward, right);

		float x = -vec3f::dot(	 right, eye);
		float y = -vec3f::dot(	    up, eye);
		float z = -vec3f::dot( forward, eye);

		Matrix4x4 view = {
			   right.x,     right.y,	 right.z,		x,
			      up.x,        up.y,        up.z,		y,
			-forward.x,  -forward.y,  -forward.z,		z,
			      0.0f,        0.0f,        0.0f,     1.0f };
		return view;
	}

	inline Matrix4x4 lookAt(const vec3f &eye, const vec3f &center, const vec3f &up)
	{
#		if VML_COORDINATE_SYSTEM == VML_LEFT_HAND
		return lookAtLH(eye, center, up);
#else
		return lookAtRH(eye, center, up);
#endif
	}

	//for vulkan
	inline Matrix4x4 lookAtVK(const vec3f &eye, const vec3f &center, const vec3f &up)
	{
		vec3f forward = (eye - center).normalized();
		vec3f right = vec3f::cross(up, forward).normalized();
		vec3f upvector = vec3f::cross(forward, right).normalized();

		float x = -vec3f::dot(eye, right);
		float y = -vec3f::dot(eye, upvector);
		float z = -vec3f::dot(eye, forward);

		Matrix4x4 view = {
			right.x,upvector.x,forward.x,0.0f,
			right.y,upvector.y,forward.y,0.0f,
			right.z,upvector.z,forward.z,0.0f,
			x,y,z,1.0f
		};
		return view;
	}

	inline Matrix4x4 perspectiveVK(float fovy, float aspect, float znear, float zfar)
	{
		Matrix4x4 proj = perspective(fovy, aspect, znear, zfar);
		Matrix4x4 clip = {
			1.0f,	0.0f,	0.0f,	0.0f,
			0.0f,  -1.0f,	0.0f,	0.0f,
			0.0f,	0.0f,	0.5f,	0.5f,
			0.0f,	0.0f,	0.0f,	1.0f
		};
		return clip * proj;
	}


}

#endif // MATRIX4X4_H
