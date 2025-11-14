#include <cstdio>

#include "Maths.hpp"

#include <assert.h>
#include <corecrt_math_defines.h>

namespace Maths
{
#pragma region IVec2

	inline IVec2::IVec2(const Vec2 in) : x((s32)in.x), y((s32)in.y) {}

	inline s32 IVec2::Dot() const
	{
		return x * x + y * y;
	}

	inline s32 IVec2::Dot(IVec2 in) const
	{
		return x * in.x + y * in.y;
	}

	inline f32 IVec2::Length() const
	{
		return sqrtf(static_cast<f32>(Dot()));
	}

	inline IVec2 IVec2::operator+(const IVec2 a) const
	{
		return IVec2(a.x + x, a.y + y);
	}

	inline IVec2 IVec2::operator+(const s32 a) const
	{
		return IVec2(a + x, a + y);
	}

	inline IVec2& IVec2::operator+=(const IVec2 a)
	{
		x += a.x;
		y += a.y;
		return* this;
	}

	inline IVec2& IVec2::operator+=(const s32 a)
	{
		x += a;
		y += a;
		return*this;
	}

	inline IVec2 IVec2::operator-(const IVec2 a) const
	{
		return IVec2(x - a.x, y - a.y);
	}

	inline IVec2 IVec2::operator-(const s32 a) const
	{
		return IVec2(x - a, y - a);
	}

	inline IVec2& IVec2::operator-=(const IVec2 a)
	{
		x -= a.x;
		y -= a.y;
		return*this;
	}

	inline IVec2& IVec2::operator-=(const s32 a)
	{
		x -= a;
		y -= a;
		return*this;
	}

	inline IVec2 IVec2::operator-() const
	{
		return IVec2(-x, -y);
	}

	inline IVec2 IVec2::operator*(const IVec2 a) const
	{
		return IVec2(x * a.x, y * a.y);
	}

	inline IVec2 IVec2::operator*(const f32 a) const
	{
		return IVec2(static_cast<s32>(x * a), static_cast<s32>(y * a));
	}

	inline IVec2& IVec2::operator*=(const IVec2 a)
	{
		x *= a.x;
		y *= a.y;
		return*this;
	}

	inline IVec2& IVec2::operator*=(const s32 a)
	{
		x *= a;
		y *= a;
		return*this;
	}

	inline IVec2 IVec2::operator/(const f32 a) const
	{
		if ((s32)a == 0)
			return IVec2(0x7fffffff, 0x7fffffff);
		return IVec2(x / (s32)a, y / (s32)a);
	}

	inline IVec2& IVec2::operator/=(const s32 a)
	{
		assert(a != 0);
		x /= a;
		y /= a;
		return *this;
	}

	inline bool IVec2::operator==(const IVec2 b) const
	{
		return (x == b.x && y == b.y);
	}

	inline bool IVec2::operator!=(const IVec2 b) const
	{
		return (x != b.x || y != b.y);;
	}

#pragma endregion

#pragma region Vec2

	inline f32 Vec2::Dot() const
	{
		return (x * x + y * y);
	}

	inline Vec2 Vec2::FromAngle(f32 angle)
	{
		return Vec2(cosf(angle), sinf(angle));
	}

	inline f32 Vec2::Length() const
	{
		return sqrtf(Dot());
	}

	inline Vec2 Vec2::operator+(const Vec2 a) const
	{
		return Vec2(a.x + x, a.y + y);
	}

	inline Vec2 Vec2::operator+(const f32 a) const
	{
		return Vec2(a + x, a + y);
	}

	inline Vec2& Vec2::operator+=(const Vec2 a)
	{
		x += a.x;
		y += a.y;
		return *this;
	}

	inline Vec2& Vec2::operator+=(const f32 a)
	{
		x += a;
		y += a;
		return *this;
	}

	inline Vec2 Vec2::operator-(const Vec2 a) const
	{
		return Vec2(x - a.x, y - a.y);
	}

	inline Vec2 Vec2::operator-(const f32 a) const
	{
		return Vec2(x - a, y - a);
	}

	inline Vec2& Vec2::operator-=(const Vec2 a)
	{
		x -= a.x;
		y -= a.y;
		return *this;
	}

	inline Vec2& Vec2::operator-=(const f32 a)
	{
		x -= a;
		y -= a;
		return *this;
	}

	inline Vec2 Vec2::operator-() const
	{
		return Negate();
	}

	inline Vec2 Vec2::operator*(const Vec2 a) const
	{
		return Vec2(x * a.x, y * a.y);
	}

	inline Vec2 Vec2::operator*(const f32 a) const
	{
		return Vec2(x * a, y * a);
	}

