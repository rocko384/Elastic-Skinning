#pragma once

#include "util.h"
#include "asset.h"
#include "swapchaincontext.h"

#include <vulkan/vulkan.hpp>
#include <SDL.h>
#include <SDL_vulkan.h>

#include <string>
#include <vector>

#define REQUIRED_VULKAN_EXTENSIONS 

#define REQUIRED_DEVICE_EXTENSIONS \
	VK_KHR_SWAPCHAIN_EXTENSION_NAME

#define DEBUG_VULKAN_EXTENSIONS \
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME

#define VULKAN_VALIDATION_LAYERS \
	"VK_LAYER_KHRONOS_validation"

class Renderer;

class GfxContext {

	friend class Renderer;

public:

	~GfxContext();

	void init(const std::string& AppName = "");
	void deinit();

	bool is_initialized() { return is_init; }

	SwapchainContext render_swapchain_context;

private:

	vk::ShaderModule create_shader_module(std::filesystem::path path);
	vk::ShaderModule create_shader_module(const BinaryBlob& code);

	bool is_init{ false };

	SDL_Window* window;
	vk::Instance vulkan_instance;
	vk::DispatchLoaderDynamic instance_extension_loader;
	vk::DebugUtilsMessengerEXT debug_messenger;
	vk::SurfaceKHR render_surface;
	vk::PhysicalDevice primary_physical_device;
	vk::Device primary_logical_device;
	vk::Queue primary_queue;
	vk::Queue present_queue;
};