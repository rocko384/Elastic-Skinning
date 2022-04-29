#pragma once

#include "gfxcontext.h"

#include <vulkan/vulkan.hpp>

#include <filesystem>
#include <vector>
#include <unordered_map>

namespace Compute {

	template <typename Data, uint32_t Binding, uint32_t Count = 1>
	using UniformBuffer = ::UniformBuffer<"", Data, Binding, vk::ShaderStageFlagBits::eCompute, Count>;
	
	template <typename Data, uint32_t Binding, uint32_t Count = 1>
	using StorageBuffer = ::StorageBuffer<"", Data, Binding, vk::ShaderStageFlagBits::eCompute, Count>;

	template <uint32_t Binding, uint32_t Count = 1>
	using StorageImage = ::StorageImage<"", Binding, vk::ShaderStageFlagBits::eCompute, Count>;

	template <uint32_t Binding, uint32_t Count = 1>
	using ImageSampler = ::ImageSampler<"", Binding, vk::ShaderStageFlagBits::eCompute, Count>;

}

struct ComputePipelineImpl {

	enum class Error {
		OK,
		INVALID_CONTEXT,
		UNINITIALIZED_CONTEXT,
		NO_SHADERS,
		FAIL_CREATE_DESCRIPTOR_SET_LAYOUT,
		FAIL_CREATE_PIPELINE_LAYOUT,
		FAIL_CREATE_PIPELINE
	};

	ComputePipelineImpl() = default;
	ComputePipelineImpl(ComputePipelineImpl&& rhs) noexcept = default;
	~ComputePipelineImpl();

	ComputePipelineImpl& operator=(ComputePipelineImpl&& rhs) noexcept = default;

	Error init(GfxContext* Context);
	Error reinit();
	void deinit();

	bool is_initialized() { return is_init; }

	vk::DescriptorSetLayout descriptor_set_layout;
	vk::PipelineLayout pipeline_layout;
	vk::Pipeline pipeline;

	std::vector<vk::DescriptorSetLayoutBinding> descriptor_layout_bindings;
	vk::PushConstantRange context_push_constant;

	std::filesystem::path shader_path;

protected:

	bool is_init{ false };

	GfxContext* context{ nullptr };

};

template <typename ContextDataT, DescriptorType... Descriptors>
struct ComputePipeline : ComputePipelineImpl {

	ComputePipeline() {
		std::vector<StringHash> names = { (Descriptors::name())... };
		std::vector<vk::DescriptorSetLayoutBinding> bindings = { (Descriptors::layout_binding())... };

		descriptor_layout_bindings = bindings;

		context_push_constant = { vk::ShaderStageFlagBits::eCompute, 0, sizeof(ContextDataT) };
	}
};