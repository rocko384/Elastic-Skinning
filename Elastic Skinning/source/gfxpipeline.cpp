#include "gfxpipeline.h"

#include <vector>

GfxPipeline::GfxPipeline(GfxPipeline&& rhs) noexcept {
	this->operator=(std::move(rhs));
}

GfxPipeline::~GfxPipeline() {
	deinit();
}

GfxPipeline& GfxPipeline::operator=(GfxPipeline&& rhs) noexcept {
	this->pipeline_layout = rhs.pipeline_layout;
	this->pipeline = rhs.pipeline;
	this->is_init = rhs.is_init;
	this->should_init_with_swapchain = rhs.should_init_with_swapchain;
	this->context = rhs.context;
	this->swapchain = rhs.swapchain;
	this->vertex_shader_path = rhs.vertex_shader_path;
	this->tessellation_control_shader_path = rhs.tessellation_control_shader_path;
	this->tessellation_eval_shader_path = rhs.tessellation_eval_shader_path;
	this->geometry_shader_path = rhs.geometry_shader_path;
	this->fragment_shader_path = rhs.fragment_shader_path;

	rhs.pipeline_layout = nullptr;
	rhs.pipeline = nullptr;
	rhs.is_init = false;
	rhs.should_init_with_swapchain = true;
	rhs.context = nullptr;
	rhs.swapchain = nullptr;
	rhs.vertex_shader_path.clear();
	rhs.tessellation_control_shader_path.clear();
	rhs.tessellation_eval_shader_path.clear();
	rhs.geometry_shader_path.clear();
	rhs.fragment_shader_path.clear();

	return *this;
}

GfxPipeline& GfxPipeline::set_vertex_shader(std::filesystem::path path) {
	vertex_shader_path = path;

	return *this;
}

GfxPipeline& GfxPipeline::set_tessellation_control_shader(std::filesystem::path path) {
	tessellation_control_shader_path = path;

	return *this;
}

GfxPipeline& GfxPipeline::set_tessellation_eval_shader(std::filesystem::path path) {
	tessellation_eval_shader_path = path;

	return *this;
}

GfxPipeline& GfxPipeline::set_geometry_shader(std::filesystem::path path) {
	geometry_shader_path = path;

	return *this;
}

GfxPipeline& GfxPipeline::set_fragment_shader(std::filesystem::path path) {
	fragment_shader_path = path;

	return *this;
}

GfxPipeline& GfxPipeline::set_target(/* RenderTarget Target */) {
	should_init_with_swapchain = false;

	return *this;
}


GfxPipeline::Error GfxPipeline::init(GfxContext* Context, Swapchain* Swapchain) {
	if (!Context) {
		return Error::INVALID_CONTEXT;
	}

	if (!Context->is_initialized()) {
		return Error::UNINITIALIZED_CONTEXT;
	}

	if (!Swapchain) {
		return Error::INVALID_SWAPCHAIN;
	}

	if (!Swapchain->is_initialized()) {
		return Error::UNINITIALIZED_SWAPCHAIN;
	}

	context = Context;
	swapchain = Swapchain;

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
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	// Input assembly definition
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	// Viewport definition
	vk::Viewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = swapchain->extent.width;
	viewport.height = swapchain->extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor;
	scissor.offset = vk::Offset2D{ 0, 0 };
	scissor.extent = swapchain->extent;

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

	// Pipeline layout
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

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
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlendInfo;
	pipelineInfo.pDynamicState = nullptr; // &dynamicStateInfo;
	pipelineInfo.layout = pipeline_layout;
	pipelineInfo.renderPass = swapchain->render_pass;
	pipelineInfo.subpass = 0;
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

GfxPipeline::Error GfxPipeline::reinit() {
	deinit();

	if (is_swapchain_dependent()) {
		return init(context, swapchain);
	}

	return Error::OK;
}

void GfxPipeline::deinit() {
	if (is_initialized()) {
		context->primary_logical_device.destroy(pipeline);
		context->primary_logical_device.destroy(pipeline_layout);
	}

	is_init = false;
}