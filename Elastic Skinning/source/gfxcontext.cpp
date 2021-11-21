#include "util.h"
#include "gfxcontext.h"

#include <algorithm>
#include <vector>
#include <cstdint>
#include <cstring>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		LOG_ERROR("Validation Layer: %s\n", pCallbackData->pMessage);
	}

	return VK_FALSE;
}


GfxContext::~GfxContext() {
	deinit();
}

void GfxContext::init(const std::string& AppName) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
		LOG_ERROR("SDL failed to init");
		return;
	}

	/*
	* Window Creation
	*/

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

	/*
	* Vulkan Extension Establishment
	*/

	uint32_t vkRequiredExtensionCount;
	std::vector<const char*> vkRequiredExtensions = { REQUIRED_VULKAN_EXTENSIONS };

#ifdef _DEBUG

	std::vector<const char*> vkDebugExtensions = { DEBUG_VULKAN_EXTENSIONS };
	vkRequiredExtensions.insert(vkRequiredExtensions.end(), vkDebugExtensions.begin(), vkDebugExtensions.end());

#endif


	uint32_t nonSDLRequiredExtensionCount = vkRequiredExtensions.size();

	if (!SDL_Vulkan_GetInstanceExtensions(window, &vkRequiredExtensionCount, nullptr)) {
		LOG_ERROR("Failed to get required Vulkan extensions for SDL");
		return;
	}

	vkRequiredExtensions.resize(vkRequiredExtensionCount + vkRequiredExtensions.size());

	if (!SDL_Vulkan_GetInstanceExtensions(window, &vkRequiredExtensionCount, vkRequiredExtensions.data() + nonSDLRequiredExtensionCount)) {
		LOG_ERROR("Failed to get required Vulkan extensions for SDL");
		return;
	}

	std::vector<vk::ExtensionProperties> supportedExtensions = vk::enumerateInstanceExtensionProperties();

	LOG("Available Vulkan extensions:\n");

	for (auto& e : supportedExtensions) {
		LOG("\t%s\n", static_cast<const char*>(e.extensionName));
	}

	LOG("\n");

	bool hasAllRequiredExtensions = std::all_of(
		vkRequiredExtensions.begin(),
		vkRequiredExtensions.end(),
		[&supportedExtensions](const char* ExtensionName) -> bool {
			return supportedExtensions.end() != std::find_if(
				supportedExtensions.begin(),
				supportedExtensions.end(),
				[ExtensionName](vk::ExtensionProperties e) -> bool {
					return std::strcmp(ExtensionName, e.extensionName) == 0;
				}
			);
		}
	);

	if (!hasAllRequiredExtensions) {
		LOG_ERROR("Vulkan instance is missing necessary extensions");
		return;
	}

	/*
	* Vulkan Validation Layer Establishment
	*/

	std::vector<const char*> vkRequiredValidationLayers = {};

#ifdef _DEBUG

	vkRequiredValidationLayers = std::vector{ VULKAN_VALIDATION_LAYERS };

	std::vector<vk::LayerProperties> supportedLayers = vk::enumerateInstanceLayerProperties();

	LOG("Available Vulkan validation layers:\n");

	for (auto& l : supportedLayers) {
		LOG("\t%s\n", static_cast<const char*>(l.layerName));
	}

	LOG("\n");

	bool hasAllRequiredLayers = std::all_of(
		vkRequiredValidationLayers.begin(),
		vkRequiredValidationLayers.end(),
		[&supportedLayers](const char* LayerName) -> bool {
			return supportedLayers.end() != std::find_if(
				supportedLayers.begin(),
				supportedLayers.end(),
				[LayerName](vk::LayerProperties l) -> bool {
					return std::strcmp(LayerName, l.layerName) == 0;
				}
			);
		}
	);

	if (!hasAllRequiredLayers) {
		LOG_ERROR("Vulkan instance is missing necessary validation layers");
		return;
	}

#endif

	/*
	* Vulkan Instance Creation
	*/

	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = AppName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	vk::InstanceCreateInfo createInfo;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = vkRequiredExtensionCount;
	createInfo.ppEnabledExtensionNames = vkRequiredExtensions.data();
	createInfo.enabledLayerCount = vkRequiredValidationLayers.size();
	createInfo.ppEnabledLayerNames = vkRequiredValidationLayers.size() == 0 ? nullptr : vkRequiredValidationLayers.data();

	vulkan_instance = vk::createInstance(createInfo);

	if (!vulkan_instance) {
		LOG_ERROR("Failed to create Vulkan instance");
		return;
	}

	/*
	* InstanceExtension Loader Creation
	*/

	instance_extension_loader.init(vulkan_instance, vkGetInstanceProcAddr);

	/*
	* Debug Callback Setup
	*/

#ifdef _DEBUG
	
	vk::DebugUtilsMessengerCreateInfoEXT messengerCreateInfo;
	messengerCreateInfo.messageSeverity =
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
	messengerCreateInfo.messageType =
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
		| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
		| vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
	messengerCreateInfo.pfnUserCallback = debugCallback;
	messengerCreateInfo.pUserData = nullptr;

	debug_messenger = vulkan_instance.createDebugUtilsMessengerEXT(messengerCreateInfo, nullptr, instance_extension_loader);

	if (!debug_messenger) {
		LOG_ERROR("Failed to create debug messenger");
		return;
	}

#endif

	is_initialized = true;
}

void GfxContext::deinit() {
	if (is_initialized) {
		if (debug_messenger) {
			vulkan_instance.destroy(debug_messenger, nullptr, instance_extension_loader);
		}

		vulkan_instance.destroy();

		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	is_initialized = false;
}