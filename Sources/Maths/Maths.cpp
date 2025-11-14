#include "Maths/Maths.hpp"

#include <cstdio>

// TODO enable this if using Vulkan, or any other API that invert some axis
#define INVERTED_PROJECTION

namespace Maths
{
	// -----------------------   IVec2    -----------------------

	void IVec2::print() const
	{
		printf("(%d, %d)\n", x, y);
	}

	const std::string IVec2::toString() const
	{
		return "( " + std::to_string(x) + ", " + std::to_string(y) + ")";
	}

	// -----------------------   Vec2    -----------------------

	void Vec2::print() const
	{
		printf("(%.2f, %.2f)\n", x, y);
	}

	const std::string Vec2::toString() const
	{
		return "( " + std::to_string(x) + ", " + std::to_string(y) + ")";
	}

	// -----------------------   IVec3    -----------------------

	void IVec3::print() const
	{
		printf("(%d, %d, %d)\n", x, y, z);
	}

	const std::string IVec3::toString() const
	{
		return "( " + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
	}

	// -----------------------   Vec3    -----------------------

	void Vec3::Print() const
	{
		printf("(%.2f, %.2f, %.2f)\n", x, y, z);
	}

	const std::string Vec3::ToString() const
	{
		return "( " + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
	}

	// -----------------------   Vec4    -----------------------

	void Vec4::print() const
	{
		printf("(%.2f, %.2f, %.2f, %.2f)\n", x, y, z, w);
	}

	const std::string Vec4::toString() const
	{
		return "( " + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ", " + std::to_string(w) + ")";
	}

	// -----------------------   Mat4    -----------------------

	Mat4::Mat4(f32 diagonal)
	{
		for (size_t i = 0; i < 4; i++) content[i * 5] = diagonal;
	}

	Mat4::Mat4(const Mat4& in)
	{
		for (size_t i = 0; i < 16; i++) content[i] = in.content[i];
	}

	Mat4::Mat4(const Mat3& in)
	{
		for (size_t j = 0; j < 9; j++)
		{
			content[j + (j / 3)] = in.content[j];
		}
		content[15] = 1.0f;
	}

	Mat4::Mat4(const f32* data)
	{
		for (size_t j = 0; j < 4; j++)
		{
			for (size_t i = 0; i < 4; i++)
			{
				content[j * 4 + i] = data[j + i * 4];
			}
		}
	}

	Mat4 Mat4::operator*(const Mat4& in) const
	{
		Mat4 out;
		for (size_t j = 0; j < 4; j++)
		{
			for (size_t i = 0; i < 4; i++)
			{
				f32 res = 0;
				for (size_t k = 0; k < 4; k++)
					res += content[j + k * 4] * in.content[k + i * 4];

				out.content[j + i * 4] = res;
			}
		}
		return out;
	}

	Vec4 Mat4::operator*(const Vec4& in) const
	{
		Vec4 out;
		for (size_t i = 0; i < 4; i++)
		{
			f32 res = 0;
			for (size_t k = 0; k < 4; k++) res += content[i + k * 4] * in[k];
			out[i] = res;
		}
		return out;
	}

	Mat4 Mat4::CreateXRotationMatrix(f32 angle)
	{
		Mat4 out = Mat4(1);
		f32 radA = Util::ToRadians(angle);
		f32 cosA = cosf(radA);
		f32 sinA = sinf(radA);
		out.at(1, 1) = cosA;
		out.at(2, 1) = -sinA;
		out.at(1, 2) = sinA;
		out.at(2, 2) = cosA;
		return out;
	}

	Mat4 Mat4::CreateYRotationMatrix(f32 angle)
	{
		Mat4 out = Mat4(1);
		f32 radA = Util::ToRadians(angle);
		f32 cosA = cosf(radA);
		f32 sinA = sinf(radA);
		out.at(0, 0) = cosA;
		out.at(2, 0) = sinA;
		out.at(0, 2) = -sinA;
		out.at(2, 2) = cosA;
		return out;
	}

	Mat4 Mat4::CreateZRotationMatrix(f32 angle)
	{
		Mat4 out = Mat4(1);
		f32 radA = Util::ToRadians(angle);
		f32 cosA = cosf(radA);
		f32 sinA = sinf(radA);
		out.at(0, 0) = cosA;
		out.at(1, 0) = -sinA;
		out.at(0, 1) = sinA;
		out.at(1, 1) = cosA;
		return out;
	}

	Mat4 Mat4::CreateScaleMatrix(const Vec3& scale)
	{
		Mat4 out;
		for (s32 i = 0; i < 3; i++) out.at(i, i) = scale[i];
		out.content[15] = 1;
		return out;
	}

