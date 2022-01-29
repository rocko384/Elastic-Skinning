#pragma once

#include "renderingtypes.h"
#include "mesh.h"
#include "skeleton.h"

struct Model {
	std::vector<Mesh> meshes;
	std::vector<Material> materials;
	Skeleton skeleton;
};