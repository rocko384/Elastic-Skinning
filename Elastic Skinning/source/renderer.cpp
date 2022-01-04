#include "renderer.h"

#include <array>

Renderer::~Renderer() {
	deinit();
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

	context->window->add_resized_callback(
		[this](size_t w, size_t h) {
			this->window_resized_callback(w, h);
		}
	);

	context->window->add_minimized_callback(
		[this]() {
			this->window_minimized_callback();
		}
	);

	context->window->add_maximized_callback(
		[this]() {
			this->window_maximized_callback();
		}
	);

	context->window->add_restored_callback(
		[this]() {
			this->window_restored_callback();
		}
	);

	/*
	* Create the swapchain
	*/

	/// TODO: Break this out in some way to facilitate dynamic swapchain recreation for window resizing

	auto swapchainError = render_swapchain.init(context);

	if (swapchainError != Swapchain::Error::OK) {
		switch (swapchainError) {
		case Swapchain::Error::INVALID_CONTEXT:
			LOG_ERROR("Swapchain was given invalid graphics context");
			return;
			break;
		case Swapchain::Error::UNINITIALIZED_CONTEXT:
			LOG_ERROR("Swapchain was given uninitalized graphics context");
			return;
			break;
		case Swapchain::Error::FAIL_CREATE_SWAPCHAIN:
			LOG_ERROR("Failed to create swapchain");
			return;
			break;
		case Swapchain::Error::FAIL_CREATE_IMAGE_VIEW:
			LOG_ERROR("Failed to create swapchain image view");
			return;
			break;
		case Swapchain::Error::FAIL_CREATE_RENDER_PASS:
			LOG_ERROR("Failed to create swapchain render pass");
			return;
			break;
		default:
			break;
		}
	}

	max_frames_in_flight = render_swapchain.framebuffers.size();

	/*
	* Create command pool
	*/

	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.queueFamilyIndex = context->primary_queue_family_index;
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;

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
	commandBufferInfo.commandBufferCount = static_cast<uint32_t>(render_swapchain.framebuffers.size());

	auto commandBuffers = context->primary_logical_device.allocateCommandBuffers(commandBufferInfo);

	if (commandBuffers.empty()) {
		LOG_ERROR("Failed to allocate command buffers");
		return;
	}

	std::vector<CommandBufferWithMutex> tempBuffers(commandBuffers.size());
	render_command_buffers.swap(tempBuffers);

	for (size_t i = 0; i < commandBuffers.size(); i++) {
		render_command_buffers[i].command_bufer = commandBuffers[i];
	}

	/*
	* Create synchronization primitives
	*/

	image_available_semaphores.resize(max_frames_in_flight);
	render_finished_semaphores.resize(max_frames_in_flight);
	in_flight_fences.resize(max_frames_in_flight);
	images_in_flight.resize(render_swapchain.images.size(), {});

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

		for (auto& pipeline : pipelines) {
			pipeline.second.deinit();
		}

		render_swapchain.deinit();
	}

	is_init = false;
}

Renderer::Error Renderer::register_pipeline(const std::string& Name, GfxPipeline&& Pipeline) {
	return register_pipeline(std::hash<std::string>()(Name), std::move(Pipeline));
}

Renderer::Error Renderer::register_pipeline(StringHash Name, GfxPipeline&& Pipeline) {
	if (pipelines.contains(Name)) {
		return Error::PIPELINE_WITH_NAME_ALREADY_EXISTS;
	}

	pipelines[Name] = std::move(Pipeline);
	pipelines[Name].deinit();

	GfxPipeline::Error pipelineError;

	if (pipelines[Name].is_swapchain_dependent()) {
		pipelineError = pipelines[Name].init(context, &render_swapchain);
	}

	switch (pipelineError) {
	case GfxPipeline::Error::INVALID_CONTEXT:
		LOG_ERROR("Pipeline was given invalid graphics context");
		return Error::PIPELINE_INIT_ERROR;
		break;
	case GfxPipeline::Error::UNINITIALIZED_CONTEXT:
		LOG_ERROR("Pipeline was given uninitalized graphics context");
		return Error::PIPELINE_INIT_ERROR;
		break;
	case GfxPipeline::Error::INVALID_SWAPCHAIN:
		LOG_ERROR("Pipeline was given invalid swapchain");
		return Error::PIPELINE_INIT_ERROR;
		break;
	case GfxPipeline::Error::UNINITIALIZED_SWAPCHAIN:
		LOG_ERROR("Pipeline was given uninitialized swapchain");
		return Error::PIPELINE_INIT_ERROR;
		break;
	case GfxPipeline::Error::NO_SHADERS:
		LOG_ERROR("Pipeline was given no shaders");
		return Error::PIPELINE_INIT_ERROR;
		break;
	case GfxPipeline::Error::FAIL_CREATE_PIPELINE_LAYOUT:
		LOG_ERROR("Failed to create pipeline layout");
		return Error::PIPELINE_INIT_ERROR;
		break;
	case GfxPipeline::Error::FAIL_CREATE_PIPELINE:
		LOG_ERROR("Failed to create graphics pipeline");
		return Error::PIPELINE_INIT_ERROR;
		break;
	default:
		break;
	}

	return Error::OK;
}

