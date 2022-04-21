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
	if (!features.geometryShader || !features.samplerAnisotropy) {
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

GfxContext::GfxContext(Window* Window, const std::string& AppName, const std::string& EngineName) {
	
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
	deviceFeatures.samplerAnisotropy = VK_TRUE;

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

GfxContext::~GfxContext() {
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

vk::PhysicalDeviceProperties GfxContext::get_physical_device_properties() {
	return primary_physical_device.getProperties();
}

BufferAllocation GfxContext::create_vertex_buffer(vk::DeviceSize Size) {
	return create_buffer(
		Size,
		vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
		vk::SharingMode::eExclusive,
		VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY
	);
}

BufferAllocation GfxContext::create_index_buffer(vk::DeviceSize Size) {
	return create_buffer(
		Size,
		vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
		vk::SharingMode::eExclusive,
		VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY
	);
}

BufferAllocation GfxContext::create_transfer_buffer(vk::DeviceSize Size) {
	return create_buffer(
		Size,
		vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
		vk::SharingMode::eExclusive,
		VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY
	);
}

BufferAllocation GfxContext::create_uniform_buffer(vk::DeviceSize Size) {
	return create_buffer(
		Size,
		vk::BufferUsageFlagBits::eUniformBuffer,
		vk::SharingMode::eExclusive,
		VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU
	);
}

BufferAllocation GfxContext::create_storage_buffer(vk::DeviceSize Size) {
	return create_buffer(
		Size,
		vk::BufferUsageFlagBits::eStorageBuffer,
		vk::SharingMode::eExclusive,
		VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU
	);
}

BufferAllocation GfxContext::create_gpu_storage_buffer(vk::DeviceSize Size) {
	return create_buffer(
		Size,
		vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
		vk::SharingMode::eExclusive,
		VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY
	);
}

BufferAllocation GfxContext::create_buffer(vk::DeviceSize Size, vk::BufferUsageFlags Usage, vk::SharingMode SharingMode, VmaMemoryUsage Locality) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = Size;
	bufferInfo.usage = (VkBufferUsageFlags)Usage;
	bufferInfo.sharingMode = (VkSharingMode)SharingMode;

	VmaAllocationCreateInfo allocateInfo{};
	allocateInfo.usage = Locality;

	VkBuffer buffer;
	VmaAllocation allocation;

	vmaCreateBuffer(allocator, &bufferInfo, &allocateInfo, &buffer, &allocation, nullptr);

	return { buffer, allocation, Size };
}

void GfxContext::destroy_buffer(BufferAllocation Buffer) {
	vmaDestroyBuffer(allocator, Buffer.buffer, Buffer.allocation);
}

TextureAllocation GfxContext::create_texture_2d(vk::Extent2D Dimensions, vk::Format Format) {
	return create_texture(
		vk::ImageType::e2D,
		Format,
		vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
		vk::Extent3D{ Dimensions, 1 }
	);
}

TextureAllocation GfxContext::create_texture_3d(vk::Extent3D Dimensions, vk::Format Format) {
	return create_texture(
		vk::ImageType::e3D,
		Format,
		vk::ImageUsageFlagBits::eSampled
		| vk::ImageUsageFlagBits::eStorage
		| vk::ImageUsageFlagBits::eTransferDst
		| vk::ImageUsageFlagBits::eTransferSrc,
		Dimensions
	);
}

TextureAllocation GfxContext::create_depth_buffer(vk::Extent2D Dimensions) {
	return create_texture(
		vk::ImageType::e2D,
		vk::Format::eD32Sfloat,
		vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
		vk::Extent3D{ Dimensions, 1 }
	);
}

TextureAllocation GfxContext::create_texture(vk::ImageType Type, vk::Format Format, vk::ImageUsageFlags Usage, vk::Extent3D Dimensions, uint32_t MipLevels, uint32_t ArrayLayers, vk::SampleCountFlags Samples, VmaMemoryUsage Locality) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.flags = 0;
	imageInfo.imageType = (VkImageType)Type;
	imageInfo.format = (VkFormat)Format;
	imageInfo.extent = Dimensions;
	imageInfo.mipLevels = MipLevels;
	imageInfo.arrayLayers = ArrayLayers;
	imageInfo.samples = (VkSampleCountFlagBits)(uint32_t)Samples;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = (VkImageUsageFlagBits)(uint32_t)Usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.queueFamilyIndexCount = 1;
	imageInfo.pQueueFamilyIndices = &primary_queue_family_index;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo allocateInfo{};
	allocateInfo.usage = Locality;

	VkImage image;
	VmaAllocation allocation;

	vmaCreateImage(allocator, &imageInfo, &allocateInfo, &image, &allocation, nullptr);

	return {
		image,
		allocation,
		Dimensions,
		Format
	};
}

void GfxContext::destroy_texture(TextureAllocation Texture) {
	vmaDestroyImage(allocator, Texture.image, Texture.allocation);
}

vk::ImageView GfxContext::create_image_view(TextureAllocation Texture, vk::ImageViewType Type) {
	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = Texture.image;
	viewInfo.viewType = Type;
	viewInfo.format = Texture.format;
	viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	return primary_logical_device.createImageView(viewInfo);
}

void GfxContext::destroy_image_view(vk::ImageView View) {
	primary_logical_device.destroy(View);
}

void GfxContext::transfer_buffer_memory(BufferAllocation Dest, BufferAllocation Source, vk::DeviceSize Size) {

	vk::CommandBuffer transferCommandBuffer = one_time_command_begin();

	vk::BufferCopy copyRegion;
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = Size;

	transferCommandBuffer.copyBuffer(Source.buffer, Dest.buffer, copyRegion);

	one_time_command_end(transferCommandBuffer);
}

void GfxContext::transfer_buffer_to_texture(TextureAllocation Dest, BufferAllocation Source, vk::DeviceSize Size) {
	
	vk::CommandBuffer transferCommandBuffer = one_time_command_begin();

	vk::BufferImageCopy copyRegion;
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;
	
	copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	copyRegion.imageSubresource.mipLevel = 0;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 1;

	copyRegion.imageOffset = vk::Offset3D{ 0, 0, 0 };
	copyRegion.imageExtent = Dest.dimensions;

	transferCommandBuffer.copyBufferToImage(Source.buffer, Dest.image, vk::ImageLayout::eTransferDstOptimal, copyRegion);

	one_time_command_end(transferCommandBuffer);
}


void GfxContext::transfer_texture_to_buffer(BufferAllocation Dest, TextureAllocation Source) {
	
	vk::CommandBuffer transferCommandBuffer = one_time_command_begin();

	vk::BufferImageCopy copyRegion;
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;

	copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	copyRegion.imageSubresource.mipLevel = 0;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 1;

	copyRegion.imageOffset = vk::Offset3D{ 0, 0, 0 };
	copyRegion.imageExtent = Source.dimensions;

	transferCommandBuffer.copyImageToBuffer(Source.image, vk::ImageLayout::eTransferSrcOptimal, Dest.buffer, copyRegion);

	one_time_command_end(transferCommandBuffer);
}

void GfxContext::upload_to_gpu_buffer(BufferAllocation Dest, const BinaryBlob& Source) {
	upload_to_gpu_buffer(Dest, Source.data(), Source.size());
}

void GfxContext::upload_to_gpu_buffer(BufferAllocation Dest, const void* Source, size_t Size) {
	BufferAllocation transferBuffer = create_transfer_buffer(Size);

	void* data;
	vmaMapMemory(allocator, transferBuffer.allocation, &data);
	std::memcpy(data, Source, Size);
	vmaUnmapMemory(allocator, transferBuffer.allocation);

	vmaFlushAllocation(allocator, transferBuffer.allocation, 0, Size);

	transfer_buffer_memory(Dest, transferBuffer, Size);

	destroy_buffer(transferBuffer);
}

void GfxContext::upload_texture(TextureAllocation Dest, const Image& Source) {
	upload_texture(Dest, Source.data);
}

void GfxContext::upload_texture(TextureAllocation Dest, const BinaryBlob& Source) {
	upload_texture(Dest, Source.data(), Source.size());
}

void GfxContext::upload_texture(TextureAllocation Dest, const void* Source, size_t Size) {
	transition_image_layout(Dest, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

	BufferAllocation transferBuffer = create_transfer_buffer(Size);
	
	void* data;
	vmaMapMemory(allocator, transferBuffer.allocation, &data);
	std::memcpy(data, Source, Size);
	vmaUnmapMemory(allocator, transferBuffer.allocation);

	vmaFlushAllocation(allocator, transferBuffer.allocation, 0, Size);

	transfer_buffer_to_texture(Dest, transferBuffer, transferBuffer.size);

	destroy_buffer(transferBuffer);

	transition_image_layout(Dest, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
}

BinaryBlob GfxContext::download_gpu_buffer(BufferAllocation Source) {
	BufferAllocation transferBuffer = create_transfer_buffer(Source.size);

	transfer_buffer_memory(transferBuffer, Source, Source.size);

	BinaryBlob ret;
	ret.resize(Source.size);

	void* data;
	vmaMapMemory(allocator, transferBuffer.allocation, &data);
	std::memcpy(ret.data(), data, Source.size);
	vmaUnmapMemory(allocator, transferBuffer.allocation);

	vmaFlushAllocation(allocator, transferBuffer.allocation, 0, Source.size);

	destroy_buffer(transferBuffer);

	return ret;
}

BinaryBlob GfxContext::download_texture(TextureAllocation Source) {
	transition_image_layout(Source, Source.format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal);

	size_t volume = Source.dimensions.width * Source.dimensions.height * Source.dimensions.depth;
	size_t bytesize = 1;

	switch (Source.format) {
	case vk::Format::eR32G32B32A32Sfloat: 
		bytesize = sizeof(glm::vec4);
		break;
	case vk::Format::eR32Sfloat:
		bytesize = sizeof(float);
		break;
	default:
		bytesize = 1;
		break;
	}

	size_t size = volume * bytesize;

	BufferAllocation transferBuffer = create_transfer_buffer(size);

	transfer_texture_to_buffer(transferBuffer, Source);
	
	transition_image_layout(Source, Source.format, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

	BinaryBlob ret;
	ret.resize(size);

	void* data;
	vmaMapMemory(allocator, transferBuffer.allocation, &data);
	std::memcpy(ret.data(), data, size);
	vmaUnmapMemory(allocator, transferBuffer.allocation);

	vmaFlushAllocation(allocator, transferBuffer.allocation, 0, size);

	destroy_buffer(transferBuffer);

	return ret;
}

void GfxContext::transition_image_layout(TextureAllocation Texture, vk::Format Format, vk::ImageLayout Old, vk::ImageLayout New) {
	vk::CommandBuffer transitionCommandBuffer = one_time_command_begin();

	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = Old;
	barrier.newLayout = New;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = Texture.image;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
	barrier.dstAccessMask = vk::AccessFlagBits::eNoneKHR;

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destStage;

	if (Old == vk::ImageLayout::eUndefined) {
		if (New == vk::ImageLayout::eTransferDstOptimal) {
			barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
			destStage = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (New == vk::ImageLayout::eTransferSrcOptimal) {
			barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

			sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
			destStage = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (New == vk::ImageLayout::eDepthAttachmentOptimal || New == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
			barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

			barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
			barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead
				| vk::AccessFlagBits::eDepthStencilAttachmentWrite;

			sourceStage = vk::PipelineStageFlagBits::eVertexShader;
			destStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		}
		else if (New == vk::ImageLayout::eGeneral) {
			barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead
				| vk::AccessFlagBits::eShaderWrite;

			sourceStage = vk::PipelineStageFlagBits::eComputeShader;
			destStage = vk::PipelineStageFlagBits::eComputeShader;
		}
	}
	else if ((Old == vk::ImageLayout::eTransferDstOptimal || Old == vk::ImageLayout::eTransferSrcOptimal) && New == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destStage = vk::PipelineStageFlagBits::eFragmentShader;
	}

	transitionCommandBuffer.pipelineBarrier(
		sourceStage,
		destStage,
		vk::DependencyFlagBits(0),
		nullptr,
		nullptr,
		barrier
	);

	one_time_command_end(transitionCommandBuffer);
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

vk::CommandBuffer GfxContext::one_time_command_begin() {
	vk::CommandBufferAllocateInfo commandAllocInfo;
	commandAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	commandAllocInfo.commandPool = memory_transfer_command_pool;
	commandAllocInfo.commandBufferCount = 1;

	vk::CommandBuffer ret = primary_logical_device.allocateCommandBuffers(commandAllocInfo)[0];

	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	ret.begin(beginInfo);

	return ret;
}

void GfxContext::one_time_command_end(vk::CommandBuffer command) {
	command.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &command;

	primary_queue.submit(submitInfo, nullptr);
	primary_queue.waitIdle();

	primary_logical_device.freeCommandBuffers(memory_transfer_command_pool, command);
}