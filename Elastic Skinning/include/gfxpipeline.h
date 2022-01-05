#pragma once

#include "gfxcontext.h"
#include "swapchain.h"
#include "mesh.h"

#include <vulkan/vulkan.hpp>

#include <filesystem>

struct GfxPipelineImpl {

	enum class Error {
		OK,
		INVALID_CONTEXT,
		UNINITIALIZED_CONTEXT,
		INVALID_SWAPCHAIN,
		UNINITIALIZED_SWAPCHAIN,
		NO_SHADERS,
		FAIL_CREATE_PIPELINE_LAYOUT,
		FAIL_CREATE_PIPELINE
	};

	GfxPipelineImpl() = default;
	GfxPipelineImpl(GfxPipelineImpl&& rhs) noexcept;
	~GfxPipelineImpl();

	GfxPipelineImpl& operator=(GfxPipelineImpl&& rhs) noexcept;

	GfxPipelineImpl& set_vertex_shader(std::filesystem::path path);
	GfxPipelineImpl& set_tessellation_control_shader(std::filesystem::path path);
	GfxPipelineImpl& set_tessellation_eval_shader(std::filesystem::path path);
	GfxPipelineImpl& set_geometry_shader(std::filesystem::path path);
	GfxPipelineImpl& set_fragment_shader(std::filesystem::path path);
	GfxPipelineImpl& set_target(/* RenderTarget Target */); // TODO: Implement other possible render targets

	Error init(GfxContext* Context, Swapchain* Swapchain);
	Error reinit();
	void deinit();

	bool is_initialized() { return is_init; }
	bool is_swapchain_dependent() { return should_init_with_swapchain; };

	vk::PipelineLayout pipeline_layout;
	vk::Pipeline pipeline;

protected:

	bool is_init{ false };
	bool should_init_with_swapchain{ true };

	GfxContext* context{ nullptr };
	Swapchain* swapchain{ nullptr };

	vk::VertexInputBindingDescription vertex_binding_description;
	std::vector<vk::VertexInputAttributeDescription> vertex_attribute_descriptions;

	std::filesystem::path vertex_shader_path;
	std::filesystem::path tessellation_control_shader_path;
	std::filesystem::path tessellation_eval_shader_path;
	std::filesystem::path geometry_shader_path;
	std::filesystem::path fragment_shader_path;
};


template <VertexType T>
struct GfxPipeline : public GfxPipelineImpl {

	GfxPipeline() {
		vertex_binding_description = T::binding_description();
		vertex_attribute_descriptions.resize(T::attribute_description().size());
		
		for (size_t i = 0; i < vertex_attribute_descriptions.size(); i++) {
			vertex_attribute_descriptions[i] = T::attribute_description()[i];
		}
	}

};