	Mat4 Mat4::CreateTranslationMatrix(const Vec3& translation)
	{
		Mat4 out = Mat4(1);
		for (s32 i = 0; i < 3; i++) out.at(3, i) = translation[i];
		return out;
	}

	Mat4 Mat4::CreateTransformMatrix(const Vec3& position, const Quat& rotation, const Vec3& scale)
	{
		return CreateTranslationMatrix(position) * rotation.GetRotationMatrix4() * CreateScaleMatrix(scale);
	}

	Mat4 Mat4::CreateTransformMatrix(const Vec3& position, const Quat& rotation)
	{
		return CreateTranslationMatrix(position) * rotation.GetRotationMatrix4();
	}

	Mat4 Mat4::CreateTransformMatrix(const Vec3& position, const Vec3& rotation, const Vec3& scale)
	{
		return CreateTranslationMatrix(position) * CreateRotationMatrix(rotation) * CreateScaleMatrix(scale);
	}

	Mat4 Mat4::CreateTransformMatrix(const Vec3& position, const Vec3& rotation)
	{
		return CreateTranslationMatrix(position) * CreateRotationMatrix(rotation);
	}

	Mat4 Maths::Mat4::CreateRotationMatrix(const Quat& rot)
	{
		f32 xy = rot.v.x * rot.v.y;
		f32 xz = rot.v.x * rot.v.z;
		f32 yz = rot.v.y * rot.v.z;

		f32 xw = rot.v.x * rot.a;
		f32 yw = rot.v.y * rot.a;
		f32 zw = rot.v.z * rot.a;

		Mat4 out = Mat4(1);

		out.at(0, 0) = 1 - 2 * (rot.v.y * rot.v.y) - 2 * (rot.v.z * rot.v.z);
		out.at(1, 0) = 2 * xy - 2 * zw;
		out.at(2, 0) = 2 * xz + 2 * yw;

		out.at(0, 1) = 2 * xy + 2 * zw;
		out.at(1, 1) = 1 - 2 * (rot.v.x * rot.v.x) - 2 * (rot.v.z * rot.v.z);
		out.at(2, 1) = 2 * yz - 2 * xw;

		out.at(0, 2) = 2 * xz - 2 * yw;
		out.at(1, 2) = 2 * yz - 2 * xw;
		out.at(2, 2) = 1 - 2 * (rot.v.x * rot.v.x) - 2 * (rot.v.y * rot.v.y);

		return out;
	}

	Mat4 Maths::Mat4::CreateRotationMatrix(Vec3 angles)
	{
		return CreateYRotationMatrix(angles.y) * CreateXRotationMatrix(angles.x) * CreateZRotationMatrix(angles.z);
	}

	Mat4 Mat4::CreatePerspectiveProjectionMatrix(f32 near, f32 far, f32 fov, f32 ratio)
	{
		f32 s = 1.0f / tanf(Util::ToRadians(fov / 2.0f));
		f32 param1 = -(far + near) / (far - near);
		f32 param2 = -(2 * near * far) / (far - near);
		Mat4 out;
		out.at(0, 0) = s / ratio;
#ifdef INVERTED_PROJECTION
		out.at(1, 1) = -s; // Update for Vulkan (because Y axis is inverted from OpenGL)
#else
		out.at(1, 1) = s;
#endif
		out.at(2, 2) = param1;
		out.at(2, 3) = -1;
		out.at(3, 2) = param2;
		return out;
	}

	Mat4 Mat4::CreateOrthoProjectionMatrix(f32 near, f32 far, f32 fov, f32 aspect)
	{
		float s = 1.0f / fov;
		float param1 = -2 / (far - near);
		float param2 = -(far + near) / (far - near);
		Mat4 out;
		out.at(0, 0) = s / aspect;
#ifdef INVERTED_PROJECTION
		out.at(1, 1) = -s; // Update for Vulkan (because Y axis is inverted from OpenGL)
#else
		out.at(1, 1) = s;
#endif
		out.at(2, 2) = param1;
		out.at(3, 3) = 1;
		out.at(3, 2) = param2;
		return out;
	}

	Mat4 Mat4::CreateViewMatrix(const Vec3& position, const Vec3& focus, const Vec3& up)
	{
		Mat4 temp;
		Vec3 z = (position - focus).Normalize();
		Vec3 x = up.Cross(z).Normalize();
		Vec3 y = z.Cross(x);
		Vec3 delta = Vec3(-x.Dot(position), -y.Dot(position), -z.Dot(position));
		for (int i = 0; i < 3; i++)
		{
			temp.at(i, 0) = x[i];
			temp.at(i, 1) = y[i];
			temp.at(i, 2) = z[i];
			temp.at(3, i) = delta[i];
		}
		temp.at(3, 3) = 1;
		return temp;
	}

