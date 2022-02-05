#pragma once

#include "renderingtypes.h"
#include "mesh.h"
#include "skeleton.h"

#include <variant>

struct Model {
	std::vector<std::variant<Mesh, SkeletalMesh>> meshes;
	std::vector<Material> materials;
	Skeleton skeleton;
};