#include "swapchain.h"

#include <algorithm>
#include <array>

Swapchain::~Swapchain() {
	deinit();
}

Swapchain::Error Swapchain::init(GfxContext* context) {
	creator = context->primary_logical_device;
	
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

	vk::SwapchainKHR tswapchain = creator.createSwapchainKHR(swapchainCreateInfo);

	if (!tswapchain) {
		return Error::FAIL_CREATE_SWAPCHAIN;
	}

	swapchain = tswapchain;

	format = bestFormat.format;
	extent = bestExtent;

	images = creator.getSwapchainImagesKHR(swapchain);

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

		vk::ImageView imageView = creator.createImageView(imageViewCreateInfo);

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

	vk::RenderPass renderPass = creator.createRenderPass(renderPassInfo);

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

		vk::Framebuffer framebuffer = creator.createFramebuffer(framebufferInfo);

		if (!framebuffer) {
			return Error::FAIL_CREATE_FRAMEBUFFER;
		}

		framebuffers[i] = framebuffer;
	}

	/*
	* Finish initialization
	*/
	
	is_init = true;

	return Error::OK;
}

void Swapchain::deinit() {
	if (is_initialized()) {
		if (!framebuffers.empty()) {
			for (auto framebuffer : framebuffers) {
				creator.destroy(framebuffer);
			}

			framebuffers.clear();
		}

		if (render_pass) {
			creator.destroy(render_pass);
		}

		if (!image_views.empty()) {
			for (auto imageView : image_views) {
				creator.destroy(imageView);
			}
		}

		if (swapchain) {
			creator.destroy(swapchain);
		}
	}

	is_init = false;
}