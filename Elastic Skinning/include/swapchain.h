#pragma once

#include "gfxcontext.h"

#include <vulkan/vulkan.hpp>
#include <SDL.h>
#include <SDL_vulkan.h>

#include <vector>
#include <cstdint>

struct Swapchain {

	enum class Error {
		OK,
		INVALID_CONTEXT,
		UNINITIALIZED_CONTEXT,
		FAIL_CREATE_SWAPCHAIN,
		FAIL_CREATE_IMAGE_VIEW,
		FAIL_CREATE_SYNCH_OBJECTS,
		FAIL_ACQUIRE_IMAGE,
		OUT_OF_DATE,
		FAIL_PRESENT_SWAPCHAIN
	};

	using FrameId = uint32_t;

	~Swapchain();

	Error init(GfxContext* Context);
	Error reinit();
	void deinit();

	bool is_initialized() { return is_init; }

	size_t size() {
		assert(images.size() == image_views.size());
		return images.size();
	}

	struct Frame {
		vk::Semaphore image_available_semaphore;
		vk::Semaphore render_finished_semaphore;
		vk::Fence fence;

		FrameId id;
	};

	Retval<Frame, Error> prepare_frame();
	Error present_frame(Frame Frame);

	vk::SwapchainKHR swapchain;
	vk::Format format;
	vk::Extent2D extent;

	std::vector<vk::Image> images;
	std::vector<vk::ImageView> image_views;

	size_t max_frames_in_flight{ 0 };
	size_t current_frame{ 0 };

private:

	bool is_init{ false };

	GfxContext* context{ nullptr };

	std::vector<vk::Semaphore> image_available_semaphores;
	std::vector<vk::Semaphore> render_finished_semaphores;
	std::vector<vk::Fence> in_flight_fences;
	std::vector<vk::Fence> images_in_flight;
};