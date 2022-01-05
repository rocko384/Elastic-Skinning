#pragma once

#include "util.h"

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <vector>
#include <array>
#include <concepts>
#include <type_traits>

template <typename T>
concept VertexType =
	std::is_trivial_v<T> &&
	std::is_standard_layout_v<T> &&
	requires {
		{T::binding_description()} -> std::same_as<vk::VertexInputBindingDescription>;
		{T::attribute_description()} -> ArrayType;
	};

struct Vertex {
	glm::vec3 position;
	glm::vec3 color;

	static vk::VertexInputBindingDescription binding_description() {
		vk::VertexInputBindingDescription retval;

		retval.binding = 0;
		retval.stride = sizeof(Vertex);
		retval.inputRate = vk::VertexInputRate::eVertex;

		return retval;
	}

	static std::array<vk::VertexInputAttributeDescription, 2> attribute_description() {
		std::array<vk::VertexInputAttributeDescription, 2> retval;

		retval[0].binding = 0;
		retval[0].location = 0;
		retval[0].format = vk::Format::eR32G32B32Sfloat;
		retval[0].offset = offsetof(Vertex, position);

		retval[1].binding = 0;
		retval[1].location = 1;
		retval[1].format = vk::Format::eR32G32B32Sfloat;
		retval[1].offset = offsetof(Vertex, color);

		return retval;
	}
};

struct Mesh {
	
	std::vector<Vertex> vertices;
	std::string pipeline_name;

};