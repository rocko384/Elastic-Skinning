#include "computepipeline.h"

ComputePipelineImpl::~ComputePipelineImpl() {
	deinit();
}

ComputePipelineImpl::Error ComputePipelineImpl::init(GfxContext* Context) {
	if (!Context) {
		return Error::INVALID_CONTEXT;
	}

	if (!Context->is_initialized()) {
		return Error::UNINITIALIZED_CONTEXT;
	}

	context = Context;

	if (shader_path.empty()) {
		return Error::NO_SHADERS;
	}

	auto shaderModule = context->create_shader_module(shader_path);

	vk::PipelineShaderStageCreateInfo shaderInfo;
	shaderInfo.stage = vk::ShaderStageFlagBits::eCompute;
	shaderInfo.module = shaderModule;
	shaderInfo.pName = "main";
	shaderInfo.pSpecializationInfo = nullptr;

	// Descriptor set layouts
	std::vector<vk::DescriptorSetLayoutBinding> bufferLayoutBindings = descriptor_layout_bindings;

	vk::DescriptorSetLayoutCreateInfo bufferDescriptorLayoutInfo;
	bufferDescriptorLayoutInfo.bindingCount = bufferLayoutBindings.size();
	bufferDescriptorLayoutInfo.pBindings = bufferLayoutBindings.data();

	descriptor_set_layout = context->primary_logical_device.createDescriptorSetLayout(bufferDescriptorLayoutInfo);


	std::vector<vk::DescriptorSetLayout> descriptorSetLayouts{
		descriptor_set_layout
	};

	// Pipeline layout
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

	if (context_push_constant.size >= 4) {
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &context_push_constant;
	}
	else {
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;
	}

	pipeline_layout = context->primary_logical_device.createPipelineLayout(pipelineLayoutInfo);

	if (!pipeline_layout) {
		return Error::FAIL_CREATE_PIPELINE_LAYOUT;
	}

	vk::ComputePipelineCreateInfo pipelineInfo;
	pipelineInfo.stage = shaderInfo;
	pipelineInfo.layout = pipeline_layout;
	pipelineInfo.basePipelineHandle = nullptr;
	pipelineInfo.basePipelineIndex = -1;

	vk::Pipeline tpipeline = context->primary_logical_device.createComputePipeline(nullptr, pipelineInfo);

	if (!tpipeline) {
		return Error::FAIL_CREATE_PIPELINE;
	}

	pipeline = tpipeline;

	context->primary_logical_device.destroy(shaderModule);

	is_init = true;

	return Error::OK;
}

ComputePipelineImpl::Error ComputePipelineImpl::reinit() {
	deinit();
	return init(context);
}

void ComputePipelineImpl::deinit() {
	if (is_init) {
		is_init = false;

		context->primary_logical_device.destroy(pipeline);
		context->primary_logical_device.destroy(pipeline_layout);
		context->primary_logical_device.destroy(descriptor_set_layout);
	}
}
