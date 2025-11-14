#pragma once

#include <math.h>
#include <vector>
#include <string>

#include "Types.hpp"

namespace Maths
{

	static const f32 VEC_COLLINEAR_PRECISION = 0.001f;
	static const f32 VEC_HIGH_VALUE = 1e38f;

	class Vec2;
	class Quat;

	class IVec2
	{
	public:
		s32 x, y;
		inline IVec2() : x(0), y(0) {}
		inline IVec2(const IVec2& in) : x(in.x), y(in.y) {}
		inline IVec2(const Vec2 in);
		inline IVec2(const s32 a, const s32 b) : x(a), y(b) {}

		void print() const;
		const std::string toString() const;

		inline s32 Dot(IVec2 a) const;
		// Return the length squared of this object
		inline s32 Dot() const;

		// Return the lenght of the given Vector
		inline f32 Length() const;

		inline IVec2 operator+(const IVec2 a) const;

		inline IVec2 operator+(const s32 a) const;

		inline IVec2& operator+=(const IVec2 a);

		inline IVec2& operator+=(const s32 a);

		// Return a new vector wich is the substraction of 'a' and 'b'
		inline IVec2 operator-(const IVec2 a) const;

		inline IVec2 operator-(const s32 a) const;

		inline IVec2& operator-=(const IVec2 a);

		inline IVec2& operator-=(const s32 a);

		inline IVec2 operator-() const;

		// Return the result of the aritmetic multiplication of 'a' and 'b'
		inline IVec2 operator*(const IVec2 a) const;

		inline IVec2& operator*=(const IVec2 a);

		inline IVec2 operator*(const f32 a) const;

		inline IVec2& operator*=(const s32 a);

		inline IVec2 operator/(const f32 b) const;

		inline IVec2& operator/=(const s32 b);

		inline bool operator==(const IVec2 b) const;

		inline bool operator!=(const IVec2 b) const;
	};

	class Vec2
	{
	public:
		f32 x;
		f32 y;

		// Return a new empty Vec2
		inline Vec2() : x(0), y(0) {}

		// Return a new Vec2 initialised with 'a' and 'b'
		inline Vec2(f32 a, f32 b) : x(a), y(b) {}

		inline Vec2(f32 value) : Vec2(value, value) {}

		// Return a new Vec2 initialised with 'in'
		inline Vec2(const Vec2& in) : x(in.x), y(in.y) {}
		inline Vec2(const IVec2 in) : x((f32)in.x), y((f32)in.y) {}

		void print() const;
		const std::string toString() const;

		// Return a new Vec2 equivalent to Vec(1,0) rotated by 'angle' (in radians)
		inline static Vec2 FromAngle(float angle);

		//Return the distance between this object and 'a'
		inline f32 GetDistanceFromPoint(Vec2 a) const;

		// Return the lenght of the given Vector
		inline f32 Length() const;

		inline Vec2 operator+(const Vec2 a) const;
		inline Vec2& operator+=(const Vec2 a);
		inline Vec2 operator+(const f32 a) const;
		inline Vec2& operator+=(const f32 a);

		inline Vec2 operator-(const Vec2 a) const;
		inline Vec2& operator-=(const Vec2 a);
		inline Vec2 operator-(const f32 a) const;
		inline Vec2& operator-=(const f32 a);

		inline Vec2 operator-() const;

		inline Vec2 operator*(const Vec2 a) const;
		inline Vec2& operator*=(const Vec2 a);
		inline Vec2 operator*(const f32 a) const;
		inline Vec2& operator*=(const f32 a);

		inline Vec2 operator/(const f32 b) const;
		inline Vec2 operator/(const Vec2 b) const;
		inline Vec2& operator/=(const f32 b);
		inline Vec2& operator/=(const Vec2 b);

		inline bool operator==(const Vec2 b) const;
		inline bool operator!=(const Vec2 b) const;

		inline const f32& operator[](const size_t a) const;

		inline f32& operator[](const size_t a);

		// Return true if 'a' and 'b' are collinears (Precision defined by VEC_COLLINEAR_PRECISION)
		inline bool IsCollinearWith(Vec2 a) const;

		inline f32 Dot(Vec2 a) const;
		// Return the length squared of this object
		inline f32 Dot() const;

		// Return the z component of the cross product of 'a' and 'b'
		inline f32 Cross(Vec2 a) const;