void Renderer::draw_frame() {
	if (!should_render()) {
		return;
	}

	if (is_first_render) {
		record_command_buffers();
		is_first_render = false;
	}

	/// TODO: Consider moving frame sync and organization logic to SwapchainContext 

	std::array<vk::Fence, 1> currentFences = { in_flight_fences[current_frame] };
	context->primary_logical_device.waitForFences(currentFences, VK_TRUE, UINT64_MAX);

	auto imageIndex = context->primary_logical_device.acquireNextImageKHR(render_swapchain.swapchain, UINT64_MAX, image_available_semaphores[current_frame], nullptr);
	
	if (imageIndex.result != vk::Result::eSuccess) {
		LOG_ERROR("Error acquiring swapchain image");
	}

	if (imageIndex.result == vk::Result::eErrorOutOfDateKHR) {
		LOG_ERROR("Swapchain is out of date");
	}

	if (images_in_flight[imageIndex.value]) {
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
	submitInfo.pCommandBuffers = &render_command_buffers[imageIndex].command_bufer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	context->primary_logical_device.resetFences(currentFences);

	context->primary_queue.submit({ submitInfo }, in_flight_fences[current_frame]);

	vk::SwapchainKHR swapChains[] = { render_swapchain.swapchain };

	vk::PresentInfoKHR presentInfo;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex.value;
	presentInfo.pResults = nullptr;

	vk::Result presentStatus = context->present_queue.presentKHR(presentInfo);
	
	if (presentStatus != vk::Result::eSuccess) {
		LOG_ERROR("Swapchain presentation failure");
	}

	current_frame = (current_frame + 1) % max_frames_in_flight;
}

bool Renderer::should_render() {
	return !context->window->is_minimized()
		|| !render_swapchain.is_initialized()
		|| !are_command_buffers_recorded;
}

void Renderer::record_command_buffers() {
	context->primary_logical_device.waitIdle();

	for (size_t i = 0; i < render_command_buffers.size(); i++) {
		const std::lock_guard<std::mutex> lock(render_command_buffers[i].mutex);

		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.flags = (vk::CommandBufferUsageFlagBits)(0);
		beginInfo.pInheritanceInfo = nullptr;

		render_command_buffers[i].command_bufer.begin(beginInfo);

		vk::RenderPassBeginInfo renderPassInfo;
		renderPassInfo.renderPass = render_swapchain.render_pass;
		renderPassInfo.framebuffer = render_swapchain.framebuffers[i];
		renderPassInfo.renderArea.offset = vk::Offset2D{ 0,0 };
		renderPassInfo.renderArea.extent = render_swapchain.extent;

		vk::ClearValue clearColor;
		clearColor.color.float32 = { { 0.1f, 0.1f, 0.1f, 0.0f } };

		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		render_command_buffers[i].command_bufer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

		render_command_buffers[i].command_bufer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines[std::hash<std::string>()("base")].pipeline);

		render_command_buffers[i].command_bufer.draw(3, 1, 0, 0);

		render_command_buffers[i].command_bufer.endRenderPass();

		render_command_buffers[i].command_bufer.end();
	}

	are_command_buffers_recorded = true;
}

void Renderer::reset_command_buffers() {
	context->primary_logical_device.waitIdle();

	for (auto& commandBuffer : render_command_buffers) {
		const std::lock_guard<std::mutex> lock(commandBuffer.mutex);

		commandBuffer.command_bufer.reset();
	}

	are_command_buffers_recorded = false;
}

void Renderer::window_resized_callback(size_t w, size_t h) {
	window_restored_callback();
}

void Renderer::window_minimized_callback() {
}

void Renderer::window_maximized_callback() {
	window_restored_callback();
}

void Renderer::window_restored_callback() {
	context->primary_logical_device.waitIdle();

	render_swapchain.reinit();

	for (auto& pipeline : pipelines) {
		pipeline.second.reinit();
	}

	reset_command_buffers();
	record_command_buffers();
}