	Mat4 Mat4::CreateObliqueProjectionMatrix(const Mat4& projMatrix, const Vec4& c)
	{
		Mat4 result = projMatrix;
		Mat4 inverse = projMatrix.CreateInverseMatrix();
		Vec4 qs = Vec4(-copysignf(1.0f, c.x), copysignf(1.0f, c.y), -1.0f, 1.0f);
		Vec4 q = inverse * qs;
		Vec4 vec = c * ((-2.0f * q.z) / c.Dot(q));
		vec.z = vec.z - 1;
		for (u8 i = 0; i < 4; i++)
		{
			result.at(i, 2) = vec[i];
		}
		return result.InverseDepth();
	}

	Mat4 Mat4::TransposeMatrix()
	{
		f32 x[16] = { 0 };
		for (s32 j = 0; j < 4; j++)
		{
			for (s32 i = 0; i < 4; i++)
			{
				x[i + j * 4] = content[i + j * 4];
			}
		}

		return Mat4{ x };
	}

	void Mat4::PrintMatrix(bool raw)
	{
		if (raw)
		{
			printf("[ ");
			for (s32 j = 0; j < 16; j++)
			{
				printf("%.2f", content[j]);
				if (j != 15) printf(", ");
			}
			printf("]\n");
		}
		else
		{
			for (s32 j = 0; j < 4; j++)
			{
				for (s32 i = 0; i < 4; i++)
				{
					printf("% 8.2f ", content[j + i * 4]);
				}
				printf("\n");
			}
		}
		printf("\n");
	}

	const std::string Mat4::toString() const
	{
		std::string res = "( ";
		for (s32 i = 0; i < 16; i++)
		{
			res += std::to_string(content[(i * 4) % 16 + i / 4]);
			if (i != 15) res.append(", ");
		}
		res.append(")");
		return res;
	}

	Mat4 Mat4::Identity()
	{
		return Mat4(1);
	}

	Mat4 Mat4::GetCofactor(s32 p, s32 q, s32 n) const
	{
		Mat4 mat;
		s32 i = 0, j = 0;
		// Looping for each element of the matrix
		for (s32 row = 0; row < n; row++)
		{
			for (s32 col = 0; col < n; col++)
			{
				//  Copying into temporary matrix only those element
				//  which are not in given row and column
				if (row != p && col != q)
				{
					mat.content[i + j * 4] = content[row + col * 4];
					j++;

					// Row is filled, so increase row index and
					// reset col index
					if (j == n - 1)
					{
						j = 0;
						i++;
					}
				}
			}
		}
		return mat;
	}

	f32 Mat4::GetDeterminant(f32 n) const
	{
		Mat4 a;
		f32 D = 0; // Initialize result

		//  Base case : if matrix contains single element
		if (n == 1)
			return content[0];

		char sign = 1;  // To store sign multiplier

		// Iterate for each element of first row
		for (s32 f = 0; f < n; f++)
		{
			// Getting Cofactor of matrix[0][f]
			a = GetCofactor(0, f, (int)n);
			D += sign * content[f * 4] * a.GetDeterminant(n - 1);

			// terms are to be added with alternate sign
			sign = -sign;
		}

		return D;
	}

	Mat4 Mat4::CreateInverseMatrix() const
	{
		// Find determinant of matrix
		Mat4 inverse;
		f32 det = GetDeterminant(4);
		if (det == 0)
		{
			//printf("Singular matrix, can't find its inverse\n");
			return Mat4();
		}

		// Find adjoint
		Mat4 adj = CreateAdjMatrix();

		// Find Inverse using formula "inverse(A) = adj(A)/det(A)"
		for (s32 i = 0; i < 4; i++)
			for (s32 j = 0; j < 4; j++)
				inverse.content[i + j * 4] = adj.content[i + j * 4] / det;

		return inverse;
	}

	Mat4 Mat4::CreateAdjMatrix() const
	{
		// temp is used to store cofactors of matrix
		Mat4 temp;
		Mat4 adj;
		char sign = 1;

		for (s32 i = 0; i < 4; i++)
		{
			for (s32 j = 0; j < 4; j++)
			{
				// Get cofactor of matrix[i][j]
				temp = GetCofactor(i, j, 4);

				// sign of adj positive if sum of row
				// and column indexes is even.
				sign = ((i + j) % 2 == 0) ? 1 : -1;

				// Interchanging rows and columns to get the
				// transpose of the cofactor matrix
				adj.content[j + i * 4] = (sign) * (temp.GetDeterminant(3));
			}
		}
		return adj;
	}

	Mat3::Mat3(f32 diagonal)
	{
		for (size_t i = 0; i < 3; i++) content[i * 4] = diagonal;
	}

	Mat3::Mat3(const Mat3& in)
	{
		for (size_t j = 0; j < 9; j++)
		{
			content[j] = in.content[j];
		}
	}

