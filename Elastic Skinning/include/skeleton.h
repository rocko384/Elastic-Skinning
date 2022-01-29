#pragma once

#include "util.h"
#include "animation.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>

struct Bone {
	glm::mat4 inverse_bind_matrix;
	glm::quat rotation;
	glm::vec3 position;
	glm::vec3 scale;
};

struct Skeleton {
	std::vector<Bone> bones;
	std::vector<StringHash> bone_names;

	std::vector<Animation> animations;
	std::vector<StringHash> animation_names;
};