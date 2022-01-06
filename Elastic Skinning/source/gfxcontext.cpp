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

void GfxContext::init(Window* Window, const std::string& AppName, const std::string& EngineName) {
	
	if (Window == nullptr) {
		LOG_ERROR("Window does not exist");
		return;
	}

	if (!Window->is_initialized()) {
		LOG_ERROR("Window is not initialized");
		return;
	}

	window = Window;

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

	if (!SDL_Vulkan_GetInstanceExtensions(window->window, &vkRequiredExtensionCount, nullptr)) {
		LOG_ERROR("Failed to get required Vulkan extensions for SDL");
		return;
	}

	vkRequiredExtensions.resize(vkRequiredExtensionCount + vkRequiredExtensions.size());

	if (!SDL_Vulkan_GetInstanceExtensions(window->window, &vkRequiredExtensionCount, vkRequiredExtensions.data() + nonSDLRequiredExtensionCount)) {
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
	appInfo.pEngineName = EngineName.c_str();
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

	if (!SDL_Vulkan_CreateSurface(window->window, vulkan_instance, &surface)) {
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
	primary_queue_family_index = primaryQueueFamilyIndex.value();
	present_queue = logicalDevice.getQueue(presentQueueFamilyIndex.value(), 0);
	present_queue_family_index = presentQueueFamilyIndex.value();

	/*
	* VMA allocator creation
	*/

	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
	allocatorInfo.physicalDevice = primary_physical_device;
	allocatorInfo.device = primary_logical_device;
	allocatorInfo.instance = vulkan_instance;

	if (vmaCreateAllocator(&allocatorInfo, &allocator) != VK_SUCCESS) {
		LOG_ERROR("Failed to create memory allocator");
		return;
	}

	/*
	* Create memory transfer command pool
	*/

	vk::CommandPoolCreateInfo memoryPoolInfo;
	memoryPoolInfo.queueFamilyIndex = primary_queue_family_index;
	memoryPoolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;

	memory_transfer_command_pool = primary_logical_device.createCommandPool(memoryPoolInfo);

	if (!memory_transfer_command_pool) {
		LOG_ERROR("Failed to create command pool");
		return;
	}

	/*
	* Finish initialization
	*/

	is_init = true;
}

void GfxContext::deinit() {
	if (is_initialized()) {

		primary_logical_device.destroy(memory_transfer_command_pool);

		if (allocator) {
			vmaDestroyAllocator(allocator);
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
	}

	is_init = false;
}

BufferAllocation GfxContext::create_vertex_buffer(vk::DeviceSize Size) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = Size;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocateInfo{};
	allocateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;

	VkBuffer buffer;
	VmaAllocation allocation;

	vmaCreateBuffer(allocator, &bufferInfo, &allocateInfo, &buffer, &allocation, nullptr);

	return { buffer, allocation };
}

BufferAllocation GfxContext::create_index_buffer(vk::DeviceSize Size) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = Size;
	bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocateInfo{};
	allocateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;

	VkBuffer buffer;
	VmaAllocation allocation;

	vmaCreateBuffer(allocator, &bufferInfo, &allocateInfo, &buffer, &allocation, nullptr);

	return { buffer, allocation };
}

BufferAllocation GfxContext::create_transfer_buffer(vk::DeviceSize Size) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = Size;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocateInfo{};
	allocateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY;

	VkBuffer buffer;
	VmaAllocation allocation;

	vmaCreateBuffer(allocator, &bufferInfo, &allocateInfo, &buffer, &allocation, nullptr);

	return { buffer, allocation };
}

BufferAllocation GfxContext::create_uniform_buffer(vk::DeviceSize Size) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = Size;
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocateInfo{};
	allocateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;

	VkBuffer buffer;
	VmaAllocation allocation;

	vmaCreateBuffer(allocator, &bufferInfo, &allocateInfo, &buffer, &allocation, nullptr);

	return { buffer, allocation };
}

BufferAllocation GfxContext::create_storage_buffer(vk::DeviceSize Size) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = Size;
	bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocateInfo{};
	allocateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;

	VkBuffer buffer;
	VmaAllocation allocation;

	vmaCreateBuffer(allocator, &bufferInfo, &allocateInfo, &buffer, &allocation, nullptr);

	return { buffer, allocation };
}

void GfxContext::destroy_buffer(BufferAllocation Buffer) {
	vmaDestroyBuffer(allocator, Buffer.buffer, Buffer.allocation);
}

void GfxContext::transfer_buffer_memory(BufferAllocation Dest, BufferAllocation Source, vk::DeviceSize Size) {
	vk::CommandBufferAllocateInfo commandAllocInfo;
	commandAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	commandAllocInfo.commandPool = memory_transfer_command_pool;
	commandAllocInfo.commandBufferCount = 1;

	vk::CommandBuffer transferCommandBuffer = primary_logical_device.allocateCommandBuffers(commandAllocInfo)[0];

	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	transferCommandBuffer.begin(beginInfo);

	vk::BufferCopy copyRegion;
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = Size;

	transferCommandBuffer.copyBuffer(Source.buffer, Dest.buffer, copyRegion);

	transferCommandBuffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCommandBuffer;

	primary_queue.submit(submitInfo, nullptr);
	primary_queue.waitIdle();

	primary_logical_device.freeCommandBuffers(memory_transfer_command_pool, transferCommandBuffer);
}

void GfxContext::upload_to_gpu_buffer(BufferAllocation Dest, void* Source, size_t Size) {
	BufferAllocation transferBuffer = create_transfer_buffer(Size);

	void* data;
	vmaMapMemory(allocator, transferBuffer.allocation, &data);
	std::memcpy(data, Source, Size);
	vmaUnmapMemory(allocator, transferBuffer.allocation);

	vmaFlushAllocation(allocator, transferBuffer.allocation, 0, Size);

	transfer_buffer_memory(Dest, transferBuffer, Size);

	destroy_buffer(transferBuffer);
}

vk::ShaderModule GfxContext::create_shader_module(std::filesystem::path path) {
	return create_shader_module(load_binary_asset(path).value);
}

vk::ShaderModule GfxContext::create_shader_module(const BinaryBlob& code) {
	vk::ShaderModuleCreateInfo createInfo;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	return primary_logical_device.createShaderModule(createInfo);
}