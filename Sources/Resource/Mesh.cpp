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

	vertices.push_back(Vertex(Vec3( 0.0f, -0.5f, 0.0f), Vec3(1.0f, 0.0f, 0.0f)));
	vertices.push_back(Vertex(Vec3( 0.5f,  0.5f, 0.0f), Vec3(0.0f, 1.0f, 0.0f)));
	vertices.push_back(Vertex(Vec3(-0.5f,  0.5f, 0.0f), Vec3(0.0f, 0.0f, 1.0f)));
}