	inline Vec2& Vec2::operator*=(const Vec2 a)
	{
		x *= a.x;
		y *= a.y;
		return *this;
	}

	inline Vec2& Vec2::operator*=(const f32 a)
	{
		x *= a;
		y *= a;
		return *this;
	}

	inline Vec2 Vec2::operator/(const f32 a) const
	{
		return operator*(1 / a);
	}

	inline Vec2 Vec2::operator/(const Vec2 other) const
	{
		return Vec2(x/other.x, y/other.y);
	}

	inline Vec2& Vec2::operator/=(const Vec2 a)
	{
		x /= a.x;
		y /= a.y;
		return *this;
	}

	inline Vec2& Vec2::operator/=(const f32 a)
	{
		x /= a;
		y /= a;
		return *this;
	}

	inline bool Vec2::operator==(const Vec2 b) const
	{
		return (x == b.x && y == b.y);
	}

	inline bool Vec2::operator!=(const Vec2 b) const
	{
		return x != b.x || y != b.y;
	}

	inline f32& Vec2::operator[](const size_t a)
	{
		return *((&x) + a);
	}

	inline const f32& Vec2::operator[](const size_t a) const
	{
		return *((&x) + a);
	}

	inline bool Vec2::IsCollinearWith(Vec2 a) const
	{
		f32 res = a.x * y - a.y * x;
		return (res < VEC_COLLINEAR_PRECISION);
	}

	inline f32 Vec2::Dot(Vec2 a) const
	{
		return (a.x * x + a.y * y);
	}

	inline f32 Vec2::Cross(Vec2 a) const
	{
		return (x * a.y - y * a.x);
	}

	inline Vec2 Vec2::Normalize() const
	{
		return operator/(Length());
	}

	inline Vec2 Vec2::Negate() const
	{
		return operator*(-1);
	}

	inline Vec2 Vec2::GetNormal() const
	{
		return Vec2(-y, x);
	}

	inline bool Vec2::IsIntEquivalent(Vec2 a) const
	{
		return ((s32)x == (s32)a.x && (s32)y == a.y);
	}

	inline f32 Vec2::GetDistanceFromPoint(Vec2 a) const
	{
		f32 i = a.x - x;
		f32 j = a.y - y;
		return sqrtf(i * i + j * j);
	}

	inline f32 Vec2::GetAngle() const
	{
		return atan2f(y,x);
	}

	inline bool Vec2::IsNearlyEqual(Vec2 a, f32 prec)
	{
		return (fabsf(x-a.x) < prec) && (fabsf(y - a.y) < prec);
	}
#pragma endregion

#pragma region IVec3

	inline IVec3::IVec3(const Vec3& in) : x((s32)in.x), y((s32)in.y), z((s32)in.z) {}

	inline s32 IVec3::Dot() const
	{
		return x * x + y * y + z * z;
	}

	inline s32 IVec3::Dot(IVec3 in) const
	{
		return x * in.x + y * in.y + z * in.z;
	}

	inline f32 IVec3::Length() const
	{
		return sqrtf(static_cast<f32>(Dot()));
	}

	inline IVec3 IVec3::operator+(const IVec3& a) const
	{
		return IVec3(a.x + x, a.y + y, a.z + z);
	}

	inline IVec3 IVec3::operator+(const s32 a) const
	{
		return IVec3(a + x, a + y, a + z);
	}

	inline IVec3& IVec3::operator+=(const IVec3& a)
	{
		x += a.x;
		y += a.y;
		z += a.z;
		return *this;
	}

	inline IVec3& IVec3::operator+=(const s32 a)
	{
		x += a;
		y += a;
		z += a;
		return *this;
	}

	inline IVec3 IVec3::operator-(const IVec3& a) const
	{
		return IVec3(x - a.x, y - a.y, z - a.z);
	}

	inline IVec3 IVec3::operator-(const s32 a) const
	{
		return IVec3(x - a, y - a, z - a);
	}

	inline IVec3& IVec3::operator-=(const IVec3& a)
	{
		x -= a.x;
		y -= a.y;
		z -= a.z;
		return *this;
	}

	inline IVec3& IVec3::operator-=(const s32 a)
	{
		x -= a;
		y -= a;
		z -= a;
		return *this;
	}

	inline IVec3 IVec3::operator*(const IVec3& a) const
	{
		return IVec3(x * a.x, y * a.y, z * a.z);
	}

	inline IVec3& IVec3::operator*=(const IVec3& a)
	{
		x *= a.x;
		y *= a.y;
		z *= a.z;
		return *this;
	}

	inline IVec3& IVec3::operator*=(const s32 a)
	{
		x *= a;
		y *= a;
		z *= a;
		return *this;
	}

