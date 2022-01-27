#pragma once

#include "renderingtypes.h"
#include "mesh.h"

struct Model {
	std::vector<Mesh> meshes;

	std::vector<Material> materials;
};