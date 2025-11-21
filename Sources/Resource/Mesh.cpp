#include "Resource/Mesh.hpp"

using namespace Resource;
using namespace Maths;

Mesh::Mesh()
{
}

Mesh::~Mesh()
{
}

void Mesh::CreateDefaultCube()
{
	vertices.clear();

	const float cr = cosf(M_PI * 2 / 3);
	const float sr = sinf(M_PI * 2 / 3);

	vertices.push_back(Vertex(Vec3( 0.0f, -0.5f, 0.0f), Vec2(0.5f, 1.0f), Vec3(1.0f, 0.0f, 0.0f)));
	vertices.push_back(Vertex(Vec3( 0.5f * sr, -0.5f * cr, 0.0f), Vec2(1.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f)));
	vertices.push_back(Vertex(Vec3(-0.5f * sr, -0.5f * cr, 0.0f), Vec2(0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f)));
}

const std::vector<Vertex>& Resource::Mesh::GetVertices() const
{
	return vertices;
}
