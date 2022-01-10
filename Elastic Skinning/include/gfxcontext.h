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

struct BufferAllocation {
	vk::Buffer buffer;
	VmaAllocation allocation;

	vk::DeviceSize size;
};

struct TextureAllocation {
	vk::Image image;
	VmaAllocation allocation;

	vk::Extent3D dimensions;
	vk::Format format;
};

class GfxContext {

	friend class RendererImpl;
	friend class Swapchain;
	friend class GfxPipelineImpl;

public:

	~GfxContext();

	void init(Window* Window, const std::string& AppName = "", const std::string& EngineName = "No Engine");
	void deinit();

	bool is_initialized() { return is_init; }

	vk::PhysicalDeviceProperties get_physical_device_properties();

	BufferAllocation create_vertex_buffer(vk::DeviceSize Size);
	BufferAllocation create_index_buffer(vk::DeviceSize Size);
	BufferAllocation create_transfer_buffer(vk::DeviceSize Size);
	BufferAllocation create_uniform_buffer(vk::DeviceSize Size);
	BufferAllocation create_storage_buffer(vk::DeviceSize Size);
	BufferAllocation create_buffer(vk::DeviceSize Size, vk::BufferUsageFlags Usage, vk::SharingMode SharingMode, VmaMemoryUsage Locality);
	void destroy_buffer(BufferAllocation Buffer);

	TextureAllocation create_texture_2d(vk::Extent2D Dimensions, vk::Format Format = vk::Format::eR8G8B8A8Srgb);
	TextureAllocation create_depth_buffer(vk::Extent2D Dimensions);
	TextureAllocation create_texture(
		vk::ImageType Type,
		vk::Format Format,
		vk::ImageUsageFlags Usage,
		vk::Extent3D Dimensions,
		uint32_t MipLevels = 1,
		uint32_t ArrayLayers = 1,
		vk::SampleCountFlags Samples = vk::SampleCountFlagBits::e1,
		VmaMemoryUsage Locality = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY
	);
	void destroy_texture(TextureAllocation Texture);

	void transfer_buffer_memory(BufferAllocation Dest, BufferAllocation Source, vk::DeviceSize Size);
	void transfer_buffer_to_texture(TextureAllocation Dest, BufferAllocation Source, vk::DeviceSize Size);
	
	void upload_to_gpu_buffer(BufferAllocation Dest, const BinaryBlob& Source);
	void upload_to_gpu_buffer(BufferAllocation Dest, const void* Source, size_t Size);
	void upload_texture(TextureAllocation Dest, const Image& Source);

	void transition_image_layout(TextureAllocation Texture, vk::Format Format, vk::ImageLayout Old, vk::ImageLayout New);

	vk::ShaderModule create_shader_module(std::filesystem::path path);
	vk::ShaderModule create_shader_module(const BinaryBlob& code);

	VmaAllocator allocator;

private:

	vk::CommandBuffer one_time_command_begin();
	void one_time_command_end(vk::CommandBuffer command);

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