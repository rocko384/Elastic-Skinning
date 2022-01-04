#pragma once

#include "gfxcontext.h"
#include "swapchain.h"

#include <vulkan/vulkan.hpp>

#include <filesystem>

struct GfxPipeline {

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

	GfxPipeline() = default;
	GfxPipeline(GfxPipeline&& rhs) noexcept;
	~GfxPipeline();

	GfxPipeline& operator=(GfxPipeline&& rhs) noexcept;

	GfxPipeline& set_vertex_shader(std::filesystem::path path);
	GfxPipeline& set_tessellation_control_shader(std::filesystem::path path);
	GfxPipeline& set_tessellation_eval_shader(std::filesystem::path path);
	GfxPipeline& set_geometry_shader(std::filesystem::path path);
	GfxPipeline& set_fragment_shader(std::filesystem::path path);
	GfxPipeline& set_target(/* RenderTarget Target */); // TODO: Implement other possible render targets

	Error init(GfxContext* Context, Swapchain* Swapchain);
	Error reinit();
	void deinit();

	bool is_initialized() { return is_init; }
	bool is_swapchain_dependent() { return should_init_with_swapchain; };

	vk::PipelineLayout pipeline_layout;
	vk::Pipeline pipeline;

private:

	bool is_init{ false };
	bool should_init_with_swapchain{ true };

	GfxContext* context{ nullptr };
	Swapchain* swapchain{ nullptr };

	std::filesystem::path vertex_shader_path;
	std::filesystem::path tessellation_control_shader_path;
	std::filesystem::path tessellation_eval_shader_path;
	std::filesystem::path geometry_shader_path;
	std::filesystem::path fragment_shader_path;
};