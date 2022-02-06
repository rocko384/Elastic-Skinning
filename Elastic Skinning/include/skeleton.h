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
	enum class Error {
		OK,
		BONE_NOT_FOUND,
		ANIMATION_NOT_FOUND
	};

	void add_bone(const Bone& bone, const std::string& name);
	void add_bone(const Bone& bone, StringHash name);
	Retval<Bone*, Error> get_bone(const std::string& name);
	Retval<Bone*, Error> get_bone(StringHash name);
	
	void add_animation(const Animation& animation, const std::string& name);
	void add_animation(const Animation& animation, StringHash name);
	Retval<Animation*, Error> get_animation(const std::string& name);
	Retval<Animation*, Error> get_animation(StringHash name);

	Error play_animation(const std::string& name);
	Error play_animation(StringHash name);
	void clear_animation();

	std::vector<Bone> sample_animation_frame();

	std::vector<Bone> bones;
	std::vector<StringHash> bone_names;

	std::vector<Animation> animations;
	std::vector<StringHash> animation_names;

	Animation* active_animation{ nullptr };
	std::chrono::steady_clock::time_point animation_start_time;
};