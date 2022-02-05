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

		/*
		retval[0].binding = 0;
		retval[0].location = 0;
		retval[0].format = vk::Format::eR32G32B32Sfloat;
		retval[0].offset = offsetof(Vertex, position);

		retval[1].binding = 0;
		retval[1].location = 1;
		retval[1].format = vk::Format::eR32G32B32Sfloat;
		retval[1].offset = offsetof(Vertex, normal);

		retval[2].binding = 0;
		retval[2].location = 2;
		retval[2].format = vk::Format::eR32G32B32Sfloat;
		retval[2].offset = offsetof(Vertex, color);

		retval[3].binding = 0;
		retval[3].location = 3;
		retval[3].format = vk::Format::eR32G32Sfloat;
		retval[3].offset = offsetof(Vertex, texcoords);
		*/

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

		retval[0].binding = 0;
		retval[0].location = 0;
		retval[0].format = vk::Format::eR32G32B32Sfloat;
		retval[0].offset = offsetof(SkeletalVertex, position);

		retval[1].binding = 0;
		retval[1].location = 1;
		retval[1].format = vk::Format::eR32G32B32Sfloat;
		retval[1].offset = offsetof(SkeletalVertex, normal);

		retval[2].binding = 0;
		retval[2].location = 2;
		retval[2].format = vk::Format::eR32G32B32Sfloat;
		retval[2].offset = offsetof(SkeletalVertex, color);

		retval[3].binding = 0;
		retval[3].location = 3;
		retval[3].format = vk::Format::eR32G32Sfloat;
		retval[3].offset = offsetof(SkeletalVertex, texcoords);

		retval[4].binding = 0;
		retval[4].location = 4;
		retval[4].format = vk::Format::eR16G16B16A16Uint;
		retval[4].offset = offsetof(SkeletalVertex, joints);

		retval[5].binding = 0;
		retval[5].location = 5;
		retval[5].format = vk::Format::eR32G32B32A32Sfloat;
		retval[5].offset = offsetof(SkeletalVertex, weights);

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