	inline IVec3 IVec3::operator*(const f32 a) const
	{
		return IVec3(x * (s32)a, y * (s32)a, z * (s32)a);
	}

	inline IVec3 IVec3::operator/(const f32 a) const
	{
		if ((s32)a == 0)
			return IVec3(0x7fffffff, 0x7fffffff, 0x7fffffff);
		return IVec3(x / (s32)a, y / (s32)a, z / (s32)a);
	}

	inline IVec3 IVec3::operator/(const IVec3& a) const
	{
		return IVec3(x / a.x, y / a.y, z / a.z);
	}

	inline IVec3& IVec3::operator/=(const IVec3& a)
	{
		x /= a.x;
		y /= a.y;
		z /= a.z;
		return *this;
	}

	inline IVec3& IVec3::operator/=(const s32 a)
	{
		assert(a != 0);
		x /= a;
		y /= a;
		z /= a;
		return *this;
	}

	inline bool IVec3::operator==(const IVec3& b) const
	{
		return (x == b.x && y == b.y && z == b.z);
	}

	inline bool IVec3::operator!=(const IVec3& b) const
	{
		return !operator==(b);
	}

	inline const s32& IVec3::operator[](const size_t a) const
	{
		return *((&x) + a);
	}

	inline s32& IVec3::operator[](const size_t a)
	{
		return *((&x) + a);
	}
#pragma endregion

#pragma region Vec3

	inline f32 Vec3::Dot() const
	{
		return (x * x + y * y + z * z);
	}

	inline f32 Vec3::Length() const
	{
		return sqrtf(Dot());
	}

	inline Vec3 Vec3::operator+(const Vec3& a) const
	{
		return Vec3(a.x + x, a.y + y, a.z + z);
	}

	inline Vec3 Vec3::operator+(const f32 a) const
	{
		return Vec3(a + x, a + y, a + z);
	}

	inline Vec3& Vec3::operator+=(const Vec3& a)
	{
		x += a.x;
		y += a.y;
		z += a.z;
		return *this;
	}

	inline Vec3& Vec3::operator+=(const f32 a)
	{
		x += a;
		y += a;
		z += a;
		return *this;
	}

	inline Vec3 Vec3::operator-(const Vec3& a) const
	{
		return Vec3(x - a.x, y - a.y, z - a.z);
	}

	inline Vec3 Vec3::operator-(const f32 a) const
	{
		return Vec3(x - a, y - a, z - a);
	}

	inline Vec3& Vec3::operator-=(const Vec3& a)
	{
		x -= a.x;
		y -= a.y;
		z -= a.z;
		return *this;
	}

	inline Vec3& Vec3::operator-=(const f32 a)
	{
		x -= a;
		y -= a;
		z -= a;
		return *this;
	}

	inline Vec3 Vec3::operator-() const
	{
		return Negate();
	}

	inline Vec3 Vec3::operator*(const Vec3& a) const
	{
		Vec3 res = Vec3(x * a.x, y * a.y, z * a.z);
		return res;
	}

	inline Vec3 Vec3::operator*(const f32 a) const
	{
		return Vec3(x * a, y * a, z * a);
	}

	inline Vec3& Vec3::operator*=(const Vec3& a)
	{
		x *= a.x;
		y *= a.y;
		z *= a.z;
		return *this;
	}

	inline Vec3& Vec3::operator*=(const f32 a)
	{
		x *= a;
		y *= a;
		z *= a;
		return *this;
	}

	inline Vec3 Vec3::operator/(const f32 a) const
	{
		return operator*(1 / a);
	}

	inline Vec3 Vec3::operator/(const Vec3& a) const
	{
		return Vec3(x / a.x, y / a.y, z / a.z);
	}

	inline Vec3& Vec3::operator/=(const Vec3& a)
	{
		x /= a.x;
		y /= a.y;
		z /= a.z;
		return *this;
	}

	inline Vec3& Vec3::operator/=(const f32 a)
	{
		x /= a;
		y /= a;
		z /= a;
		return *this;
	}

	inline bool Vec3::operator==(const Vec3& b) const
	{
		return (x == b.x && y == b.y && z == b.z);
	}

	inline bool Vec3::operator!=(const Vec3& b) const
	{
		return !operator==(b);
	}

	inline f32& Vec3::operator[](const size_t a)
	{
		return *((&x) + a);
	}

	inline const f32& Vec3::operator[](const size_t a) const
	{
		return *((&x) + a);
	}

	inline f32 Vec3::Dot(Vec3 a) const
	{
		return (a.x * x + a.y * y + a.z * z);
	}

	inline Vec3 Vec3::Reflect(const Vec3& normal)
	{
		return operator-(normal * (2 * Dot(normal)));
	}

