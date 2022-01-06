#pragma once

#include "util.h"
#include "gfxcontext.h"
#include "swapchain.h"
#include "renderingtypes.h"
#include "gfxpipeline.h"
#include "mesh.h"

#include <vulkan/vulkan.hpp>

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <cstdint>

class RendererImpl {

public:

	enum class Error {
		OK,
		PIPELINE_WITH_NAME_ALREADY_EXISTS,
		PIPELINE_INIT_ERROR,
		FAILED_TO_ALLOCATE_BUFFER,
		FAILED_TO_ALLOCATE_COMMAND_BUFFERS
	};

	using MeshId = uint32_t;

	~RendererImpl();

	void init(GfxContext* Context);
	void deinit();

	bool is_initialized() { return is_init; }

	Error register_pipeline(const std::string& Name, GfxPipelineImpl&& Pipeline);
	Error register_pipeline(StringHash Name, GfxPipelineImpl&& Pipeline);

	Retval<MeshId, Error> digest_mesh(Mesh Mesh, ModelTransform* Transform);

	void set_camera(Camera* Camera);

	void draw_frame();

protected:

	bool should_render();

	void update_frame_data(uint32_t ImageIdx);

	void finish_mesh_digestion();

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

	vk::DescriptorPool descriptor_pool;
	// descriptor_sets[Pipeline Name][Swapchain Image #]
	std::unordered_map<StringHash, std::vector<vk::DescriptorSet>> descriptor_sets;

	std::vector<StringHash> ubo_type_names;
	std::unordered_map<StringHash, size_t> ubo_type_sizes;
	std::unordered_map<StringHash, bool> ubo_type_is_per_mesh;
	// frame_data_buffers[Buffer Type][Swapchain Image #]
	std::unordered_map<StringHash, std::vector<BufferAllocation>> frame_data_buffers;

	struct InternalMesh {
		StringHash pipeline_hash{ 0 };

		size_t vertex_count{ 0 };
		BufferAllocation vertex_buffer;

		size_t index_count{ 0 };
		BufferAllocation index_buffer;
	};

	std::vector<InternalMesh> meshes;
	std::vector<ModelTransform*> mesh_transforms;

	Camera* current_camera{ nullptr };

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

template <BufferObjectType... SupportedBufferTypes>
class Renderer : public RendererImpl {
	
public:

	Renderer() {
		std::vector<StringHash> names = { (SupportedBufferTypes::name())... };
		std::vector<size_t> sizes = { (sizeof(SupportedBufferTypes))... };
		std::vector<bool> perMesh = { (SupportedBufferTypes::is_per_mesh())... };

		for (size_t i = 0; i < names.size(); i++) {
			ubo_type_sizes[names[i]] = sizes[i];
			ubo_type_is_per_mesh[names[i]] = perMesh[i];
		}

		ubo_type_names = names;
	}
};