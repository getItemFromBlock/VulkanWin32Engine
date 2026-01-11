#pragma once

#include <vector>

#include "Maths/Maths.hpp"

namespace Resource
{
	class Video
	{
	public:
		Video();
		~Video();

		static std::vector<std::vector<u8>> ReadVideoFrames(const std::string& path);
	};
}
