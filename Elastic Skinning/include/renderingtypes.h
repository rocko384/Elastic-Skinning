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

template <typename GLM_T>
struct VkFormatType {
	static constexpr vk::Format format() {
		if constexpr (std::is_same_v<GLM_T, glm::vec2>) {
			return vk::Format::eR32G32Sfloat;
		}

		if constexpr (std::is_same_v<GLM_T, glm::vec3>) {
			return vk::Format::eR32G32B32Sfloat;
		}

		if constexpr (std::is_same_v<GLM_T, glm::vec4>) {
			return vk::Format::eR32G32B32A32Sfloat;
		}

		if constexpr (std::is_same_v<GLM_T, glm::uvec4>) {
			return vk::Format::eR32G32B32A32Uint;
		}

		if constexpr (std::is_same_v<GLM_T, glm::u16vec4>) {
			return vk::Format::eR16G16B16A16Uint;
		}

		if constexpr (std::is_same_v<GLM_T, uint32_t>) {
			return vk::Format::eR32Uint;
		}

		if constexpr (std::is_same_v<GLM_T, float>) {
			return vk::Format::eR32Sfloat;
		}
		
		return vk::Format::eUndefined;
	};
};

enum class RenderTarget {
	Swapchain,
	DepthBuffer
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
		typename T::DataType;
		{T::is_per_mesh()} -> BooleanTestable;
	};

template <typename T>
concept SamplerType = !BufferObjectType<T>;

template <typename T>
concept DescriptorType =
	requires {
		{T::layout_binding()} -> std::same_as<vk::DescriptorSetLayoutBinding>;
	};


template <StringLiteral Name, typename Data, uint32_t Binding, vk::ShaderStageFlagBits Stage, uint32_t Count = 1>
struct UniformBuffer {
	typedef Data DataType;

	DataType data;

	static constexpr StringHash name() {
		return CRC::crc64(Name.value);
	}

	static constexpr vk::DescriptorSetLayoutBinding layout_binding() {
		vk::DescriptorSetLayoutBinding retval;

		retval.binding = Binding;
		retval.descriptorType = vk::DescriptorType::eUniformBuffer;
		retval.descriptorCount = Count;
		retval.stageFlags = Stage;
		retval.pImmutableSamplers = nullptr;

		return retval;
	}

	static constexpr bool is_per_mesh() {
		return false;
	}
};

template <StringLiteral Name, typename Data, uint32_t Binding, vk::ShaderStageFlagBits Stage, uint32_t Count = 1>
struct StorageBuffer {
	typedef Data DataType;

	DataType data;

	static constexpr StringHash name() {
		return CRC::crc64(Name.value);
	}

	static constexpr vk::DescriptorSetLayoutBinding layout_binding() {
		vk::DescriptorSetLayoutBinding retval;

		retval.binding = Binding;
		retval.descriptorType = vk::DescriptorType::eStorageBuffer;
		retval.descriptorCount = Count;
		retval.stageFlags = Stage;
		retval.pImmutableSamplers = nullptr;

		return retval;
	}

	static constexpr bool is_per_mesh() {
		return true;
	}
};

template <StringLiteral Name, uint32_t Binding, vk::ShaderStageFlagBits Stage, uint32_t Count = 1>
struct StorageImage {

	static constexpr StringHash name() {
		return CRC::crc64(Name.value);
	}

	static constexpr vk::DescriptorSetLayoutBinding layout_binding() {
		vk::DescriptorSetLayoutBinding retval;

		retval.binding = Binding;
		retval.descriptorType = vk::DescriptorType::eStorageImage;
		retval.descriptorCount = Count;
		retval.stageFlags = Stage;
		retval.pImmutableSamplers = nullptr;

		return retval;
	}
};

template <StringLiteral Name, uint32_t Binding, vk::ShaderStageFlagBits Stage, uint32_t Count = 1>
struct ImageSampler {

	static constexpr StringHash name() {
		return CRC::crc64(Name.value);
	}

	static constexpr vk::DescriptorSetLayoutBinding layout_binding() {
		vk::DescriptorSetLayoutBinding retval;

		retval.binding = Binding;
		retval.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		retval.descriptorCount = Count;
		retval.stageFlags = Stage;
		retval.pImmutableSamplers = nullptr;

		return retval;
	}
};

using ModelBuffer = StorageBuffer<"Model", glm::mat4, 0, vk::ShaderStageFlagBits::eVertex, 1>;

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

using CameraBuffer = UniformBuffer<"Camera", Camera, 1, vk::ShaderStageFlagBits::eVertex, 1>;

using ColorSampler = ImageSampler<"Color", 0, vk::ShaderStageFlagBits::eFragment, 1>;