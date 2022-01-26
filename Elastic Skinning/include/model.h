#pragma once

#include "renderingtypes.h"
#include "mesh.h"

struct Model {
	std::vector<Mesh::VertexType> vertices;
	std::vector<Mesh::IndexType> indices;
	std::vector<Mesh> meshes;

	std::vector<Material> materials;
};