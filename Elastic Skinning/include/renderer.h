#pragma once

#include "util.h"
#include "gfxcontext.h"
#include "swapchain.h"
#include "gfxpipeline.h"
#include "mesh.h"

#include <vulkan/vulkan.hpp>

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <cstdint>

class Renderer {

public:

	enum class Error {
		OK,
		PIPELINE_WITH_NAME_ALREADY_EXISTS,
		PIPELINE_INIT_ERROR,
		FAILED_TO_ALLOCATE_BUFFER,
		FAILED_TO_ALLOCATE_COMMAND_BUFFERS
	};

	using MeshId = size_t;

	~Renderer();

	void init(GfxContext* Context);
	void deinit();

	bool is_initialized() { return is_init; }

	Error register_pipeline(const std::string& Name, GfxPipelineImpl&& Pipeline);
	Error register_pipeline(StringHash Name, GfxPipelineImpl&& Pipeline);

	Retval<MeshId, Error> digest_mesh(Mesh Mesh);

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

	std::unordered_map<StringHash, GfxPipelineImpl> pipelines;

	struct InternalMesh {
		StringHash pipeline_hash{ 0 };

		size_t vertex_count{ 0 };
		vk::Buffer vertex_buffer{ nullptr };
		VmaAllocation vertex_allocation{ nullptr };

		size_t index_count{ 0 };
		vk::Buffer index_buffer{ nullptr };
		VmaAllocation index_allocation{ nullptr };

		vk::CommandBuffer render_command_buffer{ nullptr };
	};

	std::vector<InternalMesh> meshes;

	vk::CommandPool command_pool;
	std::vector<vk::CommandBuffer> primary_render_command_buffers;
	bool are_command_buffers_recorded{ false };

	size_t max_frames_in_flight{ 0 };
	size_t current_frame{ 0 };

	std::vector<vk::Semaphore> image_available_semaphores;
	std::vector<vk::Semaphore> render_finished_semaphores;
	std::vector<vk::Fence> in_flight_fences;
	std::vector<vk::Fence> images_in_flight;

};