		// Return a vector with the same direction that 'in', but with length 1
		inline Vec2 Normalize() const;

		// Return a vector of length 'in' and with an opposite direction
		inline Vec2 Negate() const;

		// Return the normal vector of 'in'
		inline Vec2 GetNormal() const;

		// return true if 'a' converted to s32 is equivalent to 'in' converted to s32
		inline bool IsIntEquivalent(Vec2 a) const;

		// Get the angle defined by this vector, in radians
		inline f32 GetAngle() const;

		inline bool IsNearlyEqual(Vec2 a, f32 prec = 1e-5f);

#ifdef IMGUI_API
		inline Vec2(const ImVec2& in) : x(in.x), y(in.y) {}

		inline operator ImVec2() const { return ImVec2(x, y); }
#endif

#ifdef JOLT_API
		inline Vec2(const JPH::Float2& in) : x(in.x), y(in.y) {}
#endif

#ifdef ASSIMP_API
		inline Vec2(const aiVector2D& in) : x(in.x), y(in.y) {}

		inline operator aiVector2D() const { return aiVector2D(x, y); }
#endif

	};

	class Vec3;

	class IVec3
	{
	public:
		s32 x, y, z;
		inline IVec3() : x(0), y(0), z(0) {}
		inline IVec3(const IVec3& in) : x(in.x), y(in.y), z(in.z) {}
		inline IVec3(const Vec3& in);
		inline IVec3(const s32& a, const s32& b, const s32& c) : x(a), y(b), z(c) {}

		void print() const;
		const std::string toString() const;

		inline s32 Dot(IVec3 a) const;
		// Return the length squared of this object
		inline s32 Dot() const;

		// Return the lenght of the given Vector
		inline f32 Length() const;

		inline IVec3 operator+(const IVec3& a) const;
		inline IVec3 operator+(const s32 a) const;
		inline IVec3& operator+=(const IVec3& a);
		inline IVec3& operator+=(const s32 a);

		inline IVec3 operator-(const IVec3& a) const;
		inline IVec3 operator-(const s32 a) const;
		inline IVec3& operator-=(const IVec3& a);
		inline IVec3& operator-=(const s32 a);

		inline IVec3 operator*(const IVec3& a) const;
		inline IVec3 operator*(const f32 a) const;
		inline IVec3& operator*=(const IVec3& a);
		inline IVec3& operator*=(const s32 a);

		inline IVec3 operator/(const IVec3& a) const;
		inline IVec3 operator/(const f32 b) const;
		inline IVec3& operator/=(const IVec3& a);
		inline IVec3& operator/=(const s32 a);

		inline bool operator==(const IVec3& b) const;
		inline bool operator!=(const IVec3& b) const;

		inline const s32& operator[](const size_t a) const;

		inline s32& operator[](const size_t a);
	};

	class Vec3
	{
	public:
		f32 x;
		f32 y;
		f32 z;

		inline Vec3() : x(0), y(0), z(0) {}

		inline Vec3(f32 content) : x(content), y(content), z(content) {}

		inline Vec3(f32 a, f32 b, f32 c) : x(a), y(b), z(c) {}

		// Return a new Vec3 initialised with 'in'
		inline Vec3(const Vec3& in) : x(in.x), y(in.y), z(in.z) {}

		inline Vec3(const IVec3& in) : x((f32)in.x), y((f32)in.y), z((f32)in.z) {}

		void Print() const;
		const std::string ToString() const;

		inline f32 Dot() const;

		inline f32 Length() const;

		inline Vec3 operator+(const Vec3& a) const;
		inline Vec3 operator+(const f32 a) const;
		inline Vec3& operator+=(const Vec3& a);
		inline Vec3& operator+=(const f32 a);

		inline Vec3 operator-(const Vec3& a) const;
		inline Vec3 operator-(const f32 a) const;
		inline Vec3& operator-=(const Vec3& a);
		inline Vec3& operator-=(const f32 a);

		inline Vec3 operator-() const;

		inline Vec3 operator*(const Vec3& a) const;
		inline Vec3 operator*(const f32 a) const;
		inline Vec3& operator*=(const Vec3& a);
		inline Vec3& operator*=(const f32 a);

		inline Vec3 operator/(const Vec3& b) const;
		inline Vec3 operator/(const f32 a) const;
		inline Vec3& operator/=(const Vec3& a);
		inline Vec3& operator/=(const f32 a);