	inline Vec3 Vec3::Refract(const Vec3& normal, f32 ior)
	{
		f32 cosi = Dot(normal);
		f32 cost2 = 1 - ior * ior * (1 - cosi * cosi);
		if (cost2 <= 0) return Vec3();
		return (operator*(ior) - normal * (ior * cosi + sqrtf(cost2))).Normalize();
	}

	inline bool Vec3::IsCollinearWith(Vec3 a) const
	{
		f32 res = Dot(a);
		return (res < VEC_COLLINEAR_PRECISION);
	}

	inline Vec3 Vec3::Cross(Vec3 a) const
	{
		return Vec3((y * a.z) - (z * a.y), (z * a.x) - (x * a.z), (x * a.y) - (y * a.x));
	}

	inline Vec3 Vec3::Normalize() const
	{
		return operator/(Length());
	}

	inline Vec3 Vec3::Negate() const
	{
		return operator*(-1);
	}

	inline Vec3 Vec3::GetPerpendicular() const
	{
		return Vec3(copysignf(z, x), copysignf(z, y), -copysignf(x, z) - copysignf(y, z));
	}

	inline bool Vec3::IsIntEquivalent(Vec3 a) const
	{
		return ((s32)x == (s32)a.x && (s32)y == a.y && (s32)z == (s32)a.z);
	}

	inline bool Vec3::IsNearlyEqual(Vec3 a, f32 prec)
	{
		return (fabsf(x - a.x) < prec) && (fabsf(y - a.y) < prec) && (fabsf(z - a.z) < prec);
	}
#pragma endregion

#pragma region Vec4

	inline Vec3 Vec4::GetVector() const
	{
		return Vec3(x, y, z);
	}

	inline Vec4 Vec4::Homogenize() const
	{
		return Vec4(GetVector() / w);
	}

	inline Vec4 Vec4::Normalize() const
	{
		Vec4 homogenized = Homogenize();
		return homogenized / homogenized.GetVector().Length();
	}

	inline f32 Vec4::Dot() const
	{
		return Homogenize().GetVector().Dot();
	}

	inline f32 Vec4::Length() const
	{
		return sqrtf(Dot());
	}

	inline Vec4 Vec4::operator+(const Vec4& a) const
	{
		return Vec4(x + a.x, y + a.y, z + a.z, w + a.w);
	}

	inline Vec4 Vec4::operator+(const f32 a) const
	{
		return Vec4(x + a, y + a, z + a, w + a);
	}

	inline Vec4& Vec4::operator+=(const Vec4& a)
	{
		x += a.x;
		y += a.y;
		z += a.z;
		w += a.w;
		return *this;
	}

	inline Vec4& Vec4::operator+=(const f32 a)
	{
		x += a;
		y += a;
		z += a;
		w += a;
		return *this;
	}

	inline Vec4 Vec4::operator-(const Vec4& a) const
	{
		return Vec4(x - a.x, y - a.y, z - a.z, w - a.w);
	}

	inline Vec4 Vec4::operator-(const f32 a) const
	{
		return Vec4(x - a, y - a, z - a, w - a);
	}

	inline Vec4& Vec4::operator-=(const Vec4& a)
	{
		x -= a.x;
		y -= a.y;
		z -= a.z;
		w -= a.w;
		return *this;
	}

	inline Vec4& Vec4::operator-=(const f32 a)
	{
		x -= a;
		y -= a;
		z -= a;
		w -= a;
		return *this;
	}

	inline Vec4 Vec4::operator-() const
	{
		return Negate();
	}

	inline Vec4 Vec4::operator*(const Vec4& a) const
	{
		return Vec4(x * a.x, y * a.y, z * a.z, w * a.w);
	}

	inline Vec4 Vec4::operator*(const f32 a) const
	{
		return Vec4(x * a, y * a, z * a, w * a);
	}

	inline Vec4& Vec4::operator*=(const Vec4& a)
	{
		x *= a.x;
		y *= a.y;
		z *= a.z;
		w *= a.w;
		return *this;
	}

	inline Vec4& Vec4::operator*=(const f32 a)
	{
		x *= a;
		y *= a;
		z *= a;
		w *= a;
		return *this;
	}

	inline Vec4 Vec4::operator/(const f32 b) const
	{
		return operator*(1 / b);
	}

	inline Vec4 Vec4::operator/(const Vec4& a) const
	{
		return Vec4(x / a.x, y / a.y, z / a.z, w / a.w);
	}

	inline Vec4& Vec4::operator/=(const Vec4& a)
	{
		x /= a.x;
		y /= a.y;
		z /= a.z;
		w /= a.w;
		return *this;
	}

