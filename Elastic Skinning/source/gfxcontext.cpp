#include "util.h"
#include "gfxcontext.h"

#include <vector>
#include <cstdint>

GfxContext::~GfxContext() {
	deinit();
}

void GfxContext::init(const std::string& AppName) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
		LOG_ERROR("SDL failed to init");
		return;
	}

	window = SDL_CreateWindow(
		AppName.c_str(),
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		1280,
		720,
		SDL_WINDOW_VULKAN
	);

	if (window == nullptr) {
		LOG_ERROR("Failed to create window");
		return;
	}

	uint32_t vkExtensionCount;
	std::vector<const char*> vkExtensions = {};
	uint32_t existingExtensionCount = vkExtensions.size();

	if (!SDL_Vulkan_GetInstanceExtensions(window, &vkExtensionCount, nullptr)) {
		LOG_ERROR("Failed to get required Vulkan extensions for SDL");
		return;
	}

	vkExtensions.resize(vkExtensionCount + vkExtensions.size());

	if (!SDL_Vulkan_GetInstanceExtensions(window, &vkExtensionCount, vkExtensions.data() + existingExtensionCount)) {
		LOG_ERROR("Failed to get required Vulkan extensions for SDL");
		return;
	}

	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = AppName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	vk::InstanceCreateInfo createInfo;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = vkExtensionCount;
	createInfo.ppEnabledExtensionNames = vkExtensions.data();
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;

	vulkan_instance = vk::createInstance(createInfo);

	if (!vulkan_instance) {
		LOG_ERROR("Failed to create Vulkan instance");
		return;
	}

	is_initialized = true;
}

void GfxContext::deinit() {
	if (is_initialized) {
		vulkan_instance.destroy();

		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	is_initialized = false;
}