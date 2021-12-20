#include "util.h"
#include "gfxcontext.h"

#include <algorithm>
#include <set>
#include <optional>
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

int scoreDevice(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
	auto properties = device.getProperties();
	auto memoryProperties = device.getMemoryProperties();
	auto features = device.getFeatures();
	auto extensions = device.enumerateDeviceExtensionProperties();

	auto surfaceCapabilities = device.getSurfaceCapabilitiesKHR(surface);
	auto surfaceFormats = device.getSurfaceFormatsKHR(surface);
	auto surfacePresentModes = device.getSurfacePresentModesKHR(surface);

	int result = 0;


	// Test for required extensions
	std::vector<std::string> requiredDeviceExtensions = { REQUIRED_DEVICE_EXTENSIONS };

	for (auto extension : requiredDeviceExtensions) {
		if (extensions.end() == std::find_if(
				extensions.begin(),
				extensions.end(),
				[extension](vk::ExtensionProperties e) {
					return extension == e.extensionName;
				})
			) {
			return 0;
		}
	}

	// Test for required features
	if (!features.geometryShader) {
		return 0;
	}

	// Test for adequate swapchain
	if (surfaceFormats.empty() || surfacePresentModes.empty()) {
		return 0;
	}

	// Score based on heuristics
	if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
		result += 2000;
	}
	else if (properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu
			|| properties.deviceType == vk::PhysicalDeviceType::eVirtualGpu) {
		result += 1000;
	}

	int approxVRAMGB = memoryProperties.memoryHeaps[0].size / 1'000'000'000;

	result += approxVRAMGB;

	return result;
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
	createInfo.enabledExtensionCount = vkRequiredExtensions.size();
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


	/*
	* Create render surface
	*/

	VkSurfaceKHR surface;

	if (!SDL_Vulkan_CreateSurface(window, vulkan_instance, &surface)) {
		LOG_ERROR("Failed to create window surface");
		return;
	}

	render_surface = surface;

	/*
	* Device Selection
	*/

	std::vector<vk::PhysicalDevice> physicalDevices = vulkan_instance.enumeratePhysicalDevices();

	if (physicalDevices.size() == 0) {
		LOG_ERROR("No physical Vulkan devices found");
		return;
	}

	vk::PhysicalDevice bestDevice = physicalDevices[0];

	for (auto device : physicalDevices) {
		if (scoreDevice(device, render_surface) > scoreDevice(bestDevice, render_surface)) {
			bestDevice = device;
		}
	}

	if (scoreDevice(bestDevice, render_surface) == 0) {
		LOG_ERROR("Failed to find suitable device");
		return;
	}

	primary_physical_device = bestDevice;

	/*
	* Queue Selection
	*/

	std::vector<vk::QueueFamilyProperties> queueProperties = bestDevice.getQueueFamilyProperties();

	std::optional<uint32_t> primaryQueueFamilyIndex;
	std::optional<uint32_t> presentQueueFamilyIndex;

	// Attempt to find a queue for graphics, compute, and memory commands
	for (uint32_t idx = 0; idx < queueProperties.size(); idx++) {
		if (queueProperties[idx].queueFlags & vk::QueueFlagBits::eGraphics
			&& queueProperties[idx].queueFlags & vk::QueueFlagBits::eCompute
			&& queueProperties[idx].queueFlags & vk::QueueFlagBits::eTransfer) {
			primaryQueueFamilyIndex = idx;
			break;
		}
	}

	// Attempt to find a queue for presenting
	for (uint32_t idx = 0; idx < queueProperties.size(); idx++) {
		if (primary_physical_device.getSurfaceSupportKHR(idx, render_surface)) {
			presentQueueFamilyIndex = idx;
			break;
		}
	}

	if (!primaryQueueFamilyIndex.has_value() || !presentQueueFamilyIndex.has_value()) {
		LOG_ERROR("Failed to find appropriate command queues");
		return;
	}

	/*
	* Create logical devices
	*/

	// Define queues
	float queuePriority = 1.0f;
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { primaryQueueFamilyIndex.value(), presentQueueFamilyIndex.value() };

	for (uint32_t queueFamily : uniqueQueueFamilies) {
		vk::DeviceQueueCreateInfo queueCreateInfo;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		queueCreateInfos.push_back(queueCreateInfo);
	}

	// Define actual device
	vk::PhysicalDeviceFeatures deviceFeatures;

	std::vector<const char*> requiredDeviceExtensions = { REQUIRED_DEVICE_EXTENSIONS };

	vk::DeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.setQueueCreateInfos(queueCreateInfos);
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
	deviceCreateInfo.enabledExtensionCount = requiredDeviceExtensions.size();
	deviceCreateInfo.ppEnabledLayerNames = nullptr;
	deviceCreateInfo.enabledLayerCount = 0;

	vk::Device logicalDevice = primary_physical_device.createDevice(deviceCreateInfo);

	if (!logicalDevice) {
		LOG_ERROR("Failed to create logical device");
		return;
	}

	primary_logical_device = logicalDevice;

	primary_queue = logicalDevice.getQueue(primaryQueueFamilyIndex.value(), 0);
	present_queue = logicalDevice.getQueue(presentQueueFamilyIndex.value(), 0);

	/*
	* Create the swapchain
	*/

	/// TODO: Break this out in some way to facilitate dynamic swapchain recreation for window resizing

	auto surfaceCapabilities = primary_physical_device.getSurfaceCapabilitiesKHR(render_surface);
	auto surfaceFormats = primary_physical_device.getSurfaceFormatsKHR(render_surface);
	auto surfacePresentModes = primary_physical_device.getSurfacePresentModesKHR(render_surface);

	vk::SurfaceFormatKHR bestFormat = surfaceFormats[0].format;

	for (auto format : surfaceFormats) {
		if (format.format == vk::Format::eB8G8R8A8Srgb
			&& format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			bestFormat = format;
			break;
		}
	}

	vk::PresentModeKHR bestPresentMode = vk::PresentModeKHR::eFifo;

	for (auto mode : surfacePresentModes) {
		if (mode == vk::PresentModeKHR::eMailbox) {
			bestPresentMode = vk::PresentModeKHR::eMailbox;
			break;
		}
	}

	vk::Extent2D bestExtent;

	if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
		bestExtent = surfaceCapabilities.currentExtent;
	}
	else {
		int width, height;
		SDL_Vulkan_GetDrawableSize(window, &width, &height);

		vk::Extent2D actualExtent {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(
			actualExtent.width,
			surfaceCapabilities.minImageExtent.width,
			surfaceCapabilities.maxImageExtent.width
		);

		actualExtent.height = std::clamp(
			actualExtent.height,
			surfaceCapabilities.minImageExtent.height,
			surfaceCapabilities.maxImageExtent.height
		);

		bestExtent = actualExtent;
	}
	
	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
	
	imageCount = std::clamp(imageCount, imageCount, surfaceCapabilities.maxImageCount);
	
	vk::SwapchainCreateInfoKHR swapchainCreateInfo;
	swapchainCreateInfo.surface = render_surface;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = bestFormat.format;
	swapchainCreateInfo.imageColorSpace = bestFormat.colorSpace;
	swapchainCreateInfo.imageExtent = bestExtent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

	uint32_t queueFamilyIndices[] = { primaryQueueFamilyIndex.value(), presentQueueFamilyIndex.value() };

	if (primaryQueueFamilyIndex.value() != presentQueueFamilyIndex.value()) {
		swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	swapchainCreateInfo.presentMode = bestPresentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	vk::SwapchainKHR swapchain = primary_logical_device.createSwapchainKHR(swapchainCreateInfo);
	
	if (!swapchain) {
		LOG_ERROR("Failed to create swapchain");
		return;
	}

	render_swapchain = swapchain;

	swapchain_format = bestFormat.format;
	swapchain_extent = bestExtent;

	swapchain_images = primary_logical_device.getSwapchainImagesKHR(render_swapchain);

	swapchain_image_views.resize(swapchain_images.size());

	for (size_t i = 0; i < swapchain_images.size(); i++) {
		vk::ImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.image = swapchain_images[i];
		imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
		imageViewCreateInfo.format = swapchain_format;
		imageViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		vk::ImageView imageView = primary_logical_device.createImageView(imageViewCreateInfo);

		if (!imageView) {
			LOG_ERROR("Failed to create swapchain image view");
			swapchain_image_views.clear();
			return;
		}

		swapchain_image_views[i] = imageView;
	}

	/*
	* Finish initialization
	*/

	is_initialized = true;
}

void GfxContext::deinit() {
	if (is_initialized) {
		if (!swapchain_image_views.empty()) {
			for (auto imageView : swapchain_image_views) {
				primary_logical_device.destroy(imageView);
			}
		}

		if (render_swapchain) {
			primary_logical_device.destroy(render_swapchain);
		}

		if (primary_logical_device) {
			primary_logical_device.destroy();
		}

		if (debug_messenger) {
			vulkan_instance.destroy(debug_messenger, nullptr, instance_extension_loader);
		}

		if (render_surface) {
			vulkan_instance.destroy(render_surface);
		}

		vulkan_instance.destroy();

		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	is_initialized = false;
}