	inline Vec4& Vec4::operator/=(const f32 a)
	{
		x /= a;
		y /= a;
		z /= a;
		w /= a;
		return *this;
	}

	inline bool Vec4::operator==(const Vec4& b) const
	{
		return (x == b.x && y == b.y && z == b.z && w == b.w);
	}

	inline bool Vec4::operator!=(const Vec4& b) const
	{
		return !operator==(b);
	}

	inline f32& Vec4::operator[](const size_t a)
	{
		return *((&x) + a);
	}

	inline const f32& Vec4::operator[](const size_t a) const
	{
		return *((&x) + a);
	}

	inline bool Vec4::IsCollinearWith(Vec4 a) const
	{
		f32 res = Dot(a);
		return (res < VEC_COLLINEAR_PRECISION);
	}

	inline f32 Vec4::Dot(Vec4 a) const
	{
		return (a.x * x + a.y * y + a.z * z + a.w * w);
	}

	inline Vec4 Vec4::Cross(Vec4 a) const
	{
		return Vec4((y * a.z) - (z * a.y), (z * a.x) - (x * a.z), (x * a.y) - (y * a.x), 1.0f);
	}

	inline Vec4 Vec4::Negate() const
	{
		return operator*(-1);
	}

	inline Vec4 Vec4::Clip(const Vec4& other)
	{
		return Vec4(Util::MaxF(x, other.x), Util::MaxF(y, other.y), Util::MinF(z, other.z), Util::MinF(w, other.w));
	}

	inline bool Vec4::IsIntEquivalent(Vec4 a) const
	{
		return ((s32)x == (s32)a.x && (s32)y == a.y && (s32)z == (s32)a.z && (s32)w == (s32)a.w);
	}

	inline bool Vec4::IsNearlyEqual(Vec4 a, f32 prec)
	{
		return (fabsf(x - a.x) < prec) && (fabsf(y - a.y) < prec) && (fabsf(z - a.z) < prec) && (fabsf(w - a.w) < prec);
	}

	inline f32 Vec4::GetSignedDistanceToPlane(const Vec3& point) const
	{
		return GetVector().Dot(point) - w;
	}

#pragma endregion

#pragma region Color4

	inline Color4::Color4(const f32* in)
	{
		r = (u8)(in[0] * 255);
		g = (u8)(in[1] * 255);
		b = (u8)(in[2] * 255);
		a = (u8)(in[3] * 255);
	}

	inline Color4::Color4(const Vec4& in)
	{
		r = (u8)(Util::Clamp(in[0], 0.0f, 1.0f) * 255);
		g = (u8)(Util::Clamp(in[1], 0.0f, 1.0f) * 255);
		b = (u8)(Util::Clamp(in[2], 0.0f, 1.0f) * 255);
		a = (u8)(Util::Clamp(in[3], 0.0f, 1.0f) * 255);
	}

	inline Color4 Color4::operator*(const f32 in) const
	{
		return Color4(r * (s32)in, g * (s32)in, b * (s32)in, a);
	}

	inline Color4 Color4::operator+(const Color4& in) const
	{
		return Color4(r + in.r, g + in.g, b + in.b, a);
	}

#pragma endregion

#pragma region Mat4

	inline f32& Mat4::operator[](const size_t in)
	{
		assert(in < 16);
		return content[in];
	}

	inline const f32& Mat4::operator[](const size_t in) const
	{
		assert(in < 16);
		return content[in];
	}

	inline f32& Mat4::at(const u8 x, const u8 y)
	{
		assert(x < 4 && y < 4);
		return content[x*4+y];
	}

	inline const f32& Mat4::at(const u8 x, const u8 y) const
	{
		assert(x < 4 && y < 4);
		return content[x * 4 + y];
	}

#pragma endregion

#pragma region Mat3

	inline f32& Mat3::operator[](const size_t in)
	{
		assert(in < 9);
		return content[in];
	}

	inline const f32& Mat3::operator[](const size_t in) const
	{
		assert(in < 9);
		return content[in];
	}

	inline f32& Mat3::at(const u8 x, const u8 y)
	{
		assert(x < 3 && y < 3);
		return content[x * 3 + y];
	}

#pragma endregion

#pragma region Quat

	inline Quat::Quat(const Mat3& in) : Quat(Mat4(in))
	{
	}

