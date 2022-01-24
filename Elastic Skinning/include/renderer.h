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

protected:

	static const StringHash DEFAULT_TEXTURE_NAME = 1;
	static const StringHash DEPTH_PIPELINE_NAME = 42;

public:

	enum class Error {
		OK,
		PIPELINE_WITH_NAME_ALREADY_EXISTS,
		PIPELINE_HAS_UNSUPPORTED_RENDER_TARGET,
		PIPELINE_INIT_ERROR,
		FAILED_TO_ALLOCATE_BUFFER,
		FAILED_TO_ALLOCATE_COMMAND_BUFFERS,
		TEXTURE_WITH_NAME_ALREADY_EXISTS
	};

	using MeshId = uint32_t;

	RendererImpl(GfxContext* Context);
	~RendererImpl();

	bool is_initialized() { return is_init; }

	Error register_pipeline_impl(const std::string& Name, GfxPipelineImpl& Pipeline);
	Error register_pipeline_impl(StringHash Name, GfxPipelineImpl& Pipeline);

	Error register_texture(const std::string& Name, const Image& Image);
	Error register_texture(StringHash Name, const Image& Imaage);
	Error set_default_texture(const Image& Image);

	Retval<MeshId, Error> digest_mesh(Mesh Mesh, ModelTransform* Transform);

	void set_camera(Camera* Camera);

	void draw_frame();

protected:

	RendererImpl() = default;

	void constructor_impl(GfxContext* Context);

	void create_render_state();
	void destroy_render_state();

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

	struct InternalTexture {
		TextureAllocation texture;
		vk::ImageView view;
	};

	GfxContext* context{ nullptr };
	Swapchain render_swapchain;
	std::vector<InternalTexture> depthbuffers;

	vk::RenderPass geometry_render_pass;
	uint32_t depth_subpass;
	uint32_t color_subpass;

	std::vector<vk::Framebuffer> framebuffers;

	std::unordered_map<StringHash, GfxPipelineImpl> pipelines;

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
		StringHash depth_pipeline_hash{ 0 };
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

	Renderer(GfxContext* Context) {

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

		constructor_impl(Context);
	}

	template <VertexType Vertex, DescriptorType... Descriptors>
	Error register_pipeline(const std::string& Name, GfxPipeline<Vertex, Descriptors...>& Pipeline) {
		return register_pipeline(CRC::crc64(Name), Pipeline);
	}

	template <VertexType Vertex, DescriptorType... Descriptors>
	Error register_pipeline(StringHash Name, GfxPipeline<Vertex, Descriptors...>& Pipeline) {
		typename GfxPipeline<Vertex, Descriptors...>::DepthPipelineType depthPipeline;
		depthPipeline.vertex_shader_path = Pipeline.vertex_shader_path;
		depthPipeline.target = RenderTarget::DepthBuffer;

		Error ret = register_pipeline_impl(Name, Pipeline);
		
		if (ret != Error::OK) {
			return ret;
		}

		return register_pipeline_impl(hash_combine({ Name, RendererImpl::DEPTH_PIPELINE_NAME }), depthPipeline);
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