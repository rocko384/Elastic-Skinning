#include "swapchain.h"

#include <algorithm>
#include <array>

Swapchain::~Swapchain() {
	deinit();
}

Swapchain::Error Swapchain::init(GfxContext* Context) {
	if (!Context) {
		return Error::INVALID_CONTEXT;
	}

	if (!Context->is_initialized()) {
		return Error::UNINITIALIZED_CONTEXT;
	}

	context = Context;
	
	auto surfaceCapabilities = context->primary_physical_device.getSurfaceCapabilitiesKHR(context->render_surface);
	auto surfaceFormats = context->primary_physical_device.getSurfaceFormatsKHR(context->render_surface);
	auto surfacePresentModes = context->primary_physical_device.getSurfacePresentModesKHR(context->render_surface);

	vk::SurfaceFormatKHR bestFormat = surfaceFormats[0].format;

	for (auto format : surfaceFormats) {
		if (format.format == vk::Format::eB8G8R8A8Srgb
			&& format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			bestFormat = format;
			break;
		}
	}

	vk::PresentModeKHR bestPresentMode = vk::PresentModeKHR::eFifo;

	for (auto mode : surfacePresentModes) {
		if (mode == vk::PresentModeKHR::eMailbox) {
			bestPresentMode = vk::PresentModeKHR::eMailbox;
			break;
		}
	}

	vk::Extent2D bestExtent;

	if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
		bestExtent = surfaceCapabilities.currentExtent;
	}
	else {
		int width, height;
		SDL_Vulkan_GetDrawableSize(context->window->window, &width, &height);

		vk::Extent2D actualExtent{
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(
			actualExtent.width,
			surfaceCapabilities.minImageExtent.width,
			surfaceCapabilities.maxImageExtent.width
		);

		actualExtent.height = std::clamp(
			actualExtent.height,
			surfaceCapabilities.minImageExtent.height,
			surfaceCapabilities.maxImageExtent.height
		);

		bestExtent = actualExtent;
	}

	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;

	imageCount = std::clamp(imageCount, imageCount, surfaceCapabilities.maxImageCount);

	vk::SwapchainCreateInfoKHR swapchainCreateInfo;
	swapchainCreateInfo.surface = context->render_surface;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = bestFormat.format;
	swapchainCreateInfo.imageColorSpace = bestFormat.colorSpace;
	swapchainCreateInfo.imageExtent = bestExtent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

	std::array<uint32_t, 2> queueFamilyIndices{ context->primary_queue_family_index, context->present_queue_family_index };

	bool areQueuesUnique = true;
	if (queueFamilyIndices.size() > 1) {
		for (size_t i = 1; i < queueFamilyIndices.size(); i++) {
			if (queueFamilyIndices[i] == queueFamilyIndices[i - 1]) {
				areQueuesUnique = false;
				break;
			}
		}
	}

	if (areQueuesUnique) {
		swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		swapchainCreateInfo.queueFamilyIndexCount = queueFamilyIndices.size();
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
	}
	else {
		swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	swapchainCreateInfo.presentMode = bestPresentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	vk::SwapchainKHR tswapchain = context->primary_logical_device.createSwapchainKHR(swapchainCreateInfo);

	if (!tswapchain) {
		return Error::FAIL_CREATE_SWAPCHAIN;
	}

	swapchain = tswapchain;

	format = bestFormat.format;
	extent = bestExtent;

	images = context->primary_logical_device.getSwapchainImagesKHR(swapchain);

	image_views.resize(images.size());

	for (size_t i = 0; i < images.size(); i++) {
		vk::ImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.image = images[i];
		imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
		imageViewCreateInfo.format = format;
		imageViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		vk::ImageView imageView = context->primary_logical_device.createImageView(imageViewCreateInfo);

		if (!imageView) {
			image_views.clear();
			return Error::FAIL_CREATE_IMAGE_VIEW;
		}

		image_views[i] = imageView;
	}

	/*
	* Create render pass
	*/

	vk::AttachmentDescription colorAttachment;
	colorAttachment.format = format;
	colorAttachment.samples = vk::SampleCountFlagBits::e1;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

	vk::AttachmentReference colorAttachmentRef;
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	vk::SubpassDependency subpassDependency;
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	subpassDependency.srcAccessMask = (vk::AccessFlagBits)(0);
	subpassDependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	subpassDependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &subpassDependency;

	vk::RenderPass renderPass = context->primary_logical_device.createRenderPass(renderPassInfo);

	if (!renderPass) {
		return Error::FAIL_CREATE_RENDER_PASS;
	}

	render_pass = renderPass;

	/*
	* Create framebuffers
	*/

	framebuffers.resize(image_views.size());

	for (size_t i = 0; i < image_views.size(); i++) {
		vk::ImageView attachments[] = { image_views[i] };

		vk::FramebufferCreateInfo framebufferInfo;
		framebufferInfo.renderPass = render_pass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;

		vk::Framebuffer framebuffer = context->primary_logical_device.createFramebuffer(framebufferInfo);

		if (!framebuffer) {
			return Error::FAIL_CREATE_FRAMEBUFFER;
		}

		framebuffers[i] = framebuffer;
	}


	max_frames_in_flight =  framebuffers.size();

	/*
	* Create synchronization primitives
	*/

	image_available_semaphores.resize(max_frames_in_flight);
	render_finished_semaphores.resize(max_frames_in_flight);
	in_flight_fences.resize(max_frames_in_flight);
	images_in_flight.resize(framebuffers.size(), {});

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
			return Error::FAIL_CREATE_SYNCH_OBJECTS;
		}
	}

	/*
	* Finish initialization
	*/
	
	current_frame = 0;
	is_init = true;

	return Error::OK;
}

