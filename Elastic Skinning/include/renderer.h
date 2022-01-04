#pragma once

#include "util.h"
#include "gfxcontext.h"
#include "swapchain.h"
#include "gfxpipeline.h"

#include <vulkan/vulkan.hpp>

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <cstdint>

class Renderer {

public:

	enum class Error {
		OK,
		PIPELINE_WITH_NAME_ALREADY_EXISTS,
		PIPELINE_INIT_ERROR
	};

	~Renderer();

	void init(GfxContext* Context);
	void deinit();

	bool is_initialized() { return is_init; }

	Error register_pipeline(const std::string& Name, GfxPipeline&& Pipeline);
	Error register_pipeline(StringHash Name, GfxPipeline&& Pipeline);

	void draw_frame();

private:

	bool should_render();

	void record_command_buffers();
	void reset_command_buffers();

	void window_resized_callback(size_t w, size_t h);
	void window_minimized_callback();
	void window_maximized_callback();
	void window_restored_callback();

	bool is_init{ false };
	bool is_first_render{ true };

	GfxContext* context{ nullptr };
	Swapchain render_swapchain;

	std::unordered_map<StringHash, GfxPipeline> pipelines;

	struct CommandBufferWithMutex {
		vk::CommandBuffer command_bufer;
		std::mutex mutex;
	};

	vk::CommandPool command_pool;
	std::vector<CommandBufferWithMutex> render_command_buffers;
	bool are_command_buffers_recorded{ false };

	size_t max_frames_in_flight{ 0 };
	size_t current_frame{ 0 };

	std::vector<vk::Semaphore> image_available_semaphores;
	std::vector<vk::Semaphore> render_finished_semaphores;
	std::vector<vk::Fence> in_flight_fences;
	std::vector<vk::Fence> images_in_flight;

};