	Mat3::Mat3(const Mat4& in)
	{
		size_t index = 0;
		for (size_t j = 0; j < 11; j++)
		{
			if (j == 3 || j == 7) continue;
			content[index] = in.content[j];
			index++;
		}
	}

	Mat3::Mat3(const f32* data)
	{
		for (size_t j = 0; j < 3; j++)
		{
			for (size_t i = 0; i < 3; i++)
			{
				content[j * 3 + i] = data[j + i * 3];
			}
		}
	}

	Mat3 Mat3::operator*(const Mat3& in)
	{
		Mat3 out;
		for (size_t j = 0; j < 3; j++)
		{
			for (size_t i = 0; i < 3; i++)
			{
				f32 res = 0;
				for (size_t k = 0; k < 3; k++)
					res += content[j + k * 3] * in.content[k + i * 3];

				out.content[j + i * 3] = res;
			}
		}
		return out;
	}

	Vec3 Mat3::operator*(const Vec3& in)
	{
		Vec3 out;
		for (size_t i = 0; i < 3; i++)
		{
			f32 res = 0;
			for (size_t k = 0; k < 3; k++) res += content[i + k * 3] * in[k];
			out[i] = res;
		}
		return out;
	}

	Mat3 Mat3::CreateXRotationMatrix(f32 angle)
	{
		Mat3 out = Mat3(1);
		f32 radA = Util::ToRadians(angle);
		f32 cosA = cosf(radA);
		f32 sinA = sinf(radA);
		out.at(1, 1) = cosA;
		out.at(2, 1) = -sinA;
		out.at(1, 2) = sinA;
		out.at(2, 2) = cosA;
		return out;
	}

	Mat3 Mat3::CreateYRotationMatrix(f32 angle)
	{
		Mat3 out = Mat3(1);
		f32 radA = Util::ToRadians(angle);
		f32 cosA = cosf(radA);
		f32 sinA = sinf(radA);
		out.at(0, 0) = cosA;
		out.at(2, 0) = sinA;
		out.at(0, 2) = -sinA;
		out.at(2, 2) = cosA;
		return out;
	}

	Mat3 Mat3::CreateZRotationMatrix(f32 angle)
	{
		Mat3 out = Mat3(1);
		f32 radA = Util::ToRadians(angle);
		f32 cosA = cosf(radA);
		f32 sinA = sinf(radA);
		out.at(0, 0) = cosA;
		out.at(1, 0) = -sinA;
		out.at(0, 1) = sinA;
		out.at(1, 1) = cosA;
		return out;
	}

	Mat3 Maths::Mat3::CreateRotationMatrix(Vec3 angles)
	{
		return CreateYRotationMatrix(angles.y) * CreateXRotationMatrix(angles.x) * CreateZRotationMatrix(angles.z);
	}

	Vec3 Mat3::GetRotationFromTranslation(const Vec3& scale) const
	{
		f32 thetaX;
		f32 thetaY;
		f32 thetaZ;
		if (Util::MinF(fabsf(scale.x), Util::MinF(fabsf(scale.y), fabsf(scale.z))) < 0.0001f) return Vec3();
		f32 a = content[7] / scale.z;
		if (a < 0.9999f)
		{
			if (a > -0.9999f)
			{
				thetaX = asinf(-a);
				thetaY = atan2f(content[6] / scale.z, content[8] / scale.z);
				thetaZ = atan2f(content[1] / scale.x, content[4] / scale.y);
			}
			else
			{
				thetaX = (f32)M_PI_2;
				thetaY = -atan2f(-content[3] / scale.y, content[0] / scale.x);
				thetaZ = 0;
			}
		}
		else
		{
			thetaX = -(f32)M_PI_2;
			thetaY = atan2f(-content[3] / scale.y, content[0] / scale.x);
			thetaZ = 0;
		}
		return Vec3(Util::ToDegrees(thetaX), Util::ToDegrees(thetaY), Util::ToDegrees(thetaZ));
	}

	Vec3 Mat3::GetRotationFromTranslation() const
	{
		f32 thetaX;
		f32 thetaY;
		f32 thetaZ;
		f32 a = content[7];
		if (a < 0.9999999f)
		{
			if (a > -0.9999999f)
			{
				thetaX = asinf(-a);
				thetaY = atan2f(content[6], content[8]);
				thetaZ = atan2f(content[1], content[4]);
			}
			else
			{
				thetaX = (f32)M_PI_2;
				thetaY = -atan2f(-content[3], content[0]);
				thetaZ = 0;
			}
		}
		else
		{
			thetaX = -(f32)M_PI_2;
			thetaY = atan2f(-content[3], content[0]);
			thetaZ = 0;
		}
		return Vec3(Util::ToDegrees(thetaX), Util::ToDegrees(thetaY), Util::ToDegrees(thetaZ));
	}

