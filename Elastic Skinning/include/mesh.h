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
size_t _desc_index = 0
#define DESCRIPTION(Out, Member) \
	Out[_desc_index].binding = 0; \
	Out[_desc_index].location = _desc_index; \
	Out[_desc_index].format = VkFormatType<decltype(Member)>::format(); \
	Out[_desc_index].offset = offsetof(_Desc_Parent_T, Member); \
	_desc_index++

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 color;
	glm::vec2 texcoords;

	static vk::VertexInputBindingDescription binding_description() {
		vk::VertexInputBindingDescription retval;

		retval.binding = 0;
		retval.stride = sizeof(Vertex);
		retval.inputRate = vk::VertexInputRate::eVertex;

		return retval;
	}

	static std::array<vk::VertexInputAttributeDescription, 4> attribute_description() {
		std::array<vk::VertexInputAttributeDescription, 4> retval;

		BEGIN_DESCRIPTIONS(Vertex);
		DESCRIPTION(retval, position);
		DESCRIPTION(retval, normal);
		DESCRIPTION(retval, color);
		DESCRIPTION(retval, texcoords);

		return retval;
	}
};

struct SkeletalVertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 color;
	glm::vec2 texcoords;
	glm::u16vec4 joints;
	glm::vec4 weights;

	static vk::VertexInputBindingDescription binding_description() {
		vk::VertexInputBindingDescription retval;

		retval.binding = 0;
		retval.stride = sizeof(SkeletalVertex);
		retval.inputRate = vk::VertexInputRate::eVertex;

		return retval;
	}

	static std::array<vk::VertexInputAttributeDescription, 6> attribute_description() {
		std::array<vk::VertexInputAttributeDescription, 6> retval;

		BEGIN_DESCRIPTIONS(SkeletalVertex);
		DESCRIPTION(retval, position);
		DESCRIPTION(retval, normal);
		DESCRIPTION(retval, color);
		DESCRIPTION(retval, texcoords);
		DESCRIPTION(retval, joints);
		DESCRIPTION(retval, weights);

		return retval;
	}
};

#undef BEGIN_DESCRIPTIONS
#undef DESCRIPTION

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