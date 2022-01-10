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

	static const StringHash DEFAULT_TEXTURE_NAME = 1;

public:

	enum class Error {
		OK,
		PIPELINE_WITH_NAME_ALREADY_EXISTS,
		PIPELINE_INIT_ERROR,
		FAILED_TO_ALLOCATE_BUFFER,
		FAILED_TO_ALLOCATE_COMMAND_BUFFERS,
		TEXTURE_WITH_NAME_ALREADY_EXISTS
	};

	using MeshId = uint32_t;

	~RendererImpl();

	void init(GfxContext* Context);
	void deinit();

	bool is_initialized() { return is_init; }

	Error register_pipeline(const std::string& Name, GfxPipelineImpl& Pipeline);
	Error register_pipeline(StringHash Name, GfxPipelineImpl& Pipeline);

	Error register_texture(const std::string& Name, const Image& Image);
	Error register_texture(StringHash Name, const Image& Imaage);
	Error set_default_texture(const Image& Image);

	Retval<MeshId, Error> digest_mesh(Mesh Mesh, ModelTransform* Transform);

	void set_camera(Camera* Camera);

	void draw_frame();

protected:

	bool should_render();

	void update_frame_data(Swapchain::FrameId ImageIdx);

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

	struct InternalTexture {
		TextureAllocation texture;
		vk::ImageView view;
	};

	std::unordered_map<StringHash, InternalTexture> textures;

	vk::DescriptorPool descriptor_pool;
	// buffer_descriptor_sets[Pipeline Name][Swapchain Image #]
	std::unordered_map<StringHash, std::vector<vk::DescriptorSet>> buffer_descriptor_sets;
	// texture_descriptor_sets[hash_combine(Pipeline Name, Sampler Name, Texture Name)]
	// a la: texture_descriptor_sets[Pipeline Name][Sampler Name][Texture Name]
	std::unordered_map<StringHash, vk::DescriptorSet> texture_descriptor_sets;

	std::vector<StringHash> buffer_type_names;
	std::vector<StringHash> sampler_type_names;
	std::unordered_map<StringHash, size_t> buffer_type_sizes;
	std::unordered_map<StringHash, bool> buffer_type_is_per_mesh;
	// frame_data_buffers[Buffer Type][Swapchain Image #]
	std::unordered_map<StringHash, std::vector<BufferAllocation>> frame_data_buffers;

	vk::Sampler texture_sampler;

	struct InternalMesh {
		BufferAllocation vertex_buffer;
		BufferAllocation index_buffer;

		StringHash pipeline_hash{ 0 };
		StringHash color_texture_hash{ DEFAULT_TEXTURE_NAME };

		size_t vertex_count{ 0 };
		size_t index_count{ 0 };
	};

	std::vector<InternalMesh> meshes;
	std::vector<ModelTransform*> mesh_transforms;

	Camera* current_camera{ nullptr };

	vk::CommandPool command_pool;
	std::vector<vk::CommandBuffer> primary_render_command_buffers;
	bool are_command_buffers_recorded{ false };

};

template <DescriptorType... SupportedDescriptors>
class Renderer : public RendererImpl {
	
public:

	Renderer() {

		std::vector<StringHash> names = { (SupportedDescriptors::name())... };
		std::vector<bool> isBuffer = { (BufferObjectType<SupportedDescriptors>)... };
		std::vector<bool> isPerMeshTemp = { (is_per_mesh_impl<SupportedDescriptors>())... };
		std::vector<size_t> sizesTemp = { (sizeof(SupportedDescriptors))... };

		std::vector<StringHash> bufferNames;
		std::vector<bool> perMesh;
		std::vector<size_t> sizes;
		std::vector<StringHash> samplerNames;

		for (size_t i = 0; i < names.size(); i++) {
			if (isBuffer[i]) {
				bufferNames.push_back(names[i]);
				perMesh.push_back(isPerMeshTemp[i]);
				sizes.push_back(sizesTemp[i]);
			}
			else {
				samplerNames.push_back(names[i]);
			}
		}

		for (size_t i = 0; i < bufferNames.size(); i++) {
			buffer_type_sizes[bufferNames[i]] = sizes[i];
			buffer_type_is_per_mesh[bufferNames[i]] = perMesh[i];
		}

		buffer_type_names = bufferNames;
		sampler_type_names = samplerNames;
	}

private:

	template <BufferObjectType T>
	bool is_per_mesh_impl() {
		return T::is_per_mesh();
	}

	template <SamplerType T>
	bool is_per_mesh_impl() {
		return false;
	}

};