	Mat3 Mat3::CreateScaleMatrix(const Vec3& scale)
	{
		Mat3 out;
		for (s32 i = 0; i < 3; i++) out.at(i, i) = scale[i];
		return out;
	}

	Mat3 Mat3::TransposeMatrix()
	{
		f32 x[9] = { 0 };
		for (s32 j = 0; j < 3; j++)
		{
			for (s32 i = 0; i < 3; i++)
			{
				x[i + j * 3] = content[i + j * 3];
			}
		}

		return Mat3{ x };
	}

	void Mat3::PrintMatrix(bool raw)
	{
		if (raw)
		{
			printf("[ ");
			for (s32 j = 0; j < 9; j++)
			{
				printf("%.2f", content[j]);
				if (j != 8) printf(", ");
			}
			printf("]\n");
		}
		else
		{
			for (s32 j = 0; j < 3; j++)
			{
				for (s32 i = 0; i < 3; i++)
				{
					printf("% 8.2f ", content[j + i * 3]);
				}
				printf("\n");
			}
		}
		printf("\n");
	}

	const std::string Mat3::toString() const
	{
		std::string res = "( ";
		for (s32 i = 0; i < 9; i++)
		{
			res += std::to_string(content[(i * 3) % 9 + i / 3]);
			if (i != 8) res.append(", ");
		}
		res.append(")");
		return res;
	}

	Mat3 Mat3::Identity()
	{
		return Mat3(1);
	}

	Mat3 Mat3::GetCofactor(s32 p, s32 q, s32 n)
	{
		Mat3 mat;
		s32 i = 0, j = 0;
		// Looping for each element of the matrix
		for (s32 row = 0; row < n; row++)
		{
			for (s32 col = 0; col < n; col++)
			{
				//  Copying into temporary matrix only those element
				//  which are not in given row and column
				if (row != p && col != q)
				{
					mat.content[i + j * 3] = content[row + col * 3];
					j++;

					// Row is filled, so increase row index and
					// reset col index
					if (j == n - 1)
					{
						j = 0;
						i++;
					}
				}
			}
		}
		return mat;
	}

	f32 Mat3::GetDeterminant(f32 n)
	{
		Mat3 a;
		f32 D = 0; // Initialize result

		//  Base case : if matrix contains single element
		if (n == 1)
			return content[0];

		char sign = 1;  // To store sign multiplier

		// Iterate for each element of first row
		for (s32 f = 0; f < n; f++)
		{
			// Getting Cofactor of matrix[0][f]
			a = GetCofactor(0, f, (int)n);
			D += sign * content[f * 3] * a.GetDeterminant(n - 1);

			// terms are to be added with alternate sign
			sign = -sign;
		}

		return D;
	}

	Mat3 Mat3::CreateInverseMatrix()
	{
		// Find determinant of matrix
		Mat3 inverse;
		f32 det = GetDeterminant(3);
		if (det == 0)
		{
			printf("Singular matrix, can't find its inverse\n");
			return 1;
		}

		// Find adjoint
		Mat3 adj = CreateAdjMatrix();

		// Find Inverse using formula "inverse(A) = adj(A)/det(A)"
		for (s32 i = 0; i < 3; i++)
			for (s32 j = 0; j < 3; j++)
				inverse.content[i + j * 3] = adj.content[i + j * 3] / det;

		return inverse;
	}

	Mat3 Mat3::CreateAdjMatrix()
	{
		// temp is used to store cofactors of matrix
		Mat3 temp;
		Mat3 adj;
		char sign = 1;

		for (s32 i = 0; i < 3; i++)
		{
			for (s32 j = 0; j < 3; j++)
			{
				// Get cofactor of matrix[i][j]
				temp = GetCofactor(i, j, 3);

				// sign of adj positive if sum of row
				// and column indexes is even.
				sign = ((i + j) % 2 == 0) ? 1 : -1;

				// Interchanging rows and columns to get the
				// transpose of the cofactor matrix
				adj.content[j + i * 3] = (sign) * (temp.GetDeterminant(2));
			}
		}
		return adj;
	}

	Mat4 Maths::Mat4::InverseDepth() const
	{
		Mat4 result = Mat4(*this);
		for (u8 i = 0; i < 4; i++)
		{
			result.at(i, 2) = at(i, 3) - at(i, 2);
		}
		return result;
	}

	Vec3 Maths::Mat4::GetPositionFromTranslation() const
	{
		return Vec3(content[12], content[13], content[14]);
	}

