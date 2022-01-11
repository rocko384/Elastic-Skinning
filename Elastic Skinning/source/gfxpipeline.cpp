#include "gfxpipeline.h"

#include <vector>

GfxPipelineImpl::~GfxPipelineImpl() {
	deinit();
}

GfxPipelineImpl& GfxPipelineImpl::set_vertex_shader(std::filesystem::path path) {
	vertex_shader_path = path;

	return *this;
}

GfxPipelineImpl& GfxPipelineImpl::set_tessellation_control_shader(std::filesystem::path path) {
	tessellation_control_shader_path = path;

	return *this;
}

GfxPipelineImpl& GfxPipelineImpl::set_tessellation_eval_shader(std::filesystem::path path) {
	tessellation_eval_shader_path = path;

	return *this;
}

GfxPipelineImpl& GfxPipelineImpl::set_geometry_shader(std::filesystem::path path) {
	geometry_shader_path = path;

	return *this;
}

GfxPipelineImpl& GfxPipelineImpl::set_fragment_shader(std::filesystem::path path) {
	fragment_shader_path = path;

	return *this;
}

GfxPipelineImpl& GfxPipelineImpl::set_target(RenderTarget Target) {
	target = Target;

	return *this;
}

GfxPipelineImpl::Error GfxPipelineImpl::init(GfxContext* Context, vk::Extent2D* Viewport, vk::RenderPass* RenderPass, uint32_t Subpass) {
	if (!Context) {
		return Error::INVALID_CONTEXT;
	}

	if (!Context->is_initialized()) {
		return Error::UNINITIALIZED_CONTEXT;
	}

	if (!RenderPass) {
		return Error::INVALID_RENDER_PASS;
	}

	context = Context;
	viewport_size = Viewport;
	render_pass = RenderPass;
	subpass = Subpass;

	// Shader stage definition

	std::vector<vk::ShaderModule> shaderModules;
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

	if (!vertex_shader_path.empty()) {
		auto shaderModule = context->create_shader_module(vertex_shader_path);

		vk::PipelineShaderStageCreateInfo shaderInfo;
		shaderInfo.stage = vk::ShaderStageFlagBits::eVertex;
		shaderInfo.module = shaderModule;
		shaderInfo.pName = "main";
		shaderInfo.pSpecializationInfo = nullptr;

		shaderModules.push_back(shaderModule);
		shaderStages.push_back(shaderInfo);
	}

	if (!tessellation_control_shader_path.empty()) {
		auto shaderModule = context->create_shader_module(tessellation_control_shader_path);

		vk::PipelineShaderStageCreateInfo shaderInfo;
		shaderInfo.stage = vk::ShaderStageFlagBits::eTessellationControl;
		shaderInfo.module = shaderModule;
		shaderInfo.pName = "main";
		shaderInfo.pSpecializationInfo = nullptr;

		shaderModules.push_back(shaderModule);
		shaderStages.push_back(shaderInfo);
	}

	if (!tessellation_eval_shader_path.empty()) {
		auto shaderModule = context->create_shader_module(tessellation_eval_shader_path);

		vk::PipelineShaderStageCreateInfo shaderInfo;
		shaderInfo.stage = vk::ShaderStageFlagBits::eTessellationEvaluation;
		shaderInfo.module = shaderModule;
		shaderInfo.pName = "main";
		shaderInfo.pSpecializationInfo = nullptr;

		shaderModules.push_back(shaderModule);
		shaderStages.push_back(shaderInfo);
	}

	if (!geometry_shader_path.empty()) {
		auto shaderModule = context->create_shader_module(geometry_shader_path);

		vk::PipelineShaderStageCreateInfo shaderInfo;
		shaderInfo.stage = vk::ShaderStageFlagBits::eGeometry;
		shaderInfo.module = shaderModule;
		shaderInfo.pName = "main";
		shaderInfo.pSpecializationInfo = nullptr;

		shaderModules.push_back(shaderModule);
		shaderStages.push_back(shaderInfo);
	}

	if (!fragment_shader_path.empty()) {
		auto shaderModule = context->create_shader_module(fragment_shader_path);

		vk::PipelineShaderStageCreateInfo shaderInfo;
		shaderInfo.stage = vk::ShaderStageFlagBits::eFragment;
		shaderInfo.module = shaderModule;
		shaderInfo.pName = "main";
		shaderInfo.pSpecializationInfo = nullptr;

		shaderModules.push_back(shaderModule);
		shaderStages.push_back(shaderInfo);
	}

	if (shaderModules.empty()) {
		return Error::NO_SHADERS;
	}

	// Vertex input definition
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &vertex_binding_description;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_descriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = vertex_attribute_descriptions.data();

	// Input assembly definition
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	// Viewport definition
	vk::Viewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = viewport_size->width;
	viewport.height = viewport_size->height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor;
	scissor.offset = vk::Offset2D{ 0, 0 };
	scissor.extent = *viewport_size;

	vk::PipelineViewportStateCreateInfo viewportStateInfo;
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.pViewports = &viewport;
	viewportStateInfo.scissorCount = 1;
	viewportStateInfo.pScissors = &scissor;

	// Rasterizer definition
	vk::PipelineRasterizationStateCreateInfo rasterizerInfo;
	rasterizerInfo.depthClampEnable = VK_FALSE;
	rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerInfo.polygonMode = vk::PolygonMode::eFill;
	rasterizerInfo.lineWidth = 1.0f;
	rasterizerInfo.cullMode = vk::CullModeFlagBits::eBack;
	rasterizerInfo.frontFace = vk::FrontFace::eClockwise;
	rasterizerInfo.depthBiasEnable = VK_FALSE;
	rasterizerInfo.depthBiasConstantFactor = 0.0f;
	rasterizerInfo.depthBiasClamp = 0.0f;
	rasterizerInfo.depthBiasSlopeFactor = 0.0f;

	// Multisampling definition
	vk::PipelineMultisampleStateCreateInfo multisampleInfo;
	multisampleInfo.sampleShadingEnable = VK_FALSE;
	multisampleInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
	multisampleInfo.minSampleShading = 1.0f;
	multisampleInfo.pSampleMask = nullptr;
	multisampleInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleInfo.alphaToOneEnable = VK_FALSE;

	// Depth stencil state
	vk::PipelineDepthStencilStateCreateInfo depthStencilState;
	depthStencilState.depthTestEnable = VK_TRUE;
	
	if (target == RenderTarget::DepthBuffer) {
		depthStencilState.depthWriteEnable = VK_TRUE;
	}
	else {
		depthStencilState.depthWriteEnable = VK_FALSE;
	}

	depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.stencilTestEnable = VK_FALSE;
	depthStencilState.front = vk::StencilOp(0);
	depthStencilState.back = vk::StencilOp(0);
	depthStencilState.minDepthBounds = 0.0f;
	depthStencilState.maxDepthBounds = 1.0f;

	// Color blending
	vk::PipelineColorBlendAttachmentState colorBlendAttachment;
	colorBlendAttachment.colorWriteMask =
		vk::ColorComponentFlagBits::eR
		| vk::ColorComponentFlagBits::eG
		| vk::ColorComponentFlagBits::eB
		| vk::ColorComponentFlagBits::eA;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
	colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
	colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;

	vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
	colorBlendInfo.logicOpEnable = VK_FALSE;
	colorBlendInfo.logicOp = vk::LogicOp::eCopy;
	colorBlendInfo.attachmentCount = 1;
	colorBlendInfo.pAttachments = &colorBlendAttachment;
	colorBlendInfo.blendConstants[0] = 0.0f;
	colorBlendInfo.blendConstants[1] = 0.0f;
	colorBlendInfo.blendConstants[2] = 0.0f;
	colorBlendInfo.blendConstants[3] = 0.0f;

	// Dynamic state
	vk::DynamicState dynamicStates[] = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eLineWidth
	};

	vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
	dynamicStateInfo.dynamicStateCount = 2;
	dynamicStateInfo.pDynamicStates = dynamicStates;

	// Descriptor set layouts
	std::vector<vk::DescriptorSetLayoutBinding> bufferLayoutBindings;
	std::vector<vk::DescriptorSetLayoutBinding> textureLayoutBindings;

	for (auto& binding : descriptor_layout_bindings) {
		if (descriptor_is_buffer[binding.first]) {
			bufferLayoutBindings.push_back(binding.second);
		}
		else {
			textureLayoutBindings.push_back(binding.second);
		}
	}

	vk::DescriptorSetLayoutCreateInfo bufferDescriptorLayoutInfo;
	bufferDescriptorLayoutInfo.bindingCount = bufferLayoutBindings.size();
	bufferDescriptorLayoutInfo.pBindings = bufferLayoutBindings.data();

	buffer_descriptor_set_layout = context->primary_logical_device.createDescriptorSetLayout(bufferDescriptorLayoutInfo);

	vk::DescriptorSetLayoutCreateInfo textureDescriptorLayoutInfo;
	textureDescriptorLayoutInfo.bindingCount = textureLayoutBindings.size();
	textureDescriptorLayoutInfo.pBindings = textureLayoutBindings.data();

	texture_descriptor_set_layout = context->primary_logical_device.createDescriptorSetLayout(textureDescriptorLayoutInfo);

	if (!buffer_descriptor_set_layout || !texture_descriptor_set_layout) {
		return Error::FAIL_CREATE_DESCRIPTOR_SET_LAYOUT;
	}

	std::vector<vk::DescriptorSetLayout> descriptorSetLayouts{
		buffer_descriptor_set_layout,
		texture_descriptor_set_layout
	};

	// Push constant ranges
	std::vector<vk::PushConstantRange> pushConstantRanges{ mesh_id_push_constant };

	// Pipeline layout
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
	pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

	vk::PipelineLayout pipelineLayout = context->primary_logical_device.createPipelineLayout(pipelineLayoutInfo);

	if (!pipelineLayout) {
		return Error::FAIL_CREATE_PIPELINE_LAYOUT;
	}

	pipeline_layout = pipelineLayout;

	vk::GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.stageCount = shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pViewportState = &viewportStateInfo;
	pipelineInfo.pRasterizationState = &rasterizerInfo;
	pipelineInfo.pMultisampleState = &multisampleInfo;
	pipelineInfo.pDepthStencilState = &depthStencilState;
	pipelineInfo.pColorBlendState = &colorBlendInfo;
	pipelineInfo.pDynamicState = nullptr; // &dynamicStateInfo;
	pipelineInfo.layout = pipeline_layout;
	pipelineInfo.renderPass = *render_pass;
	pipelineInfo.subpass = subpass;
	pipelineInfo.basePipelineHandle = nullptr;
	pipelineInfo.basePipelineIndex = -1;

	vk::Pipeline tpipeline = context->primary_logical_device.createGraphicsPipeline(nullptr, pipelineInfo);

	if (!tpipeline) {
		return Error::FAIL_CREATE_PIPELINE;
	}

	pipeline = tpipeline;

	for (auto shader : shaderModules) {
		context->primary_logical_device.destroy(shader);
	}

	is_init = true;

	return Error::OK;
}

GfxPipelineImpl::Error GfxPipelineImpl::reinit() {
	deinit();

	return init(context, viewport_size, render_pass, subpass);
}

void GfxPipelineImpl::deinit() {
	if (is_initialized()) {
		is_init = false;

		context->primary_logical_device.destroy(pipeline);
		context->primary_logical_device.destroy(pipeline_layout);
		context->primary_logical_device.destroy(buffer_descriptor_set_layout);
		context->primary_logical_device.destroy(texture_descriptor_set_layout);
	}
}