		inline bool operator==(const Vec3& b) const;
		inline bool operator!=(const Vec3& b) const;

		inline const f32& operator[](const size_t a) const;

		inline f32& operator[](const size_t a);

		inline Vec3 Reflect(const Vec3& normal);

		inline Vec3 Refract(const Vec3& normal, f32 ior);

		// Return tue if 'a' and 'b' are collinears (Precision defined by VEC_COLLINEAR_PRECISION)
		inline bool IsCollinearWith(Vec3 a) const;

		// Return the dot product of 'a' and 'b'
		inline f32 Dot(Vec3 a) const;

		// Return the z component of the cross product of 'a' and 'b'
		inline Vec3 Cross(Vec3 a) const;

		// Return a vector with the same direction that 'in', but with length 1
		inline Vec3 Normalize() const;

		// Return a vector of length 'in' and with an opposite direction
		inline Vec3 Negate() const;

		// Found this here: https://math.stackexchange.com/q/4112622
		inline Vec3 GetPerpendicular() const;

		// return true if 'a' converted to s32 is equivalent to 'in' converted to s32
		inline bool IsIntEquivalent(Vec3 a) const;

		inline bool IsNearlyEqual(Vec3 a, f32 prec = 1e-5f);
#ifdef JOLT_API
		inline Vec3(const JPH::Vec3Arg& in) : x(in.GetX()), y(in.GetY()), z(in.GetZ()) {}

		inline operator JPH::Vec3Arg() const { return JPH::Vec3Arg(x, y, z); }

		inline operator JPH::Float3() const { return JPH::Float3(x, y, z); }

		inline Vec3(const JPH::Float3& in) : x(in.x), y(in.y), z(in.z) {}

		inline Vec3(const JPH::ColorArg& in) : x(in.r), y(in.g), z(in.b) {}
#endif

#ifdef ASSIMP_API
		inline Vec3(const aiVector3D& in) : x(in.x), y(in.y), z(in.z) {}

		inline operator aiVector3D() const { return aiVector3D(x, y, z); }
#endif
	};

	class Vec4;

	class Color4
	{
	public:
		u8 r;
		u8 g;
		u8 b;
		u8 a;

		inline Color4() : r(0), g(0), b(0), a(0) {}
		inline Color4(const f32* in);
		inline Color4(const Vec4& in);
		inline Color4(u8 red, u8 green, u8 blue, u8 alpha = 0xff) : r(red), g(green), b(blue), a(alpha) {}
		inline Color4(u32 rgba) : r((rgba & 0xff000000) >> 24), g((rgba & 0x00ff0000) >> 16), b((rgba & 0x0000ff00) >> 8), a(rgba & 0x000000ff) {}

		inline Color4 operator*(const f32 a) const;
		inline Color4 operator+(const Color4& a) const;
	};

	class Vec4
	{
	public:
		f32 x;
		f32 y;
		f32 z;
		f32 w;

		// Return a new empty Vec4
		inline Vec4() : x(0), y(0), z(0), w(0) {}

		// Return a new Vec4 initialised with 'a', 'b', 'c' and 'd'
		inline Vec4(f32 a, f32 b, f32 c, f32 d = 1) : x(a), y(b), z(c), w(d) {}

		// Return a new Vec4 initialised with 'in'
		inline Vec4(const Vec3& in, f32 wIn = 1.0f) : x(in.x), y(in.y), z(in.z), w(wIn) {}

		// Return a new Vec4 initialised with 'in'
		inline Vec4(const Vec4& in) : x(in.x), y(in.y), z(in.z), w(in.w) {}

		inline Vec4(const Color4& in) : x(in.r / 255.0f), y(in.g / 255.0f), z(in.b / 255.0f), w(in.a / 255.0f) {}


		// Print the Vec4
		void print() const;
		const std::string toString() const;

		// Return the Vec3 of Vec4
		inline Vec3 GetVector() const;

		// Return the length squared
		inline f32 Dot() const;

		// Return the length
		inline f32 Length() const;

		// Divide each components by w, or set to VEC_HIGH_VALUE if w equals 0
		inline Vec4 Homogenize() const;

		inline Vec4 operator+(const Vec4& a) const;
		inline Vec4 operator+(const f32 a) const;
		inline Vec4& operator+=(const Vec4& a);
		inline Vec4& operator+=(const f32 a);