	Vec3 Maths::Mat4::GetRotationFromTranslation(const Vec3& scale) const
	{
		f32 thetaX;
		f32 thetaY;
		f32 thetaZ;
		if (Util::MinF(fabsf(scale.x), Util::MinF(fabsf(scale.y), fabsf(scale.z))) < 0.0001f) return Vec3();
		f32 a = content[9] / scale.z;
		if (a < 0.9999f)
		{
			if (a > -0.9999f)
			{
				thetaX = asinf(-a);
				thetaY = atan2f(content[8] / scale.z, content[10] / scale.z);
				thetaZ = atan2f(content[1] / scale.x, content[5] / scale.y);
			}
			else
			{
				thetaX = (f32)M_PI_2;
				thetaY = -atan2f(-content[4] / scale.y, content[0] / scale.x);
				thetaZ = 0;
			}
		}
		else
		{
			thetaX = -(f32)M_PI_2;
			thetaY = atan2f(-content[4] / scale.y, content[0] / scale.x);
			thetaZ = 0;
		}
		return Vec3(Util::ToDegrees(thetaX), Util::ToDegrees(thetaY), Util::ToDegrees(thetaZ));
	}

	Vec3 Mat4::GetRotationFromTranslation() const
	{
		f32 thetaX;
		f32 thetaY;
		f32 thetaZ;
		f32 a = content[9];
		if (a < 0.9999f)
		{
			if (a > -0.9999f)
			{
				thetaX = asinf(-a);
				thetaY = atan2f(content[8], content[10]);
				thetaZ = atan2f(content[1], content[5]);
			}
			else
			{
				thetaX = (f32)M_PI_2;
				thetaY = -atan2f(-content[4], content[0]);
				thetaZ = 0;
			}
		}
		else
		{
			thetaX = -(f32)M_PI_2;
			thetaY = atan2f(-content[4], content[0]);
			thetaZ = 0;
		}
		return Vec3(Util::ToDegrees(thetaX), Util::ToDegrees(thetaY), Util::ToDegrees(thetaZ));
	}

	Vec3 Maths::Mat4::GetScaleFromTranslation() const
	{
		Vec3 x = Vec3(content[0], content[1], content[2]);
		Vec3 y = Vec3(content[4], content[5], content[6]);
		Vec3 z = Vec3(content[8], content[9], content[10]);
		return Vec3(x.Length(), y.Length(), z.Length());
	}

	void Maths::Util::GenerateSphere(s32 x, s32 y, std::vector<Vec3>* PosOut, std::vector<Vec3>* NormOut, std::vector<Vec2>* UVOut)
	{
		f32 DtY = 180.0f / y;
		f32 DtX = 360.0f / x;
		for (s32 b = 1; b < y + 1; b++)
		{
			for (s32 a = 0; a < x; a++)
			{
				if (b != y)
				{
					PosOut->push_back(GetSphericalCoord(DtX * a, DtY * b - 90));
					NormOut->push_back(PosOut->back());
					UVOut->push_back(Vec2(0, 1));
					PosOut->push_back(GetSphericalCoord(DtX * (a + 1), DtY * b - 90));
					NormOut->push_back(PosOut->back());
					UVOut->push_back(Vec2(1, 1));
					PosOut->push_back(GetSphericalCoord(DtX * (a + 1), DtY * (b - 1) - 90));
					NormOut->push_back(PosOut->back());
					UVOut->push_back(Vec2(1, 0));
				}
				if (b == 1) continue;
				PosOut->push_back(GetSphericalCoord(DtX * a, DtY * (b - 1) - 90));
				NormOut->push_back(PosOut->back());
				UVOut->push_back(Vec2(0, 0));
				PosOut->push_back(GetSphericalCoord(DtX * a, DtY * b - 90));
				NormOut->push_back(PosOut->back());
				UVOut->push_back(Vec2(0, 1));
				PosOut->push_back(GetSphericalCoord(DtX * (a + 1), DtY * (b - 1) - 90));
				NormOut->push_back(PosOut->back());
				UVOut->push_back(Vec2(1, 0));
			}
		}
	}

	void Maths::Util::GenerateCube(std::vector<Vec3>* PosOut, std::vector<Vec3>* NormOut, std::vector<Vec2>* UVOut)
	{
		f32 sign = 1.0f;
		Vec3 V[4];
		Vec3 N;
		Vec2 UV[4];
		for (char i = 0; i < 6; i++)
		{
			if (i == 3) sign = -sign;
			f32 A = 1.0f;
			f32 B = 1.0f;
			for (char j = 0; j < 4; j++)
			{
				V[static_cast<u32>(j)][i % 3] = sign / 2;
				V[static_cast<u32>(j)][(i + 1 + (i < 3)) % 3] = A / 2;
				V[static_cast<u32>(j)][(i + 2 - (i < 3)) % 3] = B / 2;
				N[i % 3] = sign;
				N[(i + 1) % 3] = 0;
				N[(i + 2) % 3] = 0;
				UV[static_cast<u32>(j)][i % 2] = A < 0.0f ? 0.0f : 1.0f;
				UV[static_cast<u32>(j)][(i + 1) % 2] = B * sign < 0.0f ? 0.0f : 1.0f;
				A = -A;
				std::swap(A, B);
			}
			for (char j = 0; j < 2; j++)
			{
				PosOut->push_back(V[0]);
				PosOut->push_back(V[1 + j]);
				PosOut->push_back(V[2 + j]);
				for (char k = 0; k < 3; k++) NormOut->push_back(N);
				UVOut->push_back(UV[0]);
				UVOut->push_back(UV[1 + j]);
				UVOut->push_back(UV[2 + j]);
			}
		}
	}

