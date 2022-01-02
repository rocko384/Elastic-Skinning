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

	~GfxPipeline();

	void set_vertex_shader(std::filesystem::path path);
	void set_tessellation_control_shader(std::filesystem::path path);
	void set_tessellation_eval_shader(std::filesystem::path path);
	void set_geometry_shader(std::filesystem::path path);
	void set_fragment_shader(std::filesystem::path path);

	Error init(GfxContext* Context, Swapchain* Swapchain);
	void deinit();

	bool is_initialized() { return is_init; }

	vk::PipelineLayout pipeline_layout;
	vk::Pipeline pipeline;
private:

	bool is_init{ false };

	GfxContext* context{ nullptr };

	std::filesystem::path vertex_shader_path;
	std::filesystem::path tessellation_control_shader_path;
	std::filesystem::path tessellation_eval_shader_path;
	std::filesystem::path geometry_shader_path;
	std::filesystem::path fragment_shader_path;
};