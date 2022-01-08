#pragma once

#include "gfxcontext.h"
#include "swapchain.h"
#include "renderingtypes.h"
#include "mesh.h"

#include <vulkan/vulkan.hpp>

#include <filesystem>
#include <vector>
#include <unordered_map>

struct GfxPipelineImpl {

	enum class Error {
		OK,
		INVALID_CONTEXT,
		UNINITIALIZED_CONTEXT,
		INVALID_SWAPCHAIN,
		UNINITIALIZED_SWAPCHAIN,
		NO_SHADERS,
		FAIL_CREATE_DESCRIPTOR_SET_LAYOUT,
		FAIL_CREATE_PIPELINE_LAYOUT,
		FAIL_CREATE_PIPELINE
	};

	GfxPipelineImpl() = default;
	GfxPipelineImpl(GfxPipelineImpl&& rhs) noexcept = default;
	~GfxPipelineImpl();

	GfxPipelineImpl& operator=(GfxPipelineImpl&& rhs) noexcept = default;

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

	vk::DescriptorSetLayout descriptor_set_layout;
	vk::PipelineLayout pipeline_layout;
	vk::Pipeline pipeline;

	vk::PushConstantRange mesh_id_push_constant{ vk::ShaderStageFlagBits::eVertex, 0, sizeof(uint32_t) };

	std::vector<StringHash> buffer_type_names;
	std::unordered_map<StringHash, vk::DescriptorSetLayoutBinding> buffer_layout_bindings;

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


template <VertexType Vtx, BufferObjectType... Buf>
struct GfxPipeline : public GfxPipelineImpl {

	GfxPipeline() {
		vertex_binding_description = Vtx::binding_description();

		vertex_attribute_descriptions.resize(Vtx::attribute_description().size());
		for (size_t i = 0; i < vertex_attribute_descriptions.size(); i++) {
			vertex_attribute_descriptions[i] = Vtx::attribute_description()[i];
		}

		std::vector<StringHash> names = { (Buf::name())... };
		std::vector<vk::DescriptorSetLayoutBinding> bindings = { (Buf::layout_binding())... };

		for (size_t i = 0; i < names.size(); i++) {
			buffer_layout_bindings[names[i]] = bindings[i];
		}

		buffer_type_names = names;
	}
};