	void Util::GenerateDome(s32 x, s32 y, bool reversed, std::vector<Vec3>* PosOut, std::vector<Vec3>* NormOut, std::vector<Vec2>* UVOut)
	{
		f32 DtY = 180.0f / y;
		f32 DtX = 360.0f / x;
		for (s32 b = (reversed ? 1 : y / 2 + 1); b < (reversed ? y / 2 : y) + 1; b++)
		{
			for (s32 a = 0; a < x; a++)
			{
				if (b != y)
				{
					PosOut->push_back(GetSphericalCoord(DtX * a, DtY * b - 90));
					NormOut->push_back(PosOut->back());
					UVOut->push_back(Vec2(0, 1));
					PosOut->push_back(GetSphericalCoord(DtX * (a + 1), DtY * b - 90));
					NormOut->push_back(PosOut->back());
					UVOut->push_back(Vec2(1, 1));
					PosOut->push_back(GetSphericalCoord(DtX * (a + 1), DtY * (b - 1) - 90));
					NormOut->push_back(PosOut->back());
					UVOut->push_back(Vec2(1, 0));
				}
				if (b == 1) continue;
				PosOut->push_back(GetSphericalCoord(DtX * a, DtY * (b - 1) - 90));
				NormOut->push_back(PosOut->back());
				UVOut->push_back(Vec2(0, 0));
				PosOut->push_back(GetSphericalCoord(DtX * a, DtY * b - 90));
				NormOut->push_back(PosOut->back());
				UVOut->push_back(Vec2(0, 1));
				PosOut->push_back(GetSphericalCoord(DtX * (a + 1), DtY * (b - 1) - 90));
				NormOut->push_back(PosOut->back());
				UVOut->push_back(Vec2(1, 0));
			}
		}
	}

	void Util::GenerateCylinder(s32 x, s32 y, std::vector<Vec3>* PosOut, std::vector<Vec3>* NormOut, std::vector<Vec2>* UVOut)
	{
		f32 DtY = 2.0f / y;
		f32 DtX = 360.0f / x;
		for (s32 b = 0; b < y; b++)
		{
			for (s32 a = 0; a < x; a++)
			{
				NormOut->push_back(GetSphericalCoord(DtX * a, 0));
				PosOut->push_back(NormOut->back() + Vec3(0, DtY * (b + 1) - 1, 0));
				UVOut->push_back(Vec2(0, 1));
				NormOut->push_back(GetSphericalCoord(DtX * (a + 1), 0));
				PosOut->push_back(NormOut->back() + Vec3(0, DtY * (b + 1) - 1, 0));
				UVOut->push_back(Vec2(1, 1));
				NormOut->push_back(GetSphericalCoord(DtX * (a + 1), 0));
				PosOut->push_back(NormOut->back() + Vec3(0, DtY * b - 1, 0));
				UVOut->push_back(Vec2(1, 0));
				NormOut->push_back(GetSphericalCoord(DtX * a, 0));
				PosOut->push_back(NormOut->back() + Vec3(0, DtY * b - 1, 0));
				UVOut->push_back(Vec2(0, 0));
				NormOut->push_back(GetSphericalCoord(DtX * a, 0));
				PosOut->push_back(NormOut->back() + Vec3(0, DtY * (b + 1) - 1, 0));
				UVOut->push_back(Vec2(0, 1));
				NormOut->push_back(GetSphericalCoord(DtX * (a + 1), 0));
				PosOut->push_back(NormOut->back() + Vec3(0, DtY * b - 1, 0));
				UVOut->push_back(Vec2(1, 0));
			}
		}
	}

