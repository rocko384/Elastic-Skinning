#pragma once

#include <vulkan/vulkan.hpp>
#include <SDL.h>
#include <SDL_vulkan.h>

#include <string>

#define REQUIRED_VULKAN_EXTENSIONS 

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

};