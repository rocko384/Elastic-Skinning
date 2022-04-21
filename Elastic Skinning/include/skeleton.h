#pragma once

#include "util.h"
#include "animation.h"

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>

struct Bone {
	alignas(16) glm::mat4 bind_matrix;
	alignas(16) glm::mat4 inverse_bind_matrix;
	alignas(16) glm::quat rotation;
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 scale;
};

struct BoneBuffer {
	Bone bone;

	static constexpr StringHash name() {
		return CRC::crc64("Bone");
	}

	static constexpr vk::DescriptorSetLayoutBinding layout_binding() {
		vk::DescriptorSetLayoutBinding retval;

		retval.binding = 0;
		retval.descriptorType = vk::DescriptorType::eStorageBuffer;
		retval.descriptorCount = 1;
		retval.stageFlags = vk::ShaderStageFlagBits::eCompute;
		retval.pImmutableSamplers = nullptr;

		return retval;
	}
};

struct BoneRelationship {
	StringHash parent{ NULL_HASH };
	StringHash child{ NULL_HASH };
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
	Retval<size_t, Error> get_bone_index(const std::string& name);
	Retval<size_t, Error> get_bone_index(StringHash name);

	void add_bone_relationship(const std::string& parent, const std::string& child);
	void add_bone_relationship(StringHash parent, StringHash child);
	Retval<std::vector<StringHash>, Error> get_bone_children(const std::string& parent);
	Retval<std::vector<StringHash>, Error> get_bone_children(StringHash parent);
	Retval<StringHash, Error> get_bone_parent(const std::string& child);
	Retval<StringHash, Error> get_bone_parent(StringHash child);
	Retval<StringHash, Error> get_root_bone();
	Retval<std::vector<StringHash>, Error> get_leaf_bones();
	Retval<size_t, Error> distance_to_root(const std::string& name);
	Retval<size_t, Error> distance_to_root(StringHash name);

	void add_animation(const Animation& animation, const std::string& name);
	void add_animation(const Animation& animation, StringHash name);
	Retval<Animation*, Error> get_animation(const std::string& name);
	Retval<Animation*, Error> get_animation(StringHash name);

	Error play_animation(const std::string& name, bool loop = false);
	Error play_animation(StringHash name, bool loop = false);
	void clear_animation();

	std::vector<Bone> sample_animation_frame();

	std::vector<Bone> bones;
	std::vector<StringHash> bone_names;
	std::vector<BoneRelationship> bone_relationships;

	std::vector<Animation> animations;
	std::vector<StringHash> animation_names;

	Animation* active_animation{ nullptr };
	std::chrono::steady_clock::time_point animation_start_time;
	bool is_looped{ false };
};