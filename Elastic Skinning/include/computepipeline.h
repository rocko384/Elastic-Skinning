#pragma once

#include "gfxcontext.h"

#include <vulkan/vulkan.hpp>

#include <filesystem>
#include <vector>
#include <unordered_map>

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

	std::unordered_map<StringHash, vk::DescriptorSetLayoutBinding> descriptor_layout_bindings;
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

		for (size_t i = 0; i < names.size(); i++) {
			descriptor_layout_bindings[names[i]] = bindings[i];
		}

		context_push_constant = { vk::ShaderStageFlagBits::eCompute, 0, sizeof(ContextDataT) };
	}

};