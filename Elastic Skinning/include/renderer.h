#pragma once

#include "util.h"
#include "gfxcontext.h"
#include "swapchain.h"
#include "gfxpipeline.h"

#include <vulkan/vulkan.hpp>

#include <vector>
#include <cstdint>

class Renderer {

public:

	~Renderer();

	void init(GfxContext* Context);
	void deinit();

	bool is_initialized() { return is_init; }

	void draw_frame();

private:

	bool should_render();

	void window_resized_callback(size_t w, size_t h);
	void window_minimized_callback();
	void window_restored_callback();

	bool is_init{ false };

	GfxContext* context{ nullptr };
	Swapchain render_swapchain;

	GfxPipeline pipeline;

	vk::CommandPool command_pool;
	std::vector<vk::CommandBuffer> render_command_buffers;

	size_t max_frames_in_flight{ 0 };
	size_t current_frame{ 0 };

	std::vector<vk::Semaphore> image_available_semaphores;
	std::vector<vk::Semaphore> render_finished_semaphores;
	std::vector<vk::Fence> in_flight_fences;
	std::vector<vk::Fence> images_in_flight;

};