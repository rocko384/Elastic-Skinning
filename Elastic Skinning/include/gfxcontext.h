#pragma once

#include "util.h"
#include "asset.h"
#include "window.h"

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include <SDL.h>
#include <SDL_vulkan.h>

#include <string>
#include <vector>
#include <filesystem>

#define REQUIRED_VULKAN_EXTENSIONS 

#define REQUIRED_DEVICE_EXTENSIONS \
	VK_KHR_SWAPCHAIN_EXTENSION_NAME

#define DEBUG_VULKAN_EXTENSIONS \
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME

#define VULKAN_VALIDATION_LAYERS \
	"VK_LAYER_KHRONOS_validation"

class GfxContext {

	friend class Renderer;
	friend class Swapchain;
	friend class GfxPipelineImpl;

public:

	~GfxContext();

	void init(Window* Window, const std::string& AppName = "", const std::string& EngineName = "No Engine");
	void deinit();

	bool is_initialized() { return is_init; }

	void transfer_buffer_memory(vk::Buffer Dest, vk::Buffer Source, vk::DeviceSize Size);
	void upload_to_gpu_buffer(vk::Buffer Dest, void* Source, size_t Size);

	vk::ShaderModule create_shader_module(std::filesystem::path path);
	vk::ShaderModule create_shader_module(const BinaryBlob& code);

	VmaAllocator allocator;

private:

	bool is_init{ false };

	Window* window;

	vk::Instance vulkan_instance;
	vk::DispatchLoaderDynamic instance_extension_loader;
	vk::DebugUtilsMessengerEXT debug_messenger;
	vk::SurfaceKHR render_surface;
	vk::PhysicalDevice primary_physical_device;
	vk::Device primary_logical_device;

	vk::Queue primary_queue;
	uint32_t primary_queue_family_index;
	vk::Queue present_queue;
	uint32_t present_queue_family_index;

	vk::CommandPool memory_transfer_command_pool;
};