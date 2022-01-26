#pragma once

#include "crc.h"

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <optional>
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

		return retval;
	}
};

struct Image {
	BinaryBlob data;
	size_t width;
	size_t height;
	size_t channel_count;
};

struct Sampler {
	enum class Filter {
		Nearest,
		Linear,
		NearestMipmapNearest,
		LinearMipmapNearest,
		NearestMipmapLinear,
		LinearMipmapLinear
	};

	enum class Wrap {
		Repeat,
		ClampToEdge,
		MirroredRepeat
	};

	Filter mag_filter{ Filter::Nearest };
	Filter min_filter{ Filter::Nearest };
	Wrap wrap_u{ Wrap::Repeat };
	Wrap wrap_v{ Wrap::Repeat };
};

struct Texture {
	Image image;
	Sampler sampler;
};

struct Material {
	std::optional<Texture> albedo;
	std::optional<Texture> normal;
	std::optional<Texture> metallic_roughness;

	std::string name;

	std::string pipeline_name;

	glm::vec4 albedo_factor{ 1.0f, 1.0f, 1.0f, 1.0f };
	float metallic_factor{ 0.0f };
	float roughness_factor{ 0.0f };
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

	static constexpr StringHash name() {
		return CRC::crc64("Model");
	}

	static constexpr vk::DescriptorSetLayoutBinding layout_binding() {
		vk::DescriptorSetLayoutBinding retval;

		retval.binding = 0;
		retval.descriptorType = vk::DescriptorType::eStorageBuffer;
		retval.descriptorCount = 1;
		retval.stageFlags = vk::ShaderStageFlagBits::eVertex;
		retval.pImmutableSamplers = nullptr;

		return retval;
	}

	static constexpr bool is_per_mesh() {
		return layout_binding().descriptorType == vk::DescriptorType::eStorageBuffer;
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

	static constexpr StringHash name() {
		return CRC::crc64("Camera");
	}

	static constexpr vk::DescriptorSetLayoutBinding layout_binding() {
		vk::DescriptorSetLayoutBinding retval;

		retval.binding = 1;
		retval.descriptorType = vk::DescriptorType::eUniformBuffer;
		retval.descriptorCount = 1;
		retval.stageFlags = vk::ShaderStageFlagBits::eVertex;
		retval.pImmutableSamplers = nullptr;

		return retval;
	}

	static constexpr bool is_per_mesh() {
		return layout_binding().descriptorType == vk::DescriptorType::eStorageBuffer;
	}
};

struct ColorSampler {
	static constexpr StringHash name() {
		return CRC::crc64("Color");
	}

	static constexpr vk::DescriptorSetLayoutBinding layout_binding() {
		vk::DescriptorSetLayoutBinding retval;

		retval.binding = 0;
		retval.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		retval.descriptorCount = 1;
		retval.stageFlags = vk::ShaderStageFlagBits::eFragment;
		retval.pImmutableSamplers = nullptr;

		return retval;
	}
};