	void Util::GeneratePlane(std::vector<Vec3>* PosOut, std::vector<Vec3>* NormOut, std::vector<Vec2>* UVOut)
	{
		PosOut->push_back(Vec3(-1, 1, 0));
		NormOut->push_back(Vec3(0, 0, 1));
		UVOut->push_back(Vec2(0, 0));
		PosOut->push_back(Vec3(-1, -1, 0));
		NormOut->push_back(Vec3(0, 0, 1));
		UVOut->push_back(Vec2(0, 1));
		PosOut->push_back(Vec3(1, 1, 0));
		NormOut->push_back(Vec3(0, 0, 1));
		UVOut->push_back(Vec2(1, 0));
		PosOut->push_back(Vec3(1, 1, 0));
		NormOut->push_back(Vec3(0, 0, 1));
		UVOut->push_back(Vec2(1, 0));
		PosOut->push_back(Vec3(-1, -1, 0));
		NormOut->push_back(Vec3(0, 0, 1));
		UVOut->push_back(Vec2(0, 1));
		PosOut->push_back(Vec3(1, -1, 0));
		NormOut->push_back(Vec3(0, 0, 1));
		UVOut->push_back(Vec2(1, 1));
	}

	void Util::GenerateSkyPlane(std::vector<Vec3>* PosOut, std::vector<Vec3>* NormOut, std::vector<Vec2>* UVOut)
	{
		PosOut->push_back(Vec3(-1, 1, 1.0f));
		NormOut->push_back(Vec3(0, 0, -1));
		UVOut->push_back(Vec2(0, 0));

		PosOut->push_back(Vec3(1, -1, 1.0f));
		NormOut->push_back(Vec3(0, 0, -1));
		UVOut->push_back(Vec2(1, 1));

		PosOut->push_back(Vec3(1, 1, 1.0f));
		NormOut->push_back(Vec3(0, 0, -1));
		UVOut->push_back(Vec2(1, 0));

		PosOut->push_back(Vec3(-1, 1, 1.0f));
		NormOut->push_back(Vec3(0, 0, -1));
		UVOut->push_back(Vec2(0, 0));

		PosOut->push_back(Vec3(-1, -1, 1.0f));
		NormOut->push_back(Vec3(0, 0, -1));
		UVOut->push_back(Vec2(0, 1));

		PosOut->push_back(Vec3(1, -1, 1.0f));
		NormOut->push_back(Vec3(0, 0, -1));
		UVOut->push_back(Vec2(1, 1));
	}

	Vec3 Maths::Util::GetSphericalCoord(f32 longitude, f32 latitude)
	{
		longitude = ToRadians(longitude);
		latitude = ToRadians(latitude);
		return Vec3(cosf(longitude) * cosf(latitude), sinf(latitude), sinf(longitude) * cosf(latitude));
	}

	bool AABB::IsOnFrustum(const Frustum& camFrustum, const Maths::Mat4& transform) const
	{
		Vec3 globalCenter = (transform * Vec4(center)).GetVector();
		Maths::Mat3 rot = transform;
		Vec3 right = (rot * Vec3(1, 0, 0)) * size.x * 2.0f;
		Vec3 up = (rot * Vec3(0, 1, 0)) * size.y * 2.0f;
		Vec3 forward = (rot * Vec3(0, 0, 1)) * size.z * 2.0f;
		Vec3 newExtent;
		for (u8 i = 0; i < 3; ++i)
		{
			Vec3 axis;
			axis[i] = 1;
			newExtent[i] = std::abs(axis.Dot(right)) +
				std::abs(axis.Dot(up)) +
				std::abs(axis.Dot(forward));
		}
		/*
		f32 newIi = std::abs(Vec3(1, 0, 0).Dot(right)) +
			std::abs(Vec3(1, 0, 0).Dot(up)) +
			std::abs(Vec3(1, 0, 0).Dot(forward));

		f32 newIj = std::abs(Vec3(0, 1, 0).Dot(right)) +
			std::abs(Vec3(0, 1, 0).Dot(up)) +
			std::abs(Vec3(0, 1, 0).Dot(forward));

		f32 newIk = std::abs(Vec3(0, 0, 1).Dot(right)) +
			std::abs(Vec3(0, 0, 1).Dot(up)) +
			std::abs(Vec3(0, 0, 1).Dot(forward));
			*/
		const AABB globalAABB(globalCenter, newExtent);

		return (globalAABB.IsOnOrForwardPlane(camFrustum.left) &&
			globalAABB.IsOnOrForwardPlane(camFrustum.right) &&
			globalAABB.IsOnOrForwardPlane(camFrustum.top) &&
			globalAABB.IsOnOrForwardPlane(camFrustum.bottom) &&
			globalAABB.IsOnOrForwardPlane(camFrustum.front) &&
			globalAABB.IsOnOrForwardPlane(camFrustum.back));
	}

	bool AABB::IsOnOrForwardPlane(const Vec4& plane) const
	{
		const float r = size.x * std::abs(plane.x) +
			size.y * std::abs(plane.y) + size.z * std::abs(plane.z);

		return -r <= plane.GetSignedDistanceToPlane(center);
	}
}