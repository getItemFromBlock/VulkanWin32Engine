#pragma once

#include "Maths/Maths.hpp"

namespace Resource
{
	struct Vertex
	{
		Maths::Vec3 pos;
		Maths::Vec2 uv;
		Maths::Vec3 col;

		Vertex(Maths::Vec3 position, Maths::Vec2 texCoords, Maths::Vec3 color) : pos(position), uv(texCoords), col(color) {}
		Vertex() {}
	};

	class Mesh
	{
	public:
		Mesh();
		~Mesh();

		void CreateDefaultCube();
		const std::vector<Vertex>& GetVertices() const;

	private:
		std::vector<Vertex> vertices;
	};
}