		inline Vec4 operator-(const Vec4& a) const;
		inline Vec4 operator-(const f32 a) const;
		inline Vec4& operator-=(const Vec4& a);
		inline Vec4& operator-=(const f32 a);

		inline Vec4 operator-() const;

		inline Vec4 operator*(const Vec4& a) const;
		inline Vec4 operator*(const f32 a) const;
		inline Vec4& operator*=(const Vec4& a);
		inline Vec4& operator*=(const f32 a);

		inline Vec4 operator/(const Vec4& b) const;
		inline Vec4 operator/(const f32 a) const;
		inline Vec4& operator/=(const Vec4& a);
		inline Vec4& operator/=(const f32 a);

		inline bool operator==(const Vec4& b) const;
		inline bool operator!=(const Vec4& b) const;

		inline f32& operator[](const size_t a);
		inline const f32& operator[](const size_t a) const;

		// Return tue if 'a' and 'b' are collinears (Precision defined by VEC_COLLINEAR_PRECISION)
		inline bool IsCollinearWith(Vec4 a) const;

		inline f32 Dot(Vec4 a) const;

		// Return the z component of the cross product of 'a' and 'b'
		inline Vec4 Cross(Vec4 a) const;

		// Return a vector with the same direction that 'in', but with length 1
		inline Vec4 Normalize() const;

		// Return a vector of length 'in' and with an opposite direction
		inline Vec4 Negate() const;

		inline Vec4 Clip(const Vec4& other);

		// return true if 'a' converted to s32 is equivalent to 'in' converted to s32
		inline bool IsIntEquivalent(Vec4 a) const;

		inline bool IsNearlyEqual(Vec4 a, f32 prec = 1e-5f);

		inline f32 GetSignedDistanceToPlane(const Vec3& point) const;

#ifdef IMGUI_API
		inline Vec4(const ImVec4& in) : x(in.x), y(in.y), z(in.z), w(in.w) {}

		inline operator ImVec4() const { return ImVec4(x, y, z, w); }
#endif

#ifdef JOLT_API
		inline Vec4(const JPH::Vec4Arg& in) : x(in.GetX()), y(in.GetY()), z(in.GetZ()), w(in.GetW()) {}

		inline operator JPH::Vec4Arg() const { return JPH::Vec4Arg(x, y, z, w); }
#endif
	};


	class Mat3;

	class Mat4
	{
	public:
		/* data of the matrix : content[y][x]
		 * Matrix is indexed with:
		 *
		 * 00 | 04 | 08 | 12
		 * 01 | 05 | 09 | 13
		 * 02 | 06 | 10 | 14
		 * 03 | 07 | 11 | 15
		 *
		*/
		f32 content[16] = { 0 };

		Mat4() {}

		Mat4(f32 diagonal);

		Mat4(const Mat4& in);

		Mat4(const Mat3& in);

		Mat4(const f32* data);

		Mat4 operator*(const Mat4& a) const;

		Vec4 operator*(const Vec4& a) const;

		 static Mat4 Identity();

		static Mat4 CreateTransformMatrix(const Vec3& position, const Vec3& rotation, const Vec3& scale);

		static Mat4 CreateTransformMatrix(const Vec3& position, const Vec3& rotation);

		static Mat4 CreateTransformMatrix(const Vec3& position, const Quat& rotation, const Vec3& scale);

		static Mat4 CreateTransformMatrix(const Vec3& position, const Quat& rotation);

		static Mat4 CreateTranslationMatrix(const Vec3& translation);

		static Mat4 CreateScaleMatrix(const Vec3& scale);

		static Mat4 CreateRotationMatrix(const Quat& rot);

		static Mat4 CreateRotationMatrix(Vec3 angles);

		static Mat4 CreateXRotationMatrix(f32 angle);

		static Mat4 CreateYRotationMatrix(f32 angle);

		static Mat4 CreateZRotationMatrix(f32 angle);

		// aspect ratio is width / height
		static Mat4 CreatePerspectiveProjectionMatrix(f32 near, f32 far, f32 fov, f32 aspect);

		static Mat4 CreateOrthoProjectionMatrix(f32 near, f32 far, f32 fov, f32 aspect);

		static Mat4 CreateViewMatrix(const Vec3& position, const Vec3& focus, const Vec3& up);

		static Mat4 CreateObliqueProjectionMatrix(const Mat4& projMatrix, const Vec4& nearPlane);

		Mat4 InverseDepth() const;

