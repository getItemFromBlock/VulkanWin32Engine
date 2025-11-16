#pragma once

#include "Maths/Maths.hpp"

namespace Resource
{
	struct Vertex
	{
		Maths::Vec3 pos;
		Maths::Vec3 col;

		Vertex(Maths::Vec3 position, Maths::Vec3 color) : pos(position), col(color) {}
		Vertex() {}
	};

	class Mesh
	{
	public:
		Mesh();
		~Mesh();

		void CreateDefaultCube();

	private:
		std::vector<Vertex> vertices;
	};
}