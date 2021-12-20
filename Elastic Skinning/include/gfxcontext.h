#pragma once

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

class GfxContext {

public:

	~GfxContext();

	void init(const std::string& AppName = "");
	void deinit();

private:
	bool is_initialized{ false };

	SDL_Window* window;
	vk::Instance vulkan_instance;
	vk::DispatchLoaderDynamic instance_extension_loader;
	vk::DebugUtilsMessengerEXT debug_messenger;
	vk::PhysicalDevice primary_physical_device;
	vk::Device primary_logical_device;
	vk::Queue primary_queue;
	vk::Queue present_queue;

	// Swapchain context
	vk::SurfaceKHR render_surface;
	vk::SwapchainKHR render_swapchain;
	vk::Format swapchain_format;
	vk::Extent2D swapchain_extent;
	std::vector<vk::Image> swapchain_images;
	std::vector<vk::ImageView> swapchain_image_views;

};