		Vec3 GetPositionFromTranslation() const;

		Vec3 GetRotationFromTranslation(const Vec3& scale) const;

		Vec3 GetRotationFromTranslation() const;

		Vec3 GetScaleFromTranslation() const;

		Mat4 TransposeMatrix();

		inline f32& operator[](const size_t a);

		inline const f32& operator[](const size_t a) const;

		inline f32& at(const u8 x, const u8 y);
		inline const f32& at(const u8 x, const u8 y) const;

		void PrintMatrix(bool raw = false);
		const std::string toString() const;

		Mat4 CreateInverseMatrix() const;

		Mat4 CreateAdjMatrix() const;

		Mat4 GetCofactor(s32 p, s32 q, s32 n) const;

		// Recursive function for finding determinant of matrix. n is current dimension of 'in'.
		f32 GetDeterminant(f32 n) const;

#ifdef JOLT_API
		inline Mat4(const JPH::Mat44Arg& pMat) { pMat.StoreFloat(content); }
#endif // JOLT_API

	};

	class Mat3
	{
	public:
		/* data of the matrix : content[y][x]
		 * Matrix is indexed with:
		 *
		 * 00 | 03 | 06
		 * 01 | 04 | 07
		 * 02 | 05 | 08
		 *
		*/
		f32 content[9] = { 0 };

		Mat3() {}

		Mat3(f32 diagonal);

		Mat3(const Mat3& in);

		Mat3(const Mat4& in);

		Mat3(const f32* data);

		Mat3 operator*(const Mat3& a);

		Vec3 operator*(const Vec3& a);

		static Mat3 Identity();

		static Mat3 CreateScaleMatrix(const Vec3& scale);

		//Angle is in degrees
		static Mat3 CreateXRotationMatrix(f32 angle);

		//Angle is in degrees
		static Mat3 CreateYRotationMatrix(f32 angle);

		//Angle is in degrees
		static Mat3 CreateZRotationMatrix(f32 angle);

		//Angles are in degrees
		static Mat3 CreateRotationMatrix(Vec3 angles);

		Vec3 GetRotationFromTranslation(const Vec3& scale) const;

		Vec3 GetRotationFromTranslation() const;

		Mat3 TransposeMatrix();

		inline f32& operator[](const size_t a);

		inline const f32& operator[](const size_t a) const;

		inline f32& at(const u8 x, const u8 y);

		void PrintMatrix(bool raw = false);
		const std::string toString() const;

		Mat3 CreateInverseMatrix();

		Mat3 CreateAdjMatrix();

		Mat3 GetCofactor(s32 p, s32 q, s32 n);

		// Recursive function for finding determinant of matrix. n is current dimension of 'in'.
		f32 GetDeterminant(f32 n);
	};

	class Quat
	{
	public:
		Vec3 v;
		f32 a;

		inline Quat() : v(), a(1) {}

		inline Quat(Vec3 vector, f32 real) : v(vector), a(real) {}

		inline Quat(const Mat3& in);

		inline Quat(const Mat4& in);

		// Return the length squared
		inline f32 Dot() const;

		// Return the length
		inline f32 Length() const;

		inline Quat Conjugate() const;

		inline Quat Inverse() const;

		inline Quat Normalize() const;

		inline Quat NormalizeAxis() const;

		// Makes a quaternion representing a rotation in 3d space. Angle is in radians.
		static Quat AxisAngle(Vec3 axis, f32 angle);

		// Makes a quaternion from Euler angles (angle order is YXZ)
		static Quat FromEuler(Vec3 euler);

		inline f32 GetAngle();

		inline Vec3 GetAxis();

		inline Mat3 GetRotationMatrix3() const;

		inline Mat4 GetRotationMatrix4() const;

		inline Quat operator+(const Quat& other) const;

		inline Quat operator-(const Quat& other) const;

		inline Quat operator-() const;

		inline Quat operator*(const Quat& other) const;

		inline Vec3 operator*(const Vec3& other) const;

		inline Quat operator*(const f32 scalar) const;

		inline Quat operator/(const Quat& other) const;

		inline Quat operator/(const f32 scalar) const;

		inline Vec3 GetRight() const;

		inline Vec3 GetUp() const;

		inline Vec3 GetFront() const;

		inline static Quat Slerp(const Quat& a, Quat b, f32 alpha);
#ifdef JOLT_API
		inline Quat(const JPH::QuatArg& input) : v(input.GetX(), input.GetY(), input.GetZ()), a(input.GetW()) {}

