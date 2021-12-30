#pragma once

#include "util.h"
#include "gfxcontext.h"

#include <vulkan/vulkan.hpp>

class Renderer {

public:

	~Renderer();

	void init(GfxContext* Context);
	void deinit();

	bool is_initialized() { return is_init; }

private:

	bool is_init{ false };

	GfxContext* context{ nullptr };

	vk::RenderPass render_pass;
	vk::PipelineLayout pipeline_layout;
	vk::Pipeline pipeline;

};