#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <functional>
#include <concepts>
#include <type_traits>

enum class RenderTarget {
	Swapchain,
	DepthBuffer
};

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

struct Vertex {
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texcoords;

	static vk::VertexInputBindingDescription binding_description() {
		vk::VertexInputBindingDescription retval;

		retval.binding = 0;
		retval.stride = sizeof(Vertex);
		retval.inputRate = vk::VertexInputRate::eVertex;

		return retval;
	}

	static std::array<vk::VertexInputAttributeDescription, 3> attribute_description() {
		std::array<vk::VertexInputAttributeDescription, 3> retval;

		retval[0].binding = 0;
		retval[0].location = 0;
		retval[0].format = vk::Format::eR32G32B32Sfloat;
		retval[0].offset = offsetof(Vertex, position);

		retval[1].binding = 0;
		retval[1].location = 1;
		retval[1].format = vk::Format::eR32G32B32Sfloat;
		retval[1].offset = offsetof(Vertex, color);

		retval[2].binding = 0;
		retval[2].location = 2;
		retval[2].format = vk::Format::eR32G32Sfloat;
		retval[2].offset = offsetof(Vertex, texcoords);

		return retval;
	}
};

template <typename T>
concept BufferObjectType =
	std::is_trivial_v<T> &&
	std::is_standard_layout_v<T> &&
	requires {
		{T::is_per_mesh()} -> BooleanTestable;
	};

template <typename T>
concept SamplerType = !BufferObjectType<T>;

template <typename T>
concept DescriptorType =
	(BufferObjectType<T> || SamplerType<T>) &&
	requires {
		{T::name()} -> std::same_as<StringHash>;
		{T::layout_binding()} -> std::same_as<vk::DescriptorSetLayoutBinding>;
	};

struct ModelBuffer {
	glm::mat4 model;

	static StringHash name() {
		return std::hash<std::string>()("Model");
	}

	static bool is_per_mesh() {
		return true;
	}

	static vk::DescriptorSetLayoutBinding layout_binding() {
		vk::DescriptorSetLayoutBinding retval;

		retval.binding = 0;
		retval.descriptorType = vk::DescriptorType::eStorageBuffer;
		retval.descriptorCount = 1;
		retval.stageFlags = vk::ShaderStageFlagBits::eVertex;
		retval.pImmutableSamplers = nullptr;

		return retval;
	}
};

#define NO_CAMERA nullptr

struct Camera {
	glm::mat4 view;
	glm::mat4 projection;
	
	void orient(const glm::vec3& position, const glm::quat& rotation = { 1.0f, 0.0f, 0.0f, 0.0f }, const glm::vec3& up_vector = { 0.0f, -1.0f, 0.0f }) {
		view = glm::inverse(glm::translate(glm::mat4_cast(rotation), position));
	}

	void look_at(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up_vector = { 0.0f, -1.0f, 0.0f }) {
		view = glm::lookAt(position, target, up_vector);
	}
};

struct CameraBuffer {
	glm::mat4 view;
	glm::mat4 projection;

	static StringHash name() {
		return std::hash<std::string>()("Camera");
	}

	static bool is_per_mesh() {
		return false;
	}

	static vk::DescriptorSetLayoutBinding layout_binding() {
		vk::DescriptorSetLayoutBinding retval;

		retval.binding = 1;
		retval.descriptorType = vk::DescriptorType::eUniformBuffer;
		retval.descriptorCount = 1;
		retval.stageFlags = vk::ShaderStageFlagBits::eVertex;
		retval.pImmutableSamplers = nullptr;

		return retval;
	}
};

struct ColorSampler {
	static StringHash name() {
		return std::hash<std::string>()("Color");
	}

	static vk::DescriptorSetLayoutBinding layout_binding() {
		vk::DescriptorSetLayoutBinding retval;

		retval.binding = 0;
		retval.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		retval.descriptorCount = 1;
		retval.stageFlags = vk::ShaderStageFlagBits::eFragment;
		retval.pImmutableSamplers = nullptr;

		return retval;
	}
};