		inline operator JPH::QuatArg() const { return JPH::QuatArg(v.x, v.y, v.z, a); }
#endif
	};

	class Frustum
	{
	public:
		Frustum() {}
		~Frustum() {}

		Vec4 top;
		Vec4 bottom;
		Vec4 right;
		Vec4 left;
		Vec4 front;
		Vec4 back;
	};

	class AABB
	{
	public:
		AABB() {}
		AABB(Vec3 position, Vec3 extent) : center(position), size(extent) {}
		~AABB() {}

		Vec3 center;
		Vec3 size;

		bool IsOnFrustum(const Frustum& camFrustum, const Maths::Mat4& transform) const;
		bool IsOnOrForwardPlane(const Vec4& plane) const;
	};

	namespace Util
	{
		// Return the given angular value in degrees converted to radians
		inline f32 ToRadians(f32 in);

		// Return the given angular value in radians converted to degrees
		inline f32 ToDegrees(f32 in);

		inline f32 Clamp(f32 in, f32 min = 0.0f, f32 max = 1.0f);

		inline Vec2 Clamp(Vec2 in, f32 min = 0.0f, f32 max = 1.0f);

		inline Vec2 Clamp(IVec2 in, IVec2 min, IVec2 max);

		inline Vec3 Clamp(Vec3 in, f32 min = 0.0f, f32 max = 1.0f);

		inline Vec4 Clamp(Vec4 in, f32 min = 0.0f, f32 max = 1.0f);

		inline f32 Abs(f32 in);

		inline Vec2 Abs(Vec2 in);

		inline Vec3 Abs(Vec3 in);

		inline Vec4 Abs(Vec4 in);

		inline s32 IClamp(s32 in, s32 min, s32 max);

		inline u32 UClamp(u32 in, u32 min, u32 max);

		inline f32 Lerp(f32 a, f32 b, f32 delta);

		inline Vec3 Lerp(Vec3 a, Vec3 b, f32 delta);

		inline f32 Mod(f32 in, f32 value);

		inline Vec2 Mod(Vec2 in, f32 value);

		inline Vec3 Mod(Vec3 in, f32 value);

		inline s32 IMod(s32 in, s32 value);

		inline f32 MinF(f32 a, f32 b);

		inline f32 MaxF(f32 a, f32 b);

		inline Vec3 MinV(Vec3 a, Vec3 b);

		inline Vec3 MaxV(Vec3 a, Vec3 b);

		inline s32 MinI(s32 a, s32 b);

		inline s32 MaxI(s32 a, s32 b);

		// Smooth min function
		inline f32 SMin(f32 a, f32 b, f32 delta);

		inline bool IsNear(f32 a, f32 b, f32 prec = 0.0001f);

		// Returns a string with the hex representation of number
		// TODO Test parity with big/little endian
		inline std::string GetHex(u64 number);

		// Fills the given buffer with the hex representation of number
		// WARNING: buffer must be at least 16 char long
		// TODO Test parity with big/little endian
		inline void GetHex(char* buffer, u64 number);

		inline u64 ReadHex(const std::string& input);

		// Set of functions used to generate some shapes
		// TODO is this still relevant ?

		void GenerateSphere(s32 x, s32 y, std::vector<Vec3>* PosOut, std::vector<Vec3>* NormOut, std::vector<Vec2>* UVOut);

		void GenerateCube(std::vector<Vec3>* PosOut, std::vector<Vec3>* NormOut, std::vector<Vec2>* UVOut);

		void GenerateDome(s32 x, s32 y, bool reversed, std::vector<Vec3>* PosOut, std::vector<Vec3>* NormOut, std::vector<Vec2>* UVOut);

		void GenerateCylinder(s32 x, s32 y, std::vector<Vec3>* PosOut, std::vector<Vec3>* NormOut, std::vector<Vec2>* UVOut);

		void GeneratePlane(std::vector<Vec3>* PosOut, std::vector<Vec3>* NormOut, std::vector<Vec2>* UVOut);

		void GenerateSkyPlane(std::vector<Vec3>* PosOut, std::vector<Vec3>* NormOut, std::vector<Vec2>* UVOut);

		Vec3 GetSphericalCoord(f32 longitude, f32 latitude);
	};
}

#include "Maths.inl"