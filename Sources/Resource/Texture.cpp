#include "Resource/Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace Resource;
using namespace Maths;

Texture::Texture()
{
}

Texture::~Texture()
{
}

u8 *Texture::ReadTexture(const std::string &path, IVec2 &res)
{
	s32 comp;
	u8* data = stbi_load(path.c_str(), &res.x, &res.y, &comp, 4);
	return data;
}

void Resource::Texture::FreeTextureData(u8 *data)
{
	stbi_image_free(data);
}
