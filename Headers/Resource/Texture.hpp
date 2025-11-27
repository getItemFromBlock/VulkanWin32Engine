#pragma once

#include "Maths/Maths.hpp"

namespace Resource
{
	class Texture
	{
	public:
		Texture();
		~Texture();

		static u8 *ReadTexture(const std::string& path, Maths::IVec2 &res);
		static void FreeTextureData(u8 *data);
	};
}
