#pragma once

#include <vulkan/vulkan.hpp>
#include <SDL.h>
#include <SDL_vulkan.h>

#include <vector>
#include <span>
#include <cstdint>

struct SwapchainContext {
	enum class SwapchainError {
		OK,
		FAIL_CREATE_SWAPCHAIN,
		FAIL_CREATE_IMAGE_VIEW,
		FAIL_CREATE_RENDER_PASS,
		FAIL_CREATE_FRAMEBUFFER
	};

	SwapchainError init(SDL_Window* Window, vk::SurfaceKHR RenderSurface, vk::PhysicalDevice PhysicalDevice, vk::Device LogicalDevice, std::span<uint32_t> QueueFamilyIndices);
	void deinit();

	vk::SwapchainKHR swapchain;
	vk::Format format;
	vk::Extent2D extent;
	std::vector<vk::Image> images;
	std::vector<vk::ImageView> image_views;
	vk::RenderPass render_pass;
	std::vector<vk::Framebuffer> framebuffers;

private:

	vk::Device creator;
};