Swapchain::Error Swapchain::reinit() {
	deinit();
	return init(context);
}

void Swapchain::deinit() {
	if (is_initialized()) {
		is_init = false;
		
		context->primary_logical_device.waitIdle();

		images_in_flight.clear();

		for (auto fence : in_flight_fences) {
			context->primary_logical_device.destroy(fence);
		}

		in_flight_fences.clear();

		for (auto semaphore : image_available_semaphores) {
			context->primary_logical_device.destroy(semaphore);
		}

		image_available_semaphores.clear();

		for (auto semaphore : render_finished_semaphores) {
			context->primary_logical_device.destroy(semaphore);
		}

		render_finished_semaphores.clear();

		if (!framebuffers.empty()) {
			for (auto framebuffer : framebuffers) {
				context->primary_logical_device.destroy(framebuffer);
			}

			framebuffers.clear();
		}

		if (render_pass) {
			context->primary_logical_device.destroy(render_pass);
		}

		if (!image_views.empty()) {
			for (auto imageView : image_views) {
				context->primary_logical_device.destroy(imageView);
			}

			image_views.clear();
		}

		if (swapchain) {
			context->primary_logical_device.destroy(swapchain);
		}
	}
}

Retval <Swapchain::Frame, Swapchain::Error> Swapchain::prepare_frame() {
	std::array<vk::Fence, 1> currentFences = { in_flight_fences[current_frame] };
	context->primary_logical_device.waitForFences(currentFences, VK_TRUE, UINT64_MAX);

	auto imageIndex = context->primary_logical_device.acquireNextImageKHR(swapchain, UINT64_MAX, image_available_semaphores[current_frame], nullptr);

	if (imageIndex.result != vk::Result::eSuccess) {
		return { {}, Error::FAIL_ACQUIRE_IMAGE };
	}

	if (imageIndex.result == vk::Result::eErrorOutOfDateKHR) {
		return { {}, Error::OUT_OF_DATE };
	}

	if (images_in_flight[current_frame]) {
		std::array<vk::Fence, 1> ImageInFlightFences = { images_in_flight[current_frame] };
		context->primary_logical_device.waitForFences(ImageInFlightFences, VK_TRUE, UINT64_MAX);
		images_in_flight[current_frame] = nullptr;
	}

	context->primary_logical_device.resetFences(currentFences);

	return {
		image_available_semaphores[current_frame],
		render_finished_semaphores[current_frame],
		in_flight_fences[current_frame],
		imageIndex.value
	};
}

Swapchain::Error Swapchain::present_frame(Frame Frame) {
	Error retval = Error::OK;

	images_in_flight[Frame.id] = in_flight_fences[current_frame];

	vk::PresentInfoKHR presentInfo;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &Frame.render_finished_semaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &Frame.id;
	presentInfo.pResults = nullptr;

	vk::Result presentStatus = context->present_queue.presentKHR(presentInfo);

	if (presentStatus != vk::Result::eSuccess) {
		retval = Error::FAIL_PRESENT_SWAPCHAIN;
	}

	current_frame = (current_frame + 1) % max_frames_in_flight;

	return retval;
}