	inline Quat::Quat(const Mat4& in)
	{
		Vec3 scale = in.GetScaleFromTranslation();
		float tr = in.at(0, 0) / scale.x + in.at(1, 1) / scale.y + in.at(2, 2) / scale.z;
		if (tr > 0)
		{
			float S = sqrtf(tr + 1.0f) * 2; // S=4*qw 
			a = 0.25f * S;
			v = Vec3(in.at(1, 2) / scale.y - in.at(2, 1) / scale.z, in.at(2, 0) / scale.z - in.at(0, 2) / scale.x, in.at(0, 1) / scale.x - in.at(1, 0) / scale.y) / S;
		}
		else
		{
			if ((in.at(0, 0) / scale.x > in.at(1, 1) / scale.y) && (in.at(0, 0) / scale.x > in.at(2, 2) / scale.z))
			{
				float S = sqrtf(1.0f + in.at(0, 0) / scale.x - in.at(1, 1) / scale.y - in.at(2, 2) / scale.z) * 2; // S=4*qx 
				a = (in.at(1, 2) / scale.y - in.at(2, 1) / scale.z) / S;
				v = Vec3(0.25f * S, (in.at(1, 0) / scale.y + in.at(0, 1) / scale.x) / S, (in.at(2, 0) / scale.z + in.at(0, 2) / scale.x) / S);
			}
			else if (in.at(1, 1) / scale.y > in.at(2, 2) / scale.z)
			{
				float S = sqrtf(1.0f + in.at(1, 1) / scale.y - in.at(0, 0) / scale.x - in.at(2, 2) / scale.z) * 2; // S=4*qy
				a = (in.at(2, 0) / scale.z - in.at(0, 2) / scale.x) / S;
				v = Vec3((in.at(1, 0) / scale.y + in.at(0, 1) / scale.x) / S, 0.25f * S, (in.at(2, 1) / scale.z + in.at(1, 2) / scale.y) / S);
			}
			else
			{
				float S = sqrtf(1.0f + in.at(2, 2) / scale.z - in.at(0, 0) / scale.x - in.at(1, 1) / scale.y) * 2; // S=4*qz
				a = (in.at(0, 1) / scale.x - in.at(1, 0) / scale.y) / S;
				v = Vec3((in.at(2, 0) / scale.z + in.at(0, 2) / scale.x) / S, (in.at(2, 1) / scale.z + in.at(1, 2) / scale.y) / S, 0.25f * S);
			}
		}
	}

	inline f32 Quat::Dot() const
	{
		return a*a + v.Dot();
	}

	inline f32 Quat::Length() const
	{
		return sqrtf(Dot());
	}

	inline Quat Quat::Conjugate() const
	{
		return Quat(-v, a);
	}

	inline Quat Quat::Normalize() const
	{
		return operator/(Length());
	}

	inline Quat Quat::NormalizeAxis() const
	{
		f32 vsq = v.Dot();
		f32 asq = a * a;
		if (asq > 1.0f || vsq < 0.0001f) return Normalize();
		f32 b = sqrtf((1 - asq) / (vsq));
		return Quat(v * b, a).Normalize();
	}

	inline Quat Quat::Inverse() const
	{
		if (Dot() < 1e-5f) return *this;
		return Conjugate()/Length();
	}

	inline Quat Quat::AxisAngle(Vec3 axis, f32 angle)
	{
		f32 hAngle = angle / 2;
		return Quat(axis * sinf(hAngle), cosf(hAngle));
	}

	inline Quat Quat::FromEuler(Vec3 euler)
	{
		Quat qx = Quat::AxisAngle(Vec3(1, 0, 0), euler.x);
		Quat qy = Quat::AxisAngle(Vec3(0, 1, 0), euler.y);
		Quat qz = Quat::AxisAngle(Vec3(0, 0, 1), euler.z);
		return qy * qx * qz;
	}

	inline f32 Quat::GetAngle()
	{
		return 2 * acosf(a);
	}

	inline Vec3 Quat::GetAxis()
	{
		f32 factor = sqrtf(1-a*a);
		return v / factor;
	}

	inline Mat3 Quat::GetRotationMatrix3() const
	{
		Mat3 result;
		f32 b = v.x;
		f32 c = v.y;
		f32 d = v.z;
		result.at(0, 0) = 2 * (a*a + b*b) - 1;
		result.at(1, 0) = 2 * (b*c - d*a);
		result.at(2, 0) = 2 * (b*d + c*a);
		result.at(0, 1) = 2 * (b*c + d*a);
		result.at(1, 1) = 2 * (a*a + c*c) - 1;
		result.at(2, 1) = 2 * (c*d - b*a);
		result.at(0, 2) = 2 * (b*d - c*a);
		result.at(1, 2) = 2 * (c*d + b*a);
		result.at(2, 2) = 2 * (a*a + d*d) - 1;
		return result;
	}

