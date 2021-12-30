#include "renderer.h"

#include <array>

Renderer::~Renderer() {

}

void Renderer::init(GfxContext* Context) {
	if (Context == nullptr) {
		LOG_ERROR("Graphics context doesn't exist");
		return;
	}

	if (!Context->is_initialized()) {
		LOG_ERROR("Graphics context is uninitialized");
		return;
	}

	context = Context;
	max_frames_in_flight = context->render_swapchain_context.framebuffers.size();

	/*
	* Create graphics pipeline
	*/

	// Shader definition
	auto vertShaderModule = context->create_shader_module("shaders/base.vert.bin");
	auto fragShaderModule = context->create_shader_module("shaders/base.frag.bin");

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
	vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	vertShaderStageInfo.pSpecializationInfo = nullptr;

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
	fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";
	fragShaderStageInfo.pSpecializationInfo = nullptr;

	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

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
	viewport.width = context->render_swapchain_context.extent.width;
	viewport.height = context->render_swapchain_context.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor;
	scissor.offset = vk::Offset2D{ 0, 0 };
	scissor.extent = context->render_swapchain_context.extent;

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
	colorBlendInfo.blendConstants[0] = 0.0f;;
	colorBlendInfo.blendConstants[1] = 0.0f;;
	colorBlendInfo.blendConstants[2] = 0.0f;;
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
		LOG_ERROR("Failed to create pipeline layout");
		return;
	}

	pipeline_layout = pipelineLayout;

	vk::GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pViewportState = &viewportStateInfo;
	pipelineInfo.pRasterizationState = &rasterizerInfo;
	pipelineInfo.pMultisampleState = &multisampleInfo;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlendInfo;
	pipelineInfo.pDynamicState = nullptr; // &dynamicStateInfo;
	pipelineInfo.layout = pipeline_layout;
	pipelineInfo.renderPass = context->render_swapchain_context.render_pass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = nullptr;
	pipelineInfo.basePipelineIndex = -1;

	vk::Pipeline tpipeline = context->primary_logical_device.createGraphicsPipeline(nullptr, pipelineInfo);

	if (!tpipeline) {
		LOG_ERROR("Failed to create graphics pipeline");
		return;
	}

	pipeline = tpipeline;

	context->primary_logical_device.destroy(vertShaderModule);
	context->primary_logical_device.destroy(fragShaderModule);

	/*
	* Create command pool
	*/

	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.queueFamilyIndex = context->primary_queue_family_index;
	poolInfo.flags = (vk::CommandPoolCreateFlagBits)(0);

	command_pool = context->primary_logical_device.createCommandPool(poolInfo);

	if (!command_pool) {
		LOG_ERROR("Failed to create command pool");
		return;
	}

	/*
	* Allocate command buffers
	*/

	vk::CommandBufferAllocateInfo commandBufferInfo;
	commandBufferInfo.commandPool = command_pool;
	commandBufferInfo.level = vk::CommandBufferLevel::ePrimary;
	commandBufferInfo.commandBufferCount = static_cast<uint32_t>(context->render_swapchain_context.framebuffers.size());

	render_command_buffers = context->primary_logical_device.allocateCommandBuffers(commandBufferInfo);

	if (render_command_buffers.empty()) {
		LOG_ERROR("Failed to allocate command buffers");
		return;
	}

	/*
	* Record command buffers
	*/

	for (size_t i = 0; i < render_command_buffers.size(); i++) {
		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.flags = (vk::CommandBufferUsageFlagBits)(0);
		beginInfo.pInheritanceInfo = nullptr;

		render_command_buffers[i].begin(beginInfo);

		vk::RenderPassBeginInfo renderPassInfo;
		renderPassInfo.renderPass = context->render_swapchain_context.render_pass;
		renderPassInfo.framebuffer = context->render_swapchain_context.framebuffers[i];
		renderPassInfo.renderArea.offset = vk::Offset2D{ 0,0 };
		renderPassInfo.renderArea.extent = context->render_swapchain_context.extent;
		
		vk::ClearValue clearColor;
		clearColor.color.float32 = { { 0.0f, 0.0f, 0.0f, 1.0f } };

		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		render_command_buffers[i].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

		render_command_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
		
		render_command_buffers[i].draw(3, 1, 0, 0);

		render_command_buffers[i].endRenderPass();

		render_command_buffers[i].end();
	}

	/*
	* Create synchronization primitives
	*/

	image_available_semaphores.resize(max_frames_in_flight);
	render_finished_semaphores.resize(max_frames_in_flight);
	in_flight_fences.resize(max_frames_in_flight);
	images_in_flight.resize(context->render_swapchain_context.images.size(), {});

	vk::SemaphoreCreateInfo semaphoreInfo;
	vk::FenceCreateInfo fenceInfo;
	fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
	
	for (size_t i = 0; i < max_frames_in_flight; i++) {
		image_available_semaphores[i] = context->primary_logical_device.createSemaphore(semaphoreInfo);
		render_finished_semaphores[i] = context->primary_logical_device.createSemaphore(semaphoreInfo);
		in_flight_fences[i] = context->primary_logical_device.createFence(fenceInfo);

		if (!image_available_semaphores[i] 
			|| !render_finished_semaphores[i]
			|| !in_flight_fences[i]) {
			LOG_ERROR("Failed to create render synch primitives");
			return;
		}
	}

	/*
	* Finish initialization
	*/

	is_init = true;
}

void Renderer::deinit() {
	if (is_initialized()) {
		context->primary_logical_device.waitIdle();

		for (auto semaphore : image_available_semaphores) {
			context->primary_logical_device.destroy(semaphore);
		}

		for (auto semaphore : render_finished_semaphores) {
			context->primary_logical_device.destroy(semaphore);
		}

		for (auto fence : in_flight_fences) {
			context->primary_logical_device.destroy(fence);
		}

		context->primary_logical_device.destroy(command_pool);
		context->primary_logical_device.destroy(pipeline);
		context->primary_logical_device.destroy(pipeline_layout);
	}

	is_init = false;
}

void Renderer::draw_frame() {
	/// TODO: Consider moving frame sync and organization logic to SwapchainContext 

	std::array<vk::Fence, 1> currentFences = { in_flight_fences[current_frame] };
	context->primary_logical_device.waitForFences(currentFences, VK_TRUE, UINT64_MAX);

	uint32_t imageIndex = context->primary_logical_device.acquireNextImageKHR(context->render_swapchain_context.swapchain, UINT64_MAX, image_available_semaphores[current_frame], nullptr);

	if (images_in_flight[imageIndex]) {
		std::array<vk::Fence, 1> ImageInFlightFences = { images_in_flight[imageIndex] };
		context->primary_logical_device.waitForFences(ImageInFlightFences, VK_TRUE, UINT64_MAX);
	}

	vk::Semaphore waitSemaphores[] = { image_available_semaphores[current_frame]};
	vk::Semaphore signalSemaphores[] = { render_finished_semaphores[current_frame] };
	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

	vk::SubmitInfo submitInfo;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &render_command_buffers[imageIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	context->primary_logical_device.resetFences(currentFences);

	context->primary_queue.submit({ submitInfo }, in_flight_fences[current_frame]);

	vk::SwapchainKHR swapChains[] = { context->render_swapchain_context.swapchain };

	vk::PresentInfoKHR presentInfo;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	vk::Result presentStatus = context->present_queue.presentKHR(presentInfo);
	
	if (presentStatus != vk::Result::eSuccess) {
		LOG_ERROR("Swapchain presentation failure");
	}

	current_frame = (current_frame + 1) % max_frames_in_flight;
}