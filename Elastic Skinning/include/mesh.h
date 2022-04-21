#pragma once

#include "util.h"
#include "renderingtypes.h"

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>
#include <span>
#include <cstdint>

struct ModelTransform {
	glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
	glm::vec3 position{ 0.0f, 0.0f, 0.0f };
	glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
};

template <typename T>
concept VertexType =
std::is_trivial_v<T> &&
std::is_standard_layout_v<T> &&
	requires {
		{T::binding_description()} -> std::same_as<vk::VertexInputBindingDescription>;
		{T::attribute_description()} -> ArrayType;
		{T::attribute_description()[0]} -> std::same_as<vk::VertexInputAttributeDescription&>;
};

#define BEGIN_DESCRIPTIONS(T) \
	using _Desc_Parent_T = T; \
	size_t _desc_index = 0; \
	std::vector<vk::VertexInputAttributeDescription> _ret_val

#define DESCRIPTION(Member) \
	_ret_val.push_back({}); \
	_ret_val[_desc_index].binding = 0; \
	_ret_val[_desc_index].location = _desc_index; \
	_ret_val[_desc_index].format = VkFormatType<decltype(Member)>::format(); \
	_ret_val[_desc_index].offset = offsetof(_Desc_Parent_T, Member); \
	_desc_index++

#define END_DESCRIPTIONS(_) \
	return _ret_val

struct Vertex {
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 normal;
	alignas(16) glm::vec3 color;
	alignas(16) glm::vec2 texcoords;

	static vk::VertexInputBindingDescription binding_description() {
		vk::VertexInputBindingDescription retval;

		retval.binding = 0;
		retval.stride = sizeof(Vertex);
		retval.inputRate = vk::VertexInputRate::eVertex;

		return retval;
	}

	static std::vector<vk::VertexInputAttributeDescription> attribute_description() {
		BEGIN_DESCRIPTIONS(Vertex);
		DESCRIPTION(position);
		DESCRIPTION(normal);
		DESCRIPTION(color);
		DESCRIPTION(texcoords);
		END_DESCRIPTIONS();
	}
};

struct SkeletalVertex {
	alignas(16) glm::uvec4 joints;
	alignas(16) glm::vec4 weights;
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 normal;
	alignas(16) glm::vec3 color;
	alignas(16) glm::vec2 texcoords;

	static vk::VertexInputBindingDescription binding_description() {
		vk::VertexInputBindingDescription retval;

		retval.binding = 0;
		retval.stride = sizeof(SkeletalVertex);
		retval.inputRate = vk::VertexInputRate::eVertex;

		return retval;
	}

	static std::vector<vk::VertexInputAttributeDescription> attribute_description() {
		BEGIN_DESCRIPTIONS(SkeletalVertex);
		DESCRIPTION(joints);
		DESCRIPTION(weights);
		DESCRIPTION(position);
		DESCRIPTION(normal);
		DESCRIPTION(color);
		DESCRIPTION(texcoords);
		END_DESCRIPTIONS();
	}
};

struct ElasticVertex {
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 normal;
	alignas(16) glm::vec3 color;
	alignas(8) glm::vec2 texcoords;
	alignas(4) uint32_t bone;
	alignas(4) float isovalue;

	static vk::VertexInputBindingDescription binding_description() {
		vk::VertexInputBindingDescription retval;

		retval.binding = 0;
		retval.stride = sizeof(ElasticVertex);
		retval.inputRate = vk::VertexInputRate::eVertex;

		return retval;
	}

	static std::vector<vk::VertexInputAttributeDescription> attribute_description() {
		BEGIN_DESCRIPTIONS(ElasticVertex);
		DESCRIPTION(position);
		DESCRIPTION(normal);
		DESCRIPTION(color);
		DESCRIPTION(texcoords);
		DESCRIPTION(bone);
		DESCRIPTION(isovalue);
		END_DESCRIPTIONS();
	}
};

#undef BEGIN_DESCRIPTIONS
#undef DESCRIPTION
#undef END_DESCRIPTIONS

struct ElasticVertexBuffer {
	ElasticVertex vertex;

	static constexpr StringHash name() {
		return CRC::crc64("ElasticVertex");
	}

	static constexpr vk::DescriptorSetLayoutBinding layout_binding() {
		vk::DescriptorSetLayoutBinding retval;

		retval.binding = 1;
		retval.descriptorType = vk::DescriptorType::eStorageBuffer;
		retval.descriptorCount = 1;
		retval.stageFlags = vk::ShaderStageFlagBits::eCompute;
		retval.pImmutableSamplers = nullptr;

		return retval;
	}
};

struct SkeletalVertexBuffer {
	SkeletalVertex vertex;

	static constexpr StringHash name() {
		return CRC::crc64("SkeletalVertex");
	}

	static constexpr vk::DescriptorSetLayoutBinding layout_binding() {
		vk::DescriptorSetLayoutBinding retval;

		retval.binding = 1;
		retval.descriptorType = vk::DescriptorType::eStorageBuffer;
		retval.descriptorCount = 1;
		retval.stageFlags = vk::ShaderStageFlagBits::eCompute;
		retval.pImmutableSamplers = nullptr;

		return retval;
	}
};

struct VertexBuffer {
	Vertex vertex;

	static constexpr StringHash name() {
		return CRC::crc64("Vertex");
	}

	static constexpr vk::DescriptorSetLayoutBinding layout_binding() {
		vk::DescriptorSetLayoutBinding retval;

		retval.binding = 2;
		retval.descriptorType = vk::DescriptorType::eStorageBuffer;
		retval.descriptorCount = 1;
		retval.stageFlags = vk::ShaderStageFlagBits::eCompute;
		retval.pImmutableSamplers = nullptr;

		return retval;
	}
};

template <typename T>
concept MeshType =
std::is_trivial_v<T> &&
std::is_standard_layout_v<T> &&
	requires (T m) {
		typename T::VertexType;
		typename T::IndexType;
		{m.material_name} -> std::same_as<std::string>;
		{m.vertices} -> std::same_as<std::vector<typename T::VertexType>>;
		{m.indices} -> std::same_as<std::vector<typename T::IndexType>>;
};


template <typename VertexT, typename IndexT>
struct MeshBase {
	using VertexType = VertexT;
	using IndexType = IndexT;

	std::string material_name;

	std::vector<VertexType> vertices;
	std::vector<IndexType> indices;
};

using Mesh = MeshBase<Vertex, uint32_t>;
using SkeletalMesh = MeshBase<SkeletalVertex, uint32_t>;
using ElasticMesh = MeshBase<ElasticVertex, uint32_t>;