	inline Mat4 Quat::GetRotationMatrix4() const
	{
		Mat4 result;
		f32 b = v.x;
		f32 c = v.y;
		f32 d = v.z;
		result.at(0, 0) = 2 * (a * a + b * b) - 1;
		result.at(1, 0) = 2 * (b * c - d * a);
		result.at(2, 0) = 2 * (b * d + c * a);
		result.at(0, 1) = 2 * (b * c + d * a);
		result.at(1, 1) = 2 * (a * a + c * c) - 1;
		result.at(2, 1) = 2 * (c * d - b * a);
		result.at(0, 2) = 2 * (b * d - c * a);
		result.at(1, 2) = 2 * (c * d + b * a);
		result.at(2, 2) = 2 * (a * a + d * d) - 1;
		result.at(3, 3) = 1;
		return result;
	}

	inline Quat Quat::operator+(const Quat& other) const
	{
		return Quat(v + other.v, a + other.a);
	}

	inline Quat Quat::operator-(const Quat& other) const
	{
		return Quat(v - other.v, a - other.a);
	}

	inline Quat Quat::operator-() const
	{
		return Quat(-v, -a);
	}

	inline Quat Quat::operator*(const Quat& other) const
	{
		return Quat(other.v * a + v * other.a + v.Cross(other.v), a*other.a - v.Dot(other.v));
	}

	inline Vec3 Quat::operator*(const Vec3& other) const
	{
		Quat tmp = operator*(Quat(other, 0.0f)) * Inverse();
		return Vec3(tmp.v);
	}

	inline Quat Quat::operator*(const f32 scalar) const
	{
		return Quat(v * scalar, a * scalar);
	}

	inline Quat Quat::operator/(const Quat& other) const
	{
		return Quat(v / other.v, a / other.a);
	}

	inline Quat Quat::operator/(const f32 scalar) const
	{
		return Quat(v / scalar, a / scalar);
	}

	inline Vec3 Quat::GetRight() const
	{
		return operator*(Vec3(1, 0, 0));
	}

	inline Vec3 Quat::GetUp() const
	{
		return operator*(Vec3(0, 1, 0));
	}

	inline Vec3 Quat::GetFront() const
	{
		return operator*(Vec3(0, 0, 1));
	}

	Quat Quat::Slerp(const Quat& a, Quat b, f32 alpha)
	{
		Quat result = Quat();
		f32 cosHalfTheta = a.a * b.a + a.v.x * b.v.x + a.v.y * b.v.y + a.v.z * b.v.z;
		if (cosHalfTheta < 0) {
			b = -b;
			cosHalfTheta = -cosHalfTheta;
		}
		if (fabsf(cosHalfTheta) >= 1.0f) {
			result = a;
			return result;
		}
		f32 halfTheta = acosf(cosHalfTheta);
		f32 sinHalfTheta = sqrtf(1.0f - cosHalfTheta * cosHalfTheta);
		if (fabsf(sinHalfTheta) < 0.001f)
		{
			result = a * 0.5f + b * 0.5f;
			return result;
		}
		f32 ratioA = sinf((1 - alpha) * halfTheta) / sinHalfTheta;
		f32 ratioB = sinf(alpha * halfTheta) / sinHalfTheta;
		result = a * ratioA + b * ratioB;
		return result;
	}

#pragma endregion

#pragma region Utils

	inline f32 Util::ToRadians(f32 in)
	{
		return in / 180.0f * (f32)M_PI;
	}

	inline f32 Util::ToDegrees(f32 in)
	{
		return in * 180.0f / (f32)M_PI;
	}

	inline f32 Util::Clamp(f32 in, f32 min, f32 max)
	{
		if (in < min)
			in = min;
		if (in > max)
			in = max;
		return in;
	}

	inline Vec2 Util::Clamp(Vec2 in, f32 min, f32 max)
	{
		for (u8 i = 0; i < 2; ++i)
		{
			in[i] = Clamp(in[i], min, max);
		}
		return in;
	}

	inline Vec2 Util::Clamp(IVec2 in, IVec2 min, IVec2 max)
	{
		in.x = IClamp(in.x, min.x, max.x-1);
		in.y = IClamp(in.y, min.y, max.y-1);
		return in;
	}

	inline Vec3 Util::Clamp(Vec3 in, f32 min, f32 max)
	{
		for (u8 i = 0; i < 3; ++i)
		{
			in[i] = Clamp(in[i], min, max);
		}
		return in;
	}

	inline Vec4 Util::Clamp(Vec4 in, f32 min, f32 max)
	{
		for (u8 i = 0; i < 4; ++i)
		{
			in[i] = Clamp(in[i], min, max);
		}
		return in;
	}

	inline f32 Util::Abs(f32 in)
	{
		return in >= 0 ? in : -in;
	}

	inline Vec2 Util::Abs(Vec2 in)
	{
		for (u8 i = 0; i < 2; ++i)
		{
			in[i] = Abs(in[i]);
		}
		return in;
	}

	inline Vec3 Util::Abs(Vec3 in)
	{
		for (u8 i = 0; i < 3; ++i)
		{
			in[i] = Abs(in[i]);
		}
		return in;
	}

	inline Vec4 Util::Abs(Vec4 in)
	{
		for (u8 i = 0; i < 4; ++i)
		{
			in[i] = Abs(in[i]);
		}
		return in;
	}

	inline s32 Util::IClamp(s32 in, s32 min, s32 max)
	{
		if (in < min)
			in = min;
		if (in > max)
			in = max;
		return in;
	}

	inline u32 Util::UClamp(u32 in, u32 min, u32 max)
	{
		if (in < min)
			in = min;
		if (in > max)
			in = max;
		return in;
	}

	inline f32 Util::Lerp(f32 a, f32 b, f32 delta)
	{
		return a + delta * (b - a);
	}

	inline Vec3 Util::Lerp(Vec3 a, Vec3 b, f32 delta)
	{
		return a + (b - a) * delta;
	}

	inline f32 Util::Mod(f32 in, f32 value)
	{
		return in - value * floorf(in / value);
	}

	Vec2 Util::Mod(Vec2 in, f32 value)
	{
		for (u8 i = 0; i < 2; i++)
		{
			in[i] = Mod(in[i], value);
		}
		return in;
	}

	Vec3 Util::Mod(Vec3 in, f32 value)
	{
		for (u8 i = 0; i < 3; i++)
		{
			in[i] = Mod(in[i], value);
		}
		return in;
	}

	inline s32 Util::IMod(s32 in, s32 value)
	{
		s32 tmp = in % value;
		if (tmp < 0) tmp += value;
		return tmp;
	}

	inline f32 Util::MinF(f32 a, f32 b)
	{
		if (a > b)
			return b;
		return a;
	}

	inline f32 Util::MaxF(f32 a, f32 b)
	{
		if (a > b)
			return a;
		return b;
	}

	inline Vec3 Util::MinV(Vec3 a, Vec3 b)
	{
		Vec3 result;
		for (u8 i = 0; i < 3; ++i)
		{
			result[i] = MinF(a[i], b[i]);
		}
		return result;
	}

	inline Vec3 Util::MaxV(Vec3 a, Vec3 b)
	{
		Vec3 result;
		for (u8 i = 0; i < 3; ++i)
		{
			result[i] = MaxF(a[i], b[i]);
		}
		return result;
	}

	inline s32 Util::MinI(s32 a, s32 b)
	{
		if (a > b)
			return b;
		return a;
	}

	inline s32 Util::MaxI(s32 a, s32 b)
	{
		if (a > b)
			return a;
		return b;
	}

	inline f32 Util::SMin(f32 a, f32 b, f32 delta)
	{
		f32 half = Clamp(0.5f + 0.5f * (a - b) / delta, 0.0f, 1.0f);
		return Lerp(a, b, half) - delta * half * (1.0f - half);
	}

	inline bool Util::IsNear(f32 a, f32 b, f32 prec)
	{
		a -= b;
		return (a <= prec && a >= -prec);
	}

	static const char* digits = "0123456789ABCDEF";

	inline std::string Util::GetHex(u64 number)
	{
		std::string result = std::string();
		result.resize(16);
		for (u8 i = 0; i < 8; ++i)
		{
			u8 digit = (number >> (i * 8)) & 0xff;
			result[i * 2llu + 1] = digits[digit & 0xf];
			result[i * 2llu] = digits[digit >> 4];
		}
		return result;
	}

	inline void Util::GetHex(char* buffer, u64 number)
	{
		for (u8 i = 0; i < 8; ++i)
		{
			u8 digit = (number >> (i * 8)) & 0xff;
			buffer[i * 2llu + 1] = digits[digit & 0xf];
			buffer[i * 2llu] = digits[digit >> 4];
		}
	}

	inline u64 Util::ReadHex(const std::string& input)
	{
		u64 result = 0;
		for (u64 i = 0; i < 16; ++i)
		{
			u8 digit = 0;
			if (i < input.size())
			{
				char c = input[i];
				if (c >= '0' && c <= '9')
				{
					digit = c - '0';
				}
				else if (c >= 'A' && c <= 'F')
				{
					digit = c - 'A' + 10;
				}
				else if (c > 'a' && c < 'f')
				{
					digit = c - 'a' + 10;
				}
			}
			result |= static_cast<u64>(digit) << ((i ^ 0x1) * 4);
		}
		